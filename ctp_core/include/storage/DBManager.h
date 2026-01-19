#pragma once

#include <string>
#include <memory>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <iostream>
#include <vector>
#include <atomic>

// CTP Headers
#include "ThostFtdcUserApiStruct.h"
#include "protocol/message_schema.h"

// Forward declaration to avoid including pqxx in header if possible, 
// but for std::unique_ptr<pqxx::connection> we need complete type or custom deleter.
// So we include it. If user builds fail, they know why.
#include <pqxx/pqxx>

namespace atrad {

enum class DBTaskType {
    INSTRUMENT,
    ORDER,
    TRADE
};

struct DBTask {
    DBTaskType type;
    // We store copies of data.
    // Use std::variant or simple union or shared_ptr to struct?
    // Let's use specific structs for simplicity.
    InstrumentData instr;
    CThostFtdcOrderField order;
    CThostFtdcTradeField trade;
};

class DBManager {
public:
    static DBManager& instance() {
        static DBManager i;
        return i;
    }

    // Connect to DB. connStr format: "postgresql://user:pass@host:port/dbname"
    void init(const std::string& connStr);
    void stop();

    // 同步读取订阅列表 (用于启动时加载)
    std::vector<std::string> loadSubscriptions();

    void saveInstrument(const InstrumentData& data);
    void saveOrder(const CThostFtdcOrderField* pOrder);
    void saveTrade(const CThostFtdcTradeField* pTrade);

private:
    DBManager() = default;
    ~DBManager() { stop(); }

    void workerLoop();
    void processTask(pqxx::work& txn, const DBTask& task);

    std::unique_ptr<pqxx::connection> conn_;
    std::string connStr_;
    
    std::queue<DBTask> tasks_;
    std::mutex queueMutex_;
    std::condition_variable cv_;
    
    std::thread workerThread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
};

} // namespace atrad
