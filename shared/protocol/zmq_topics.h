#pragma once

namespace atrad {
namespace zmq_topics {

/**
 * @brief ZMQ Topic 常量定义
 */

// 行情广播 (PUB/SUB)
static constexpr const char* MARKET_DATA = "MT"; // Market Tick
static constexpr const char* POSITION_DATA = "PT"; // Position Tick
static constexpr const char* ACCOUNT_DATA = "AT"; // Account Tick
static constexpr const char* INSTRUMENT_DATA = "IT"; // Instrument Tick
static constexpr const char* COMMAND     = "CM"; // Command

// 交易回报广播 (PUB/SUB)
inline const char* TRADE_REPORT = "TR";

// 策略控制指令 (REQ/REP)
inline const char* CMD_STRATEGY = "CMD_STR";

/**
 * @brief ZMQ 地址定义
 */

// 行情发布地址 (Port 5555)
inline const char* PUB_MARKET_ADDR = "tcp://*:5555";
inline const char* SUB_MARKET_ADDR = "tcp://127.0.0.1:5555";

// 指令接收地址 (Port 5556)
inline const char* REP_CMD_ADDR = "tcp://*:5556";
inline const char* REQ_CMD_ADDR = "tcp://127.0.0.1:5556";

} // namespace zmq_topics
} // namespace atrad
