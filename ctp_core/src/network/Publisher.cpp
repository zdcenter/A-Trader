#include "network/Publisher.h"
#include "protocol/zmq_topics.h"
#include <iostream>

namespace atrad {

Publisher::Publisher() 
    : context_(std::make_unique<zmq::context_t>(1)),
      publisher_(std::make_unique<zmq::socket_t>(*context_, zmq::socket_type::pub)) {
}

Publisher::~Publisher() {
    publisher_->close();
}

void Publisher::init(const std::string& addr) {
    try {
        publisher_->bind(addr);
        std::cout << "[Publisher] ZMQ Publisher bound to " << addr << std::endl;
    } catch (const zmq::error_t& e) {
        std::cerr << "[Publisher] ZMQ Bind Error: " << e.what() << std::endl;
    }
}

void Publisher::publishTick(const TickData& data) {
    // 将结构体转换为 JSON
    nlohmann::json j;
    j["id"] = data.instrument_id;
    j["price"] = data.last_price;
    j["volume"] = data.volume;
    j["oi"] = data.open_interest;
    j["b1"] = data.bid_price1;
    j["bv1"] = data.bid_volume1;
    j["a1"] = data.ask_price1;
    j["av1"] = data.ask_volume1;
    
    // 5档行情
    j["b2"] = data.bid_price2; j["bv2"] = data.bid_volume2;
    j["a2"] = data.ask_price2; j["av2"] = data.ask_volume2;
    j["b3"] = data.bid_price3; j["bv3"] = data.bid_volume3;
    j["a3"] = data.ask_price3; j["av3"] = data.ask_volume3;
    j["b4"] = data.bid_price4; j["bv4"] = data.bid_volume4;
    j["a4"] = data.ask_price4; j["av4"] = data.ask_volume4;
    j["b5"] = data.bid_price5; j["bv5"] = data.bid_volume5;
    j["a5"] = data.ask_price5; j["av5"] = data.ask_volume5;
    
    j["time"] = data.update_time;
    j["ms"] = data.update_millisec;
    
    // 扩展字段
    j["turnover"] = data.turnover;
    j["pre_close"] = data.pre_close_price;
    j["pre_settlement"] = data.pre_settlement_price;
    j["limit_up"] = data.upper_limit_price;
    j["limit_down"] = data.lower_limit_price;
    j["open"] = data.open_price;
    j["high"] = data.highest_price;
    j["low"] = data.lowest_price;
    j["close"] = data.close_price;
    j["settlement"] = data.settlement_price;
    j["avg_price"] = data.average_price;
    j["action_day"] = data.action_day;
    j["trading_day"] = data.trading_day;

    std::string payload = j.dump();

    // 发送 Topic (MT)
    zmq::message_t topic_msg(zmq_topics::MARKET_DATA, 2);
    publisher_->send(topic_msg, zmq::send_flags::sndmore);

    // 发送 Payload
    zmq::message_t payload_msg(payload.data(), payload.size());
    publisher_->send(payload_msg, zmq::send_flags::none);

    // 日志 (测试用，正式环境可关闭)
    // std::cout << "[Publisher] Published: " << data.instrument_id << " " << data.last_price << std::endl;
}

void Publisher::publishPosition(const PositionData& data) {
    nlohmann::json j;
    j["id"] = data.instrument_id;
    j["dir"] = data.direction;
    j["pos"] = data.position;
    j["td"] = data.today_position;
    j["yd"] = data.yd_position;
    j["cost"] = data.position_cost;
    j["profit"] = data.pos_profit;

    std::string payload = j.dump();
    
    publisher_->send(zmq::message_t(zmq_topics::POSITION_DATA, 2), zmq::send_flags::sndmore);
    publisher_->send(zmq::message_t(payload.data(), payload.size()), zmq::send_flags::none);
}

void Publisher::publishAccount(const AccountData& data) {
    nlohmann::json j;
    j["bal"] = data.balance;
    j["avail"] = data.available;
    j["margin"] = data.margin;
    j["frozen"] = data.frozen_margin;
    j["comm"] = data.commission;

    std::string payload = j.dump();

    publisher_->send(zmq::message_t(zmq_topics::ACCOUNT_DATA, 2), zmq::send_flags::sndmore);
    publisher_->send(zmq::message_t(payload.data(), payload.size()), zmq::send_flags::none);
}

void Publisher::publishInstrument(const InstrumentData& data) {
    nlohmann::json j;
    j["id"] = data.instrument_id;
    j["exch"] = data.exchange_id;
    j["mult"] = data.volume_multiple;
    j["tick"] = data.price_tick;

    // 保证金率
    j["l_m_money"] = data.long_margin_ratio_by_money;
    j["l_m_vol"] = data.long_margin_ratio_by_volume;
    j["s_m_money"] = data.short_margin_ratio_by_money;
    j["s_m_vol"] = data.short_margin_ratio_by_volume;

    // 手续费率
    j["o_r_money"] = data.open_ratio_by_money;
    j["o_r_vol"] = data.open_ratio_by_volume;
    j["c_r_money"] = data.close_ratio_by_money;
    j["c_r_vol"] = data.close_ratio_by_volume;
    j["ct_r_money"] = data.close_today_ratio_by_money;
    j["ct_r_vol"] = data.close_today_ratio_by_volume;

    std::string payload = j.dump();

    publisher_->send(zmq::message_t(zmq_topics::INSTRUMENT_DATA, 2), zmq::send_flags::sndmore);
    publisher_->send(zmq::message_t(payload.data(), payload.size()), zmq::send_flags::none);
}

} // namespace atrad
