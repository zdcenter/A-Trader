#include "network/Publisher.h"
#include "network/CommandServer.h"
#include "api/MdHandler.h"
#include "api/TraderHandler.h"
#include "storage/DBManager.h" // 正确位置
#include "protocol/zmq_topics.h"
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

    // 6. 初始化指令服务器 (接收前端下单)
    atrad::CommandServer cmd_server;
    int rep_port = j_config["zmq"].value("rep_port", 5556);
    std::string rep_addr = "tcp://*:" + std::to_string(rep_port);

    cmd_server.start(rep_addr, [&](const nlohmann::json& req){
        std::cout << "[Main] Received Command: " << req.dump() << std::endl;
        
        // 如果是下单指令
        if (req.contains("type") && req["type"] == "ORDER") {
            std::string id = req["id"];
            double price = req["price"];
            int vol = req["vol"];
            // 执行下单
            td_handler.insertOrder(id, price, vol, THOST_FTDC_D_Buy, THOST_FTDC_OF_Open);
            return "{\"status\":\"ok\",\"msg\":\"Order sent to CTP\"}";
        }
        
        // 订阅指令
        if (req.contains("type") && req["type"] == "SUBSCRIBE") {
            std::string id = req["id"];
            if (id.empty()) return "{\"status\":\"error\",\"msg\":\"Empty ID\"}";
            
            md_handler.subscribe(id);
            // 订阅行情后，顺便查一下合约信息，并放入队列查费率
            td_handler.qryInstrument(id);
            td_handler.queueRateQuery(id);
            return "{\"status\":\"ok\",\"msg\":\"Subscribed\"}";
        }

        // 取消订阅指令
        if (req.contains("type") && req["type"] == "UNSUBSCRIBE") {
            std::string id = req["id"];
            if (id.empty()) return "{\"status\":\"error\",\"msg\":\"Empty ID\"}";
            
            md_handler.unsubscribe(id);
            return "{\"status\":\"ok\",\"msg\":\"Unsubscribed\"}";
        }

        // PING 心跳指令
        if (req.contains("type") && req["type"] == "PING") {
            return "{\"type\":\"PONG\"}";
        }

        // 状态同步指令 (前端启动时主动拉取)
        if (req.contains("type") && req["type"] == "SYNC_STATE") {
            std::cout << "[Main] Sync State requested by Client." << std::endl;
            
            // 1. 触发资金和持仓的重新查询与推送
            td_handler.qryAccount();
            
            // 2. 推送当前 Core 内存中已有的持仓和合约信息
            // 这样新连接的 QT 客户端能立刻获得数据，而不需要经过 CTP 的慢速查询
            td_handler.pushCachedPositions();
            
            return "{\"status\":\"ok\",\"msg\":\"Sync started\"}";
        }

        return "{\"status\":\"ok\",\"msg\":\"Command Received\"}";
    });

    // 保持主线程运行
    md_handler.join();

    return 0;
}
