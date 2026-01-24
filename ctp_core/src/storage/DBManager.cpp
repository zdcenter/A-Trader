#include "storage/DBManager.h"
#include <chrono>
#include <thread>
#include <cstring>

namespace atrad {

void DBManager::init(const std::string& connStr) {
    if (running_) return;
    connStr_ = connStr;
    running_ = true;
    workerThread_ = std::thread(&DBManager::workerLoop, this);
}

void DBManager::stop() {
    running_ = false;
    cv_.notify_all();
    if (workerThread_.joinable()) workerThread_.join();
}

std::vector<std::string> DBManager::loadSubscriptions() {
    std::vector<std::string> subs;
    try {
        // 等待 connStr 被设置
        if (connStr_.empty()) {
            std::cerr << "[DB] Cannot load subs: DB not initialized." << std::endl;
            return subs;
        }

        pqxx::connection c(connStr_);
        // 强制设置客户端编码为 GBK (与数据一致性) 或 UTF8 (如果是存的UTF8)
        // 假设表里存的是 UTF8 (Postgres 默认)
        c.set_client_encoding("UTF8"); 
        
        pqxx::work txn(c);
        // 检查表是否存在
        try {
            pqxx::result r = txn.exec("SELECT instrument_id FROM tb_subscriptions ORDER BY sort_order ASC");
            for (auto row : r) {
                subs.push_back(row[0].as<std::string>());
            }
            std::cout << "[DB] Loaded " << subs.size() << " subscriptions." << std::endl;
        } catch (...) {
            std::cerr << "[DB] tb_subscriptions table missing or error." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[DB] Load Subs Error: " << e.what() << std::endl;
    }
    return subs;
}

void DBManager::addSubscription(const std::string& instrumentId) {
    if (connStr_.empty()) return;
    try {
        pqxx::connection c(connStr_);
        pqxx::work w(c);
        w.exec_params("INSERT INTO tb_subscriptions (instrument_id) VALUES ($1) ON CONFLICT (instrument_id) DO NOTHING", instrumentId);
        w.commit();
        std::cout << "[DB] Added subscription: " << instrumentId << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DB] Add Sub Error: " << e.what() << std::endl;
    }
}

void DBManager::removeSubscription(const std::string& instrumentId) {
    if (connStr_.empty()) return;
    try {
        pqxx::connection c(connStr_);
        pqxx::work w(c);
        w.exec_params("DELETE FROM tb_subscriptions WHERE instrument_id = $1", instrumentId);
        w.commit();
        std::cout << "[DB] Removed subscription: " << instrumentId << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DB] Remove Sub Error: " << e.what() << std::endl;
    }
}

void DBManager::saveInstrument(const InstrumentData& data) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (tasks_.size() > 5000) return; // 防止积压
    DBTask task;
    task.type = DBTaskType::INSTRUMENT;
    task.instr = data;
    tasks_.push(task);
    cv_.notify_one();
}

void DBManager::saveOrder(const CThostFtdcOrderField* pOrder, const std::string& strategy_id) {
    if (!pOrder) return;
    std::lock_guard<std::mutex> lock(queueMutex_);
    DBTask task;
    task.type = DBTaskType::ORDER;
    task.strategy_id = strategy_id;
    std::memcpy(&task.order, pOrder, sizeof(CThostFtdcOrderField));
    tasks_.push(task);
    cv_.notify_one();
}

void DBManager::saveTrade(const CThostFtdcTradeField* pTrade, const std::string& strategy_id) {
    if (!pTrade) return;
    std::lock_guard<std::mutex> lock(queueMutex_);
    DBTask task;
    task.type = DBTaskType::TRADE;
    task.strategy_id = strategy_id;
    std::memcpy(&task.trade, pTrade, sizeof(CThostFtdcTradeField));
    tasks_.push(task);
    cv_.notify_one();
}

void DBManager::saveConditionOrder(const ConditionOrderRequest& order) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    DBTask task;
    task.type = DBTaskType::CONDITION_ORDER;
    task.condition_order = order;
    tasks_.push(task);
    cv_.notify_one();
}

void DBManager::updateConditionOrderStatus(uint64_t request_id, int status) {
    if (connStr_.empty()) return;
    try {
        pqxx::connection c(connStr_);
        pqxx::work w(c);
        w.exec_params("UPDATE tb_condition_orders SET status = $1 WHERE request_id = $2", status, (long long)request_id);
        w.commit();
        std::cout << "[DB] Updated Condition Order " << request_id << " to Status " << status << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DB] Update Order Status Error: " << e.what() << std::endl;
    }
}

void DBManager::modifyConditionOrder(uint64_t request_id, double trigger_price, double limit_price, int volume) {
    if (connStr_.empty()) return;
    try {
        pqxx::connection c(connStr_);
        pqxx::work w(c);
        // 只允许修改状态为0（待触发）的条件单
        w.exec_params(
            "UPDATE tb_condition_orders SET trigger_price = $1, limit_price = $2, volume = $3 "
            "WHERE request_id = $4 AND status = 0",
            trigger_price, limit_price, volume, (long long)request_id
        );
        w.commit();
        std::cout << "[DB] Modified Condition Order " << request_id 
                  << " (trigger=" << trigger_price << ", limit=" << limit_price << ", vol=" << volume << ")" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DB] Modify Condition Order Error: " << e.what() << std::endl;
    }
}

std::vector<ConditionOrderRequest> DBManager::loadConditionOrders(bool onlyActive) {
    std::vector<ConditionOrderRequest> orders;
    if (connStr_.empty()) return orders;
    
    try {
        pqxx::connection c(connStr_);
        pqxx::work txn(c);
        
        // Check if table exists first
        pqxx::result table_check = txn.exec("SELECT to_regclass('public.tb_condition_orders')");
        if (table_check[0][0].is_null()) return orders;

        std::string sql;
        if (onlyActive) {
            // Load only Pending (0)
            sql = "SELECT instrument_id, trigger_price, compare_type, status, direction, offset_flag, volume, limit_price, request_id, strategy_id "
                  "FROM tb_condition_orders WHERE status = 0";
        } else {
            // Load ALL (Active + History), maybe limit for performance
            sql = "SELECT instrument_id, trigger_price, compare_type, status, direction, offset_flag, volume, limit_price, request_id, strategy_id "
                  "FROM tb_condition_orders ORDER BY insert_time DESC LIMIT 100";
        }

        pqxx::result r = txn.exec(sql);
        
        for (auto row : r) {
            ConditionOrderRequest o;
            std::memset(&o, 0, sizeof(o));
            
            std::string instr = row[0].as<std::string>();
            std::strncpy(o.instrument_id, instr.c_str(), sizeof(o.instrument_id));
            
            o.trigger_price = row[1].as<double>();
            o.compare_type = static_cast<CompareType>(row[2].as<int>());
            o.status = row[3].as<int>();
            
            std::string dir = row[4].as<std::string>();
            o.direction = dir.empty() ? '0' : dir[0];
            
            std::string off = row[5].as<std::string>();
            o.offset_flag = off.empty() ? '0' : off[0];
            
            o.volume = row[6].as<int>();
            o.limit_price = row[7].as<double>();
            o.request_id = row[8].as<long long>();

            if (!row[9].is_null()) {
                 std::string strat = row[9].as<std::string>();
                 std::strncpy(o.strategy_id, strat.c_str(), sizeof(o.strategy_id));
            }
            
            // Pending defaults
            o.price_type = '1'; // Limit
            
            orders.push_back(o);
        }
        std::cout << "[DB] Loaded " << orders.size() << " pending condition orders." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DB] Load Condition Orders Error: " << e.what() << std::endl;
    }
    return orders;
}

std::vector<std::pair<std::string, std::string>> DBManager::loadStrategies() {
    std::vector<std::pair<std::string, std::string>> strategies;
    if (connStr_.empty()) return strategies;
    try {
        pqxx::connection c(connStr_);
        // c.set_client_encoding("GB18030"); // Or UTF8 depending on strategy table 
        // Assuming Strategies have UTF8 or compatible names.
        
        pqxx::work txn(c);
        // Check table
        pqxx::result table_check = txn.exec("SELECT to_regclass('public.tb_strategies')");
        if (table_check[0][0].is_null()) return strategies;

        pqxx::result r = txn.exec("SELECT strategy_id, strategy_name FROM tb_strategies WHERE status != 9");
        for (auto row : r) {
            std::string id = row[0].as<std::string>();
            std::string name = row[1].as<std::string>();
            strategies.push_back({id, name});
        }
    } catch (const std::exception& e) {
        std::cerr << "[DB] Load Strategies Error: " << e.what() << std::endl;
    }
    return strategies;
}

std::vector<InstrumentData> DBManager::loadAllInstruments() {
    std::vector<InstrumentData> instruments;
    if (connStr_.empty()) return instruments;
    try {
        pqxx::connection c(connStr_);
        c.set_client_encoding("GB18030");
        pqxx::work txn(c);
        
        // Ensure table has trading_day column (handled by schema update, but runtime check keeps it safe-ish)
        // We just run query.
        pqxx::result r = txn.exec(
            "SELECT instrument_id, instrument_name, exchange_id, product_id, underlying_instr_id, "
            "volume_multiple, price_tick, "
            "long_margin_ratio_by_money, long_margin_ratio_by_volume, "
            "short_margin_ratio_by_money, short_margin_ratio_by_volume, "
            "open_ratio_by_money, open_ratio_by_volume, "
            "close_ratio_by_money, close_ratio_by_volume, "
            "close_today_ratio_by_money, close_today_ratio_by_volume, "
            "strike_price, trading_day "
            "FROM tb_instruments"
        );
        
        for (auto row : r) {
            InstrumentData data;
            std::memset(&data, 0, sizeof(data));
            
            std::string s;
            s = row[0].as<std::string>(); std::strncpy(data.instrument_id, s.c_str(), sizeof(data.instrument_id)-1);
            if (!row[1].is_null()) { s = row[1].as<std::string>(); std::strncpy(data.instrument_name, s.c_str(), sizeof(data.instrument_name)-1); }
            if (!row[2].is_null()) { s = row[2].as<std::string>(); std::strncpy(data.exchange_id, s.c_str(), sizeof(data.exchange_id)-1); }
            if (!row[3].is_null()) { s = row[3].as<std::string>(); std::strncpy(data.product_id, s.c_str(), sizeof(data.product_id)-1); }
            if (!row[4].is_null()) { s = row[4].as<std::string>(); std::strncpy(data.underlying_instr_id, s.c_str(), sizeof(data.underlying_instr_id)-1); }
            
            data.volume_multiple = row[5].as<int>();
            data.price_tick = row[6].as<double>();
            
            data.long_margin_ratio_by_money = row[7].as<double>();
            data.long_margin_ratio_by_volume = row[8].as<double>();
            data.short_margin_ratio_by_money = row[9].as<double>();
            data.short_margin_ratio_by_volume = row[10].as<double>();
            
            data.open_ratio_by_money = row[11].as<double>();
            data.open_ratio_by_volume = row[12].as<double>();
            data.close_ratio_by_money = row[13].as<double>();
            data.close_ratio_by_volume = row[14].as<double>();
            data.close_today_ratio_by_money = row[15].as<double>();
            data.close_today_ratio_by_volume = row[16].as<double>();
            
            if (!row[17].is_null()) data.strike_price = row[17].as<double>();
            if (!row[18].is_null()) {
                s = row[18].as<std::string>();
                std::strncpy(data.trading_day, s.c_str(), sizeof(data.trading_day)-1);
            }

            instruments.push_back(data);
        }
        std::cout << "[DB] Loaded " << instruments.size() << " cached instruments." << std::endl;
    } catch (const std::exception& e) {
            std::cerr << "[DB] Load Instruments Error: " << e.what() << std::endl;
    }
    return instruments;
}

void DBManager::workerLoop() {
    while (running_) {
        try {
            if (!conn_ || !conn_->is_open()) {
                std::cout << "[DB] Connecting to DB Encod=GB18030..." << std::endl;
                // 添加 client_encoding 参数以支持 CTP 的 GBK 字符
                conn_ = std::make_unique<pqxx::connection>(connStr_);
                // 强制设置客户端编码为 GBK (CTP 默认编码)
                conn_->set_client_encoding("GB18030");
                std::cout << "[DB] Connected." << std::endl;
            }

            pqxx::work txn(*conn_);
            
            int count = 0;
            while (running_ && count < 200) { // 批量提交 200 条
                DBTask task;
                {
                    std::unique_lock<std::mutex> lock(queueMutex_);
                    if (tasks_.empty()) {
                        if (count > 0) break; 
                        cv_.wait_for(lock, std::chrono::milliseconds(500));
                        if (tasks_.empty()) continue;
                    }
                    task = tasks_.front();
                    tasks_.pop();
                }
                
                processTask(txn, task);
                count++;
            }
            
            if (count > 0) {
                txn.commit();
                // std::cout << "[DB] Committed " << count << " records." << std::endl;
            } else {
                // 如果空闲，休息一下
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }

        } catch (const std::exception& e) {
            std::cerr << "[DB] Connection Error: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            conn_.reset(); 
        }
    }
}

void DBManager::processTask(pqxx::work& txn, const DBTask& task) {
    try {
        if (task.type == DBTaskType::INSTRUMENT) {
            const auto& d = task.instr;
            // 完整的 Upsert，包含所有费率字段
            txn.exec_params(
                "INSERT INTO tb_instruments ("
                "instrument_id, instrument_name, exchange_id, product_id, underlying_instr_id, strike_price, "
                "volume_multiple, price_tick, "
                "long_margin_ratio_by_money, long_margin_ratio_by_volume, "
                "short_margin_ratio_by_money, short_margin_ratio_by_volume, "
                "open_ratio_by_money, open_ratio_by_volume, "
                "close_ratio_by_money, close_ratio_by_volume, "
                "close_today_ratio_by_money, close_today_ratio_by_volume, "
                "last_update_timestamp, trading_day "
                ") VALUES ("
                "$1, $2, $3, $4, $5, $6, "
                "$7, $8, "
                "$9, $10, $11, $12, "
                "$13, $14, $15, $16, $17, $18, NOW(), $19"
                // 智能更新：如果新数据的静态字段为空或0（说明这可能是一次纯费率更新），则保留原数据库中的值
                ") ON CONFLICT (instrument_id) DO UPDATE SET "
                "instrument_name = CASE WHEN $2 IS NOT NULL AND $2 != '' THEN $2 ELSE tb_instruments.instrument_name END, "
                "exchange_id = CASE WHEN $3 IS NOT NULL AND $3 != '' THEN $3 ELSE tb_instruments.exchange_id END, "
                "product_id = CASE WHEN $4 IS NOT NULL AND $4 != '' THEN $4 ELSE tb_instruments.product_id END, "
                "underlying_instr_id = CASE WHEN $5 IS NOT NULL AND $5 != '' THEN $5 ELSE tb_instruments.underlying_instr_id END, "
                "strike_price = CASE WHEN $6 != 0 THEN $6 ELSE tb_instruments.strike_price END, "
                "volume_multiple = CASE WHEN $7 != 0 THEN $7 ELSE tb_instruments.volume_multiple END, "
                "price_tick = CASE WHEN $8 != 0 THEN $8 ELSE tb_instruments.price_tick END, "
                
                // 费率字段通常每次都是准的，可以直接更新 (或者也应用类似逻辑，视需求而定，这里假设费率更新总是带有有效值)
                "long_margin_ratio_by_money=$9, long_margin_ratio_by_volume=$10, "
                "short_margin_ratio_by_money=$11, short_margin_ratio_by_volume=$12, "
                "open_ratio_by_money=$13, open_ratio_by_volume=$14, "
                "close_ratio_by_money=$15, close_ratio_by_volume=$16, "
                "close_today_ratio_by_money=$17, close_today_ratio_by_volume=$18,"
                "last_update_timestamp=NOW(), trading_day=$19",
                
                d.instrument_id, d.instrument_name, d.exchange_id, d.product_id, d.underlying_instr_id, d.strike_price,
                d.volume_multiple, d.price_tick,
                d.long_margin_ratio_by_money, d.long_margin_ratio_by_volume,
                d.short_margin_ratio_by_money, d.short_margin_ratio_by_volume,
                d.open_ratio_by_money, d.open_ratio_by_volume,
                d.close_ratio_by_money, d.close_ratio_by_volume,
                d.close_today_ratio_by_money, d.close_today_ratio_by_volume,
                d.trading_day
            );
        }
        else if (task.type == DBTaskType::ORDER) {
            const auto& o = task.order;
            // 报单记录 (Upsert)
            // 注意: LimitPrice 可能很大，OrderRef 唯一性
            std::string dir(1, o.Direction);
            std::string offset(1, o.CombOffsetFlag[0]);
            std::string status(1, o.OrderStatus);
            
            // Added strategy_id at param $13
            txn.exec_params("INSERT INTO tb_orders (front_id, session_id, order_ref, instrument_id, exchange_id, limit_price, volume_total_original, direction, offset_flag, order_status, status_msg, insert_time, strategy_id) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13) ON CONFLICT (front_id, session_id, order_ref) DO UPDATE SET order_status=$10, status_msg=$11, volume_traded=tb_orders.volume_traded, strategy_id=$13",
                o.FrontID, o.SessionID, o.OrderRef, o.InstrumentID, o.ExchangeID, o.LimitPrice, o.VolumeTotalOriginal, 
                dir, offset, status, o.StatusMsg, o.InsertTime, task.strategy_id);
        }
        else if (task.type == DBTaskType::TRADE) {
            const auto& t = task.trade;
            std::string dir(1, t.Direction);
            std::string offset(1, t.OffsetFlag);
            
            // Added strategy_id at param $10
            txn.exec_params("INSERT INTO tb_trades (exchange_id, trade_id, order_ref, instrument_id, direction, offset_flag, price, volume, trade_time, strategy_id) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10) ON CONFLICT (exchange_id, trade_id, direction) DO NOTHING",
                 t.ExchangeID, t.TradeID, t.OrderRef, t.InstrumentID, dir, offset, t.Price, t.Volume, t.TradeTime, task.strategy_id);
        }
        else if (task.type == DBTaskType::CONDITION_ORDER) {
            const auto& o = task.condition_order;
            

            std::string dir(1, o.direction);
            std::string off(1, o.offset_flag);

            txn.exec_params("INSERT INTO tb_condition_orders (request_id, instrument_id, trigger_price, compare_type, status, direction, offset_flag, volume, limit_price, strategy_id) "
                            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10) "
                            "ON CONFLICT (request_id) DO UPDATE SET status=$5",
                            (long long)o.request_id, o.instrument_id, o.trigger_price, (int)o.compare_type, o.status,
                            dir, off, o.volume, o.limit_price, o.strategy_id);
        }
    } catch (const std::exception& e) {
        std::cerr << "[DB] Insert Error: " << e.what() << std::endl;
    }
}

} // namespace atrad
