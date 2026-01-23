#pragma once

#include "protocol/message_schema.h"
#include "ThostFtdcUserApiStruct.h"
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

namespace atrad {

class Publisher {
public:
    Publisher();
    ~Publisher();

    /**
     * @brief 初始化发布者，绑定端口
     * @param addr 地址，如 "tcp://*:5555"
     */
    void init(const std::string& addr);

    /**
     * @brief 发送行情数据
     */
    void publishTick(const TickData& data);

    /**
     * @brief 发送持仓数据
     */
    void publishPosition(const PositionData& data);

    /**
     * @brief 发送合约基础信息
     */
    void publishInstrument(const InstrumentData& data);

    /**
     * @brief 发送账户资金数据
     */
    void publishAccount(const AccountData& data);

    /**
     * @brief 发送报单回报
     */
    void publishOrder(const CThostFtdcOrderField* pOrder);

    /**
     * @brief 发送成交换单
     */
    void publishTrade(const CThostFtdcTradeField* pTrade);

    /**
     * @brief 发送通用消息 (Topic + Message)
     */
    void publish(const std::string& topic, const std::string& message);

private:
    std::unique_ptr<zmq::context_t> context_;
    std::unique_ptr<zmq::socket_t> publisher_;
};

} // namespace atrad
