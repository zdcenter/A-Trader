#pragma once

#include "ThostFtdcMdApi.h"
#include "network/Publisher.h"
#include <memory>
#include <string>
#include <map>
#include <set>
#include <vector>

namespace atrad {

class MdHandler : public CThostFtdcMdSpi {
public:
    explicit MdHandler(Publisher& pub, std::map<std::string, std::string> config, std::set<std::string> contracts);
    virtual ~MdHandler();

    // 连接与登录
    void login();
    void subscribe(); // 订阅全部已存合约
    void subscribe(const std::string& instrument);
    void unsubscribe(const std::string& instrument);
    void join();

    // --- SPI 回调 ---
    void OnFrontConnected() override;
    void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) override;
    void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

private:
    CThostFtdcMdApi* md_api_ = nullptr;
    Publisher& pub_;
    
    std::string broker_id_;
    std::string user_id_;
    std::string password_;
    std::string md_front_;
    std::set<std::string> contracts_;
};

} // namespace atrad
