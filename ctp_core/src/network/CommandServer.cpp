#line 1 "/home/zd/A-Trader/ctp_core/src/network/CommandServer.cpp"
#include "network/CommandServer.h"
#include <iostream>

namespace QuantLabs {

CommandServer::CommandServer() : context_(1) {}

CommandServer::~CommandServer() {
    stop();
}

void CommandServer::start(const std::string& addr, CommandCallback callback) {
    addr_ = addr;
    callback_ = callback;
    running_ = true;
    thread_ = std::thread(&CommandServer::run, this);
}

void CommandServer::stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}

void CommandServer::run() {
    zmq::socket_t socket(context_, zmq::socket_type::rep);
    socket.bind(addr_);
    
    // 设置超时，防止 join 阻塞
    int timeout = 1000;
    socket.set(zmq::sockopt::rcvtimeo, timeout);

    std::cout << "[CommandServer] Listening on " << addr_ << std::endl;

    while (running_) {
        zmq::message_t request;
        auto res = socket.recv(request, zmq::recv_flags::none);
        
        if (res) {
            std::string req_str(static_cast<char*>(request.data()), request.size());
            try {
                auto j = nlohmann::json::parse(req_str);
                std::string reply_str = callback_(j);
                
                zmq::message_t reply(reply_str.size());
                std::memcpy(reply.data(), reply_str.data(), reply_str.size());
                socket.send(reply, zmq::send_flags::none);
            } catch (const std::exception& e) {
                std::string err = "{\"status\":\"error\",\"msg\":\"" + std::string(e.what()) + "\"}";
                socket.send(zmq::message_t(err.data(), err.size()), zmq::send_flags::none);
            }
        }
    }
}

} // namespace QuantLabs
