#pragma once

#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <thread>
#include <atomic>
#include <functional>

namespace QuantLabs {

/**
 * @brief ZMQ REP 服务器
 * 接收来自前端的指令并回调处理
 */
class CommandServer {
public:
    CommandServer();
    ~CommandServer();

    using CommandCallback = std::function<std::string(const nlohmann::json&)>;

    void start(const std::string& addr, CommandCallback callback);
    void stop();

private:
    void run();

    std::string addr_;
    CommandCallback callback_;
    std::thread thread_;
    std::atomic<bool> running_{false};
    zmq::context_t context_;
};

} // namespace QuantLabs
