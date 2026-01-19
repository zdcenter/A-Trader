#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include "protocol/message_schema.h"

namespace atrad {

/**
 * @brief 异步落库工人
 * 使用线程池/单线程队列异步写入 PostgreSQL
 */
class DatabaseWorker {
public:
    DatabaseWorker();
    ~DatabaseWorker();

    void start(const std::string& conn_str);
    void stop();

    // 提交任务到队列
    void enqueueTick(const TickData& tick);

private:
    void workLoop();

    std::string conn_str_;
    std::queue<TickData> tick_queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread worker_thread_;
    std::atomic<bool> running_{false};
};

} // namespace atrad
