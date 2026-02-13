#include "api/MdHandler.h"
#include <iostream>
#include <cstring>
#include <filesystem>
#include <vector>

#include "storage/DBManager.h"

namespace QuantLabs {

MdHandler::MdHandler(Publisher& pub, std::map<std::string, std::string> config, std::set<std::string> contracts) 
    : md_api_(nullptr), pub_(pub), contracts_(contracts) {
    
    broker_id_ = config["broker_id"];
    user_id_ = config["user_id"];
    password_ = config["password"];
    md_front_ = config["md_front"];
    
    std::cout << "[Md] Initializing Handler..." << std::endl;
    std::cout << "[Md] BrokerID: " << broker_id_ << ", UserID: " << user_id_ << std::endl;
    std::cout << "[Md] Binary Tick Mode: ON (Forced)" << std::endl;
    std::cout << "[Md] Front: " << md_front_ << std::endl;

    // 创建流水目录: ./flow/md/BROKER_ID/USER_ID/
    std::string flow_dir = "./flow/md/" + broker_id_ + "/" + user_id_ + "/";
    try {
        if (!std::filesystem::exists(flow_dir)) {
            std::filesystem::create_directories(flow_dir);
        }
    } catch (const std::exception& e) {
        std::cerr << "[Md] Failed to create flow directory: " << e.what() << std::endl;
    }

    md_api_ = CThostFtdcMdApi::CreateFtdcMdApi(flow_dir.c_str());
    md_api_->RegisterSpi(this);
    md_api_->RegisterFront(const_cast<char*>(md_front_.c_str()));
    
    std::cout << "[Md] Starting API Init..." << std::endl;
    md_api_->Init();
}

MdHandler::~MdHandler() {
    if (md_api_) {
        md_api_->RegisterSpi(nullptr);
        md_api_->Release();
        md_api_ = nullptr;
    }
}

void MdHandler::join() {
    if (md_api_) {
        md_api_->Join();
    }
}   

void MdHandler::OnFrontConnected() {
    std::cout << "[Md] Front Connected. Logging in..." << std::endl;
    login();
}

void MdHandler::login() {
    CThostFtdcReqUserLoginField req;
    std::memset(&req, 0, sizeof(req));
    std::strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    std::strncpy(req.UserID, user_id_.c_str(), sizeof(req.UserID));
    std::strncpy(req.Password, password_.c_str(), sizeof(req.Password));
    
    if (md_api_) {
        int res = md_api_->ReqUserLogin(&req, 0);
        if (res != 0) {
            std::cerr << "[Md] ReqUserLogin failed, res=" << res << std::endl;
        } else {
            std::cout << "[Md] Sent ReqUserLogin" << std::endl;
        }
    }
}

void MdHandler::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID == 0) {
        std::cout << "[Md] Login Success. Subscribing..." << std::endl;
        subscribe();
    } else {
        std::cerr << "[Md] Login Failed: " << (pRspInfo ? pRspInfo->ErrorMsg : "Unknown") << " (ErrorID: " << (pRspInfo ? pRspInfo->ErrorID : -1) << ")" << std::endl;
    }
}

void MdHandler::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pData) {
    if (!pData) return;

    // CTP 有时会推送 DBL_MAX 作为空值，必须过滤
    // 1e12 (一万亿) 是一个安全的上限，远超任何真实价格
    if (pData->LastPrice > 1e12) return; 
    
    // 如果您确定不交易组合合约(Spread)，保留 < 0 过滤也是好的防错措施
    // 但如果做组合合约，则应移除 || pData->LastPrice < 0
    if (pData->LastPrice < 0) return; 

    // 只有数据有效才触发回调和后续处理
    if (tick_callback_) tick_callback_(pData);

    TickData tick;
    std::memset(&tick, 0, sizeof(tick));
    
    std::strncpy(tick.instrument_id, pData->InstrumentID, sizeof(tick.instrument_id));
    tick.last_price = pData->LastPrice;
    tick.volume = pData->Volume;
    tick.open_interest = pData->OpenInterest;
    tick.turnover = pData->Turnover;
    
    // 填充扩展字段
    tick.pre_settlement_price = pData->PreSettlementPrice;
    tick.pre_close_price = pData->PreClosePrice;
    tick.upper_limit_price = pData->UpperLimitPrice;
    tick.lower_limit_price = pData->LowerLimitPrice;
    tick.open_price = pData->OpenPrice;
    tick.highest_price = pData->HighestPrice;
    tick.lowest_price = pData->LowestPrice;
    tick.close_price = pData->ClosePrice;
    tick.settlement_price = pData->SettlementPrice;
    tick.average_price = pData->AveragePrice;
    
    std::strncpy(tick.action_day, pData->ActionDay, sizeof(tick.action_day));
    std::strncpy(tick.trading_day, pData->TradingDay, sizeof(tick.trading_day));

    tick.bid_price1 = pData->BidPrice1;
    tick.bid_volume1 = pData->BidVolume1;
    tick.ask_price1 = pData->AskPrice1;
    tick.ask_volume1 = pData->AskVolume1;
    
    // 尝试读取 2-5 档 (需要 CTP 头文件支持)
    // 这里的字段名通常是 BidPrice2, BidVolume2 等
    tick.bid_price2 = pData->BidPrice2; tick.bid_volume2 = pData->BidVolume2;
    tick.ask_price2 = pData->AskPrice2; tick.ask_volume2 = pData->AskVolume2;
    tick.bid_price3 = pData->BidPrice3; tick.bid_volume3 = pData->BidVolume3;
    tick.ask_price3 = pData->AskPrice3; tick.ask_volume3 = pData->AskVolume3;
    tick.bid_price4 = pData->BidPrice4; tick.bid_volume4 = pData->BidVolume4;
    tick.ask_price4 = pData->AskPrice4; tick.ask_volume4 = pData->AskVolume4;
    tick.bid_price5 = pData->BidPrice5; tick.bid_volume5 = pData->BidVolume5;
    tick.ask_price5 = pData->AskPrice5; tick.ask_volume5 = pData->AskVolume5;

    std::strncpy(tick.update_time, pData->UpdateTime, sizeof(tick.update_time));
    tick.update_millisec = pData->UpdateMillisec;

    // std::cout << "[Md] Received tick: " << tick.instrument_id << " " << tick.last_price << " " << tick.update_time << std::endl;
    
    // Binary Transport (High Performance)
    pub_.publishTickBinary(tick);
}

void MdHandler::subscribe() {
    if (md_api_ && !contracts_.empty()) {
        std::vector<char*> ids;
        for (const auto& s : contracts_) {
            ids.push_back(const_cast<char*>(s.c_str()));
        }
        int res = md_api_->SubscribeMarketData(ids.data(), ids.size());
        std::cout << "[Md] Sent Subscription for " << ids.size() << " instruments, res=" << res << std::endl;
    }
}

void MdHandler::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        std::cerr << "[Md] Sub Error: " << pRspInfo->ErrorMsg << std::endl;
    } else if (pSpecificInstrument) {
        std::cout << "[Md] Subscribed confirmed: " << pSpecificInstrument->InstrumentID << std::endl;
    }
}

void MdHandler::subscribe(const std::string& instrument) {
    if (md_api_) {
        char* id = const_cast<char*>(instrument.c_str());
        md_api_->SubscribeMarketData(&id, 1);
        contracts_.insert(instrument);
        std::cout << "[Md] Sent live subscription for: " << instrument << std::endl;
        
        // 持久化订阅
        DBManager::instance().addSubscription(instrument);
    }
}

void MdHandler::unsubscribe(const std::string& instrument) {
    if (md_api_) {
        char* id = const_cast<char*>(instrument.c_str());
        md_api_->UnSubscribeMarketData(&id, 1);
        contracts_.erase(instrument);
        std::cout << "[Md] Sent live unsubscription for: " << instrument << std::endl;
        
        // 移除订阅
        DBManager::instance().removeSubscription(instrument);
    }
}

} // namespace QuantLabs
