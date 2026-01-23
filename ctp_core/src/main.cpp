#include "network/Publisher.h"
#include "network/CommandServer.h"
#include "api/MdHandler.h"
#include "api/TraderHandler.h"
#include "storage/DBManager.h" // 正确位置
#include "protocol/zmq_topics.h"
#include "strategy/ConditionEngine.h" // Added
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <set>

#include <fstream>
#include <nlohmann/json.hpp>

// 使用 json别名方便使用
using json = nlohmann::json;

int main() {
    // 0. 读取配置文件
    std::ifstream config_file("config.json");
    if (!config_file.is_open()) {
        std::cerr << "[Main] Error: Cannot open config.json" << std::endl;
        return -1;
    }
    
    json j_config;
    try {
        config_file >> j_config;
    } catch (json::parse_error& e) {
        std::cerr << "[Main] JSON Config Parse Error: " << e.what() << std::endl;
        return -1;
    }

    // 1. 初始化发布者 (行情广播)
    atrad::Publisher pub;
    // 使用配置文件中的端口，如果不存在则默认 5555
    int pub_port = j_config["zmq"].value("pub_port", 5555);
    std::string pub_addr = "tcp://*:" + std::to_string(pub_port);
    pub.init(pub_addr);
    std::cout << "[Main] Publisher bound to " << pub_addr << std::endl;

    // 2. 初始化数据库
    std::string db_conn = j_config["database"]["connection_string"];
    atrad::DBManager::instance().init(db_conn);
    
    // 2.1 从数据库加载订阅列表
    std::vector<std::string> db_subs = atrad::DBManager::instance().loadSubscriptions();
    if (db_subs.empty()) {
        std::cout << "[Main] No subscriptions found in DB." << std::endl;
    }

    // 3. 准备配置 Map 传给 Handler
    std::map<std::string, std::string> handler_config;
    auto& ctp = j_config["ctp"];
    handler_config["broker_id"] = ctp["broker_id"];
    handler_config["user_id"] = ctp["user_id"];
    handler_config["password"] = ctp["password"];
    handler_config["md_front"] = ctp["md_front"];
    handler_config["td_front"] = ctp["td_front"];
    handler_config["user_product_info"] = ctp.value("user_product_info", "");
    handler_config["app_id"] = ctp["app_id"];
    handler_config["auth_code"] = ctp["auth_code"];

    // 构造 sub_list 字符串 (兼容旧逻辑)
    std::string sub_list_str;
    for(const auto& s : db_subs) sub_list_str += s + ",";
    handler_config["sub_list"] = sub_list_str;
    
    // 4. 初始化行情处理器 (MdHandler 需要 set)
    std::set<std::string> sub_set(db_subs.begin(), db_subs.end());
    atrad::MdHandler md_handler(pub, handler_config, sub_set);
    
    // 5. 初始化交易处理器
    atrad::TraderHandler td_handler(handler_config, pub);
    
    // 5.1 注册费率查询任务
    for (const auto& id : db_subs) {
        td_handler.queueRateQuery(id);
    }

    // 5.2 初始化条件单引擎
    auto condition_engine = std::make_unique<atrad::ConditionEngine>(td_handler);
    
    // 5.3 从数据库恢复未完成的条件单
    auto restored_orders = atrad::DBManager::instance().loadConditionOrders();
    for (const auto& o : restored_orders) {
        // addConditionOrder 会重新 upsert 到 DB (无害) 并加入内存监控
        condition_engine->addConditionOrder(o);
    }
    std::cout << "[Main] Restored " << restored_orders.size() << " pending condition orders from DB." << std::endl;
    
    // 连接行情回调
    md_handler.setTickCallback([&](const CThostFtdcDepthMarketDataField* data) {
        condition_engine->onTick(data);
    });

    // 连接条件单状态回调 (Push to Frontend)
    condition_engine->setStatusCallback([&](const atrad::ConditionOrderRequest& order) {
        json j;
        j["type"] = atrad::CmdType::RtnConditionOrder;
        
        json data;
        data["request_id"] = order.request_id;
        data["instrument_id"] = order.instrument_id;
        data["trigger_price"] = order.trigger_price;
        data["compare_type"] = (int)order.compare_type;
        data["status"] = order.status;
        data["direction"] = std::string(1, order.direction);
        data["offset_flag"] = std::string(1, order.offset_flag);
        data["volume"] = order.volume;
        data["limit_price"] = order.limit_price;
        data["strategy_id"] = order.strategy_id;
        
        j["data"] = data;
        pub.publish(atrad::TOPIC_STRATEGY, j.dump());
    });

    // 6. 初始化指令服务器 (接收前端下单)
    atrad::CommandServer cmd_server;
    int rep_port = j_config["zmq"].value("rep_port", 5556);
    std::string rep_addr = "tcp://*:" + std::to_string(rep_port);

    // [优化] 定义命令处理器映射
    using CommandHandler = std::function<std::string(const json&)>;
    std::map<std::string, CommandHandler> handlers;

    // 1. 下单
    handlers[atrad::CmdType::Order] = [&](const json& req) {
        std::string id = req["id"];
        double price = req["price"];
        int vol = req["vol"];
        // 简单示例只买开，后续需完善解析 dir/offset
        td_handler.insertOrder(id, price, vol, THOST_FTDC_D_Buy, THOST_FTDC_OF_Open);
        return "{\"status\":\"ok\",\"msg\":\"Order sent to CTP\"}";
    };

    // 2. 订阅
    handlers[atrad::CmdType::Subscribe] = [&](const json& req) {
        std::string id = req["id"];
        if (id.empty()) return "{\"status\":\"error\",\"msg\":\"Empty ID\"}";
        md_handler.subscribe(id);
        td_handler.queueRateQuery(id);
        return "{\"status\":\"ok\",\"msg\":\"Subscribed\"}";
    };

    // 3. 退订
    handlers[atrad::CmdType::Unsubscribe] = [&](const json& req) {
        std::string id = req["id"];
        if (id.empty()) return "{\"status\":\"error\",\"msg\":\"Empty ID\"}";
        md_handler.unsubscribe(id);
        return "{\"status\":\"ok\",\"msg\":\"Unsubscribed\"}";
    };

    // 4. 心跳
    handlers[atrad::CmdType::Ping] = [](const json&) {
        return "{\"type\":\"" + atrad::CmdType::Pong + "\"}";
    };

    // 5. 状态同步
    handlers[atrad::CmdType::SyncState] = [&](const json&) {
        std::cout << "[Main] Sync State requested by Client." << std::endl;
        td_handler.qryAccount();
        td_handler.pushCachedPositions();
        td_handler.pushCachedInstruments();
        return "{\"status\":\"ok\",\"msg\":\"Sync started\"}";
    };

    // 6. 条件单
    handlers[atrad::CmdType::ConditionOrderInsert] = [&](const json& req) {
        // std::cout << "[Main] Condition Order Received: " << req["data"].dump() << std::endl;
        auto& d = req["data"];
        
        atrad::ConditionOrderRequest order;
        std::strncpy(order.instrument_id, std::string(d["instrument_id"]).c_str(), sizeof(order.instrument_id));
        order.trigger_price = d["trigger_price"];
        order.compare_type = static_cast<atrad::CompareType>(d["condition_compare"].get<int>());
        
        // Map Int to CTP Char
        int dir_idx = d["direction"];
        order.direction = (dir_idx == 0) ? THOST_FTDC_D_Buy : THOST_FTDC_D_Sell;
        
        int off_idx = d["offset_flag"];
        if (off_idx == 0) order.offset_flag = THOST_FTDC_OF_Open;
        else if (off_idx == 1) order.offset_flag = THOST_FTDC_OF_Close;
        else if (off_idx == 2) order.offset_flag = THOST_FTDC_OF_CloseToday;
        else order.offset_flag = THOST_FTDC_OF_Open;

        int pt_idx = d["price_type"];
        // 0: Fix, 1: Last, 2: Opp, 3: Mkt
        order.price_type = '0' + pt_idx;

        order.limit_price = d["limit_price"];
        order.tick_offset = d["tick_offset"];
        order.volume = d["volume"];
        std::strncpy(order.strategy_id, std::string(d["strategy_id"]).c_str(), sizeof(order.strategy_id));
        
        // Generate a request ID (timestamp based for now)
        order.request_id = std::chrono::system_clock::now().time_since_epoch().count();
        order.status = 0;

        condition_engine->addConditionOrder(order);
        
        return "{\"status\":\"ok\",\"msg\":\"Condition Order Accepted\"}";
    };

    handlers[atrad::CmdType::ConditionOrderCancel] = [&](const json& req) -> std::string {
        if (!req.contains("data")) return "{\"status\":\"error\",\"msg\":\"No data field\"}";
        auto& data = req["data"];
        if (!data.contains("request_id")) return "{\"status\":\"error\",\"msg\":\"Missing request_id\"}";

        uint64_t req_id = data["request_id"].get<uint64_t>();
        bool found = condition_engine->removeConditionOrder(req_id);
        
        if (found) {
             return "{\"status\":\"ok\",\"msg\":\"Condition Order Cancelled\"}";
        } else {
             return "{\"status\":\"error\",\"msg\":\"Order Not Found\"}";
        }
    };

    handlers[atrad::CmdType::ConditionOrderModify] = [&](const json& req) -> std::string {
        if (!req.contains("data")) return "{\"status\":\"error\",\"msg\":\"No data field\"}";
        auto& data = req["data"];
        if (!data.contains("request_id") || !data.contains("trigger_price") || 
            !data.contains("limit_price") || !data.contains("volume")) {
            return "{\"status\":\"error\",\"msg\":\"Missing required fields\"}";
        }

        uint64_t req_id = data["request_id"].get<uint64_t>();
        double trigger_price = data["trigger_price"].get<double>();
        double limit_price = data["limit_price"].get<double>();
        int volume = data["volume"].get<int>();
        
        bool found = condition_engine->modifyConditionOrder(req_id, trigger_price, limit_price, volume);
        
        if (found) {
             return "{\"status\":\"ok\",\"msg\":\"Condition Order Modified\"}";
        } else {
             return "{\"status\":\"error\",\"msg\":\"Order Not Found or Already Triggered/Cancelled\"}";
        }
    };

    handlers[atrad::CmdType::ConditionOrderQuery] = [&](const json&) -> std::string {
        std::cout << "[Main] Received ConditionOrderQuery..." << std::endl;
        // Load ALL (active + history) for frontend display
        auto pending_orders = atrad::DBManager::instance().loadConditionOrders(false);
        std::cout << "[Main] Query: Found " << pending_orders.size() << " orders in DB. Pushing to ZMQ..." << std::endl;
        
        // Push each pending order to frontend via PUB
        for (const auto& o : pending_orders) {
            json j;
            j["type"] = atrad::CmdType::RtnConditionOrder;
            
            json data;
            data["request_id"] = o.request_id;
            data["instrument_id"] = o.instrument_id;
            data["trigger_price"] = o.trigger_price;
            data["compare_type"] = (int)o.compare_type;
            data["status"] = o.status;
            data["direction"] = std::string(1, o.direction);
            data["offset_flag"] = std::string(1, o.offset_flag);
            data["volume"] = o.volume;
            data["limit_price"] = o.limit_price;
            data["strategy_id"] = o.strategy_id;
            
            j["data"] = data;
            pub.publish(atrad::TOPIC_STRATEGY, j.dump());
        }

        return "{\"status\":\"ok\",\"msg\":\"Query Request Accepted, Pushing Data...\"}";
    };

    handlers[atrad::CmdType::StrategyQuery] = [&](const json&) -> std::string {
        auto strategies = atrad::DBManager::instance().loadStrategies();
        json j_list = json::array();
        for (const auto& p : strategies) {
            j_list.push_back({{"id", p.first}, {"name", p.second}});
        }
        json msg;
        msg["type"] = atrad::CmdType::RtnStrategyList;
        msg["data"] = j_list;
        
        // Also push to generic strategy topic for async updates
        pub.publish(atrad::TOPIC_STRATEGY, msg.dump());
        
        return msg.dump();
    };

    // 启动服务，分发指令
    cmd_server.start(rep_addr, [&](const nlohmann::json& req) -> std::string {
        // std::cout << "[Main] Received Command: " << req.dump() << std::endl;
        
        if (req.contains("type")) {
            std::string type = req["type"];
            auto it = handlers.find(type);
            if (it != handlers.end()) {
                try {
                    return it->second(req);
                } catch (const std::exception& e) {
                    std::cerr << "[Main] Handler Error: " << e.what() << std::endl;
                    return std::string("{\"status\":\"error\",\"msg\":\"Internal Handler Error\"}");
                }
            }
        }
        return std::string("{\"status\":\"warning\",\"msg\":\"Unknown Command Type\"}");
    });

    // 保持主线程运行
    md_handler.join();

    return 0;
}
