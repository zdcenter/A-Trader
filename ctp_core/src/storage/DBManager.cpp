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

void DBManager::workerLoop() {
    while (running_) {
        try {
            if (!conn_ || !conn_->is_open()) {
                std::cout << "[DB] Connecting to DB Encod=GB18030..." << std::endl;
                // 添加 client_encoding 参数以支持 CTP 的 GBK 字符
                // 用户需要在连接字符串中可能已经指定，或者通过 options 指定
                // 这里我们假设 connStr 足够，或者之后执行 SET
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
                "last_update_timestamp"
                ") VALUES ("
                "$1, $2, $3, $4, $5, $6, "
                "$7, $8, "
                "$9, $10, $11, $12, "
                "$13, $14, $15, $16, $17, $18, NOW()"
                ") ON CONFLICT (instrument_id) DO UPDATE SET "
                "instrument_name=$2, exchange_id=$3, product_id=$4, underlying_instr_id=$5, strike_price=$6, "
                "volume_multiple=$7, price_tick=$8, "
                "long_margin_ratio_by_money=$9, long_margin_ratio_by_volume=$10, "
                "short_margin_ratio_by_money=$11, short_margin_ratio_by_volume=$12, "
                "open_ratio_by_money=$13, open_ratio_by_volume=$14, "
                "close_ratio_by_money=$15, close_ratio_by_volume=$16, "
                "close_today_ratio_by_money=$17, close_today_ratio_by_volume=$18,"
                "last_update_timestamp=NOW()",
                
                d.instrument_id, d.instrument_name, d.exchange_id, d.product_id, d.underlying_instr_id, d.strike_price,
                d.volume_multiple, d.price_tick,
                d.long_margin_ratio_by_money, d.long_margin_ratio_by_volume,
                d.short_margin_ratio_by_money, d.short_margin_ratio_by_volume,
                d.open_ratio_by_money, d.open_ratio_by_volume,
                d.close_ratio_by_money, d.close_ratio_by_volume,
                d.close_today_ratio_by_money, d.close_today_ratio_by_volume
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
    } catch (const std::exception& e) {
        std::cerr << "[DB] Insert Error: " << e.what() << std::endl;
    }
}

} // namespace atrad
