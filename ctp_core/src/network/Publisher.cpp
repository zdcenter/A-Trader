#include "network/Publisher.h"
#include "protocol/zmq_topics.h"
#include "utils/Encoding.h"
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
    nlohmann::json j;
    j["instrument_id"] = data.instrument_id;
    j["last_price"] = data.last_price;
    j["volume"] = data.volume;
    j["open_interest"] = data.open_interest;
    
    // 5档行情 使用标准数组或明确的字段
    j["bid_price1"] = data.bid_price1; j["bid_volume1"] = data.bid_volume1;
    j["ask_price1"] = data.ask_price1; j["ask_volume1"] = data.ask_volume1;
    j["bid_price2"] = data.bid_price2; j["bid_volume2"] = data.bid_volume2;
    j["ask_price2"] = data.ask_price2; j["ask_volume2"] = data.ask_volume2;
    j["bid_price3"] = data.bid_price3; j["bid_volume3"] = data.bid_volume3;
    j["ask_price3"] = data.ask_price3; j["ask_volume3"] = data.ask_volume3;
    j["bid_price4"] = data.bid_price4; j["bid_volume4"] = data.bid_volume4;
    j["ask_price4"] = data.ask_price4; j["ask_volume4"] = data.ask_volume4;
    j["bid_price5"] = data.bid_price5; j["bid_volume5"] = data.bid_volume5;
    j["ask_price5"] = data.ask_price5; j["ask_volume5"] = data.ask_volume5;
    
    j["update_time"] = data.update_time;
    j["update_millisec"] = data.update_millisec;
    
    j["turnover"] = data.turnover;
    j["pre_close_price"] = data.pre_close_price;
    j["pre_settlement_price"] = data.pre_settlement_price;
    j["upper_limit_price"] = data.upper_limit_price;
    j["lower_limit_price"] = data.lower_limit_price;
    j["open_price"] = data.open_price;
    j["highest_price"] = data.highest_price;
    j["lowest_price"] = data.lowest_price;
    j["close_price"] = data.close_price;
    j["settlement_price"] = data.settlement_price;
    j["average_price"] = data.average_price;
    j["action_day"] = data.action_day;
    j["trading_day"] = data.trading_day;

    std::string payload = j.dump();

    publisher_->send(zmq::message_t(zmq_topics::MARKET_DATA, 2), zmq::send_flags::sndmore);
    publisher_->send(zmq::message_t(payload.data(), payload.size()), zmq::send_flags::none);
}

void Publisher::publishPosition(const PositionData& data) {
    nlohmann::json j;
    j["instrument_id"] = data.instrument_id;
    j["direction"] = std::string(1, data.direction); // char to string
    j["position"] = data.position;
    j["today_position"] = data.today_position;
    j["yd_position"] = data.yd_position;
    j["position_cost"] = data.position_cost;
    j["pos_profit"] = data.pos_profit;

    std::string payload = j.dump();
    
    publisher_->send(zmq::message_t(zmq_topics::POSITION_DATA, 2), zmq::send_flags::sndmore);
    publisher_->send(zmq::message_t(payload.data(), payload.size()), zmq::send_flags::none);
}

void Publisher::publishAccount(const AccountData& data) {
    nlohmann::json j;
    j["balance"] = data.balance;
    j["available"] = data.available;
    j["margin"] = data.margin;
    j["frozen_margin"] = data.frozen_margin;
    j["commission"] = data.commission;
    j["close_profit"] = data.close_profit;

    std::string payload = j.dump();

    publisher_->send(zmq::message_t(zmq_topics::ACCOUNT_DATA, 2), zmq::send_flags::sndmore);
    publisher_->send(zmq::message_t(payload.data(), payload.size()), zmq::send_flags::none);
}

void Publisher::publishInstrument(const InstrumentData& data) {
    nlohmann::json j;
    j["instrument_id"] = data.instrument_id;
    j["instrument_name"] = atrad::utils::gbk_to_utf8(data.instrument_name);
    j["exchange_id"] = data.exchange_id;
    j["volume_multiple"] = data.volume_multiple;
    j["price_tick"] = data.price_tick;

    // 保证金率
    j["long_margin_ratio_by_money"] = data.long_margin_ratio_by_money;
    j["long_margin_ratio_by_volume"] = data.long_margin_ratio_by_volume;
    j["short_margin_ratio_by_money"] = data.short_margin_ratio_by_money;
    j["short_margin_ratio_by_volume"] = data.short_margin_ratio_by_volume;

    // 手续费率
    j["open_ratio_by_money"] = data.open_ratio_by_money;
    j["open_ratio_by_volume"] = data.open_ratio_by_volume;
    j["close_ratio_by_money"] = data.close_ratio_by_money;
    j["close_ratio_by_volume"] = data.close_ratio_by_volume;
    j["close_today_ratio_by_money"] = data.close_today_ratio_by_money;
    j["close_today_ratio_by_volume"] = data.close_today_ratio_by_volume;

    std::string payload = j.dump();

    publisher_->send(zmq::message_t(zmq_topics::INSTRUMENT_DATA, 2), zmq::send_flags::sndmore);
    publisher_->send(zmq::message_t(payload.data(), payload.size()), zmq::send_flags::none);
}

void Publisher::publishOrder(const CThostFtdcOrderField* pOrder) {
    if (!pOrder) return;
    nlohmann::json j;
    j["instrument_id"] = pOrder->InstrumentID;
    j["order_sys_id"] = pOrder->OrderSysID; // 报单编号
    j["order_ref"] = pOrder->OrderRef;      // 报单引用
    j["front_id"] = pOrder->FrontID;
    j["session_id"] = pOrder->SessionID;
    
    j["direction"] = std::string(1, pOrder->Direction); // 买卖方向
    j["comb_offset_flag"] = std::string(1, pOrder->CombOffsetFlag[0]); // 开平标志
    
    j["limit_price"] = pOrder->LimitPrice;
    j["volume_total_original"] = pOrder->VolumeTotalOriginal;
    j["volume_traded"] = pOrder->VolumeTraded;
    j["volume_total"] = pOrder->VolumeTotal; // 剩余数量
    
    j["order_status"] = std::string(1, pOrder->OrderStatus); // 报单状态
    j["status_msg"] = atrad::utils::gbk_to_utf8(pOrder->StatusMsg); // 状态信息
    j["exchange_id"] = pOrder->ExchangeID; // Added
    
    j["insert_time"] = pOrder->InsertTime;
    
    std::string payload = j.dump();
    publisher_->send(zmq::message_t(zmq_topics::ORDER_DATA, 2), zmq::send_flags::sndmore);
    publisher_->send(zmq::message_t(payload.data(), payload.size()), zmq::send_flags::none);
}

void Publisher::publishTrade(const CThostFtdcTradeField* pTrade, double commission, double close_profit) {
    if (!pTrade) return;
    nlohmann::json j;
    j["instrument_id"] = pTrade->InstrumentID;
    j["trade_id"] = pTrade->TradeID;
    j["order_sys_id"] = pTrade->OrderSysID;
    
    j["direction"] = std::string(1, pTrade->Direction);
    j["offset_flag"] = std::string(1, pTrade->OffsetFlag);
    
    j["price"] = pTrade->Price;
    j["volume"] = pTrade->Volume;
    j["trade_time"] = pTrade->TradeTime;
    j["trade_date"] = pTrade->TradeDate;
    j["exchange_id"] = pTrade->ExchangeID;
    
    j["commission"] = commission;
    j["close_profit"] = close_profit; // Added
    
    std::string payload = j.dump();
    publisher_->send(zmq::message_t(zmq_topics::TRADE_DATA, 2), zmq::send_flags::sndmore);
    publisher_->send(zmq::message_t(payload.data(), payload.size()), zmq::send_flags::none);
}

void Publisher::publishTrade(const TradeData& data) {
    nlohmann::json j;
    j["instrument_id"] = data.instrument_id;
    j["trade_id"] = data.trade_id;
    j["order_sys_id"] = data.order_sys_id;
    
    j["direction"] = std::string(1, data.direction);
    j["offset_flag"] = std::string(1, data.offset_flag);
    
    j["price"] = data.price;
    j["volume"] = data.volume;
    j["trade_time"] = data.trade_time;
    j["trade_date"] = data.trade_date;
    j["exchange_id"] = data.exchange_id;
    
    j["commission"] = data.commission;
    j["close_profit"] = data.close_profit;
    
    std::string payload = j.dump();
    publisher_->send(zmq::message_t(zmq_topics::TRADE_DATA, 2), zmq::send_flags::sndmore);
    publisher_->send(zmq::message_t(payload.data(), payload.size()), zmq::send_flags::none);
}

void Publisher::publish(const std::string& topic, const std::string& message) {
    zmq::message_t t(topic.c_str(), topic.length());
    zmq::message_t m(message.c_str(), message.length());
    
    publisher_->send(t, zmq::send_flags::sndmore);
    publisher_->send(m, zmq::send_flags::none);
}

} // namespace atrad
