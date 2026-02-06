#pragma once

#include <string>

namespace QuantLabs {
namespace zmq_topics {

/**
 * @brief ZMQ Topic 常量定义
 */

// 行情广播 (PUB/SUB)
static constexpr const char* MARKET_DATA = "MT"; // Market Tick
static constexpr const char* POSITION_DATA = "PT"; // Position Tick
static constexpr const char* ACCOUNT_DATA = "AT"; // Account Tick
static constexpr const char* INSTRUMENT_DATA = "IT"; // Instrument Tick
static constexpr const char* ORDER_DATA = "OT"; // Order Update
static constexpr const char* TRADE_DATA = "TT"; // Trade Update
static constexpr const char* COMMAND     = "CM"; // Command

// 交易回报广播 (PUB/SUB)
inline const char* TRADE_REPORT = "TR";

// 策略控制指令 (REQ/REP)
inline const char* CMD_STRATEGY = "CMD_STR";

/**
 * @brief ZMQ 地址定义 - 支持动态配置
 */

// 默认地址（用于 Core 端绑定）
inline const char* PUB_MARKET_ADDR = "tcp://*:5555";
inline const char* REP_CMD_ADDR = "tcp://*:5556";

// 客户端连接地址（可通过 Config 类动态配置）
class Config {
public:
    static Config& instance() {
        static Config inst;
        return inst;
    }
    
    void setServerAddress(const std::string& addr) { 
        server_addr_ = addr; 
        updateAddresses();
    }
    
    void setPubPort(int port) { 
        pub_port_ = port; 
        updateAddresses();
    }
    
    void setRepPort(int port) { 
        rep_port_ = port; 
        updateAddresses();
    }
    
    std::string getSubMarketAddr() const { return sub_market_addr_; }
    std::string getReqCmdAddr() const { return req_cmd_addr_; }
    
private:
    Config() : server_addr_("127.0.0.1"), pub_port_(5555), rep_port_(5556) {
        updateAddresses();
    }
    
    void updateAddresses() {
        sub_market_addr_ = "tcp://" + server_addr_ + ":" + std::to_string(pub_port_);
        req_cmd_addr_ = "tcp://" + server_addr_ + ":" + std::to_string(rep_port_);
    }
    
    std::string server_addr_;
    int pub_port_;
    int rep_port_;
    std::string sub_market_addr_;
    std::string req_cmd_addr_;
};

// 兼容旧代码的静态地址（默认 localhost）
inline const char* SUB_MARKET_ADDR = "tcp://127.0.0.1:5555";
inline const char* REQ_CMD_ADDR = "tcp://127.0.0.1:5556";

} // namespace zmq_topics

static constexpr const char* TOPIC_STRATEGY = "ST"; // Added for Strategy/Condition updates

} // namespace QuantLabs

