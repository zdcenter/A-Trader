#pragma once

#include "ThostFtdcTraderApi.h"
#include "protocol/message_schema.h"
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace atrad {

class Publisher;

class TraderHandler : public CThostFtdcTraderSpi {
public:
    explicit TraderHandler(std::map<std::string, std::string> config, Publisher& pub);
    virtual ~TraderHandler();

    void connect();
    void login();
    void auth();
    void confirmSettlement();
    void join();

    // 查询接口
    void qryAccount();
    void qryPosition();
    void qryInstrument(const std::string& instrument_id);
    void qryMarginRate(const std::string& instrument_id);
    void qryCommissionRate(const std::string& instrument_id);

    // 下单接口
    int insertOrder(const std::string& instrument, double price, int volume, char direction, char offset, const std::string& strategy_id = "");
    
    // 撤单 Action
    int cancelOrder(const std::string& instrument, const std::string& orderSysID, const std::string& orderRef, const std::string& exchangeID, int frontID, int sessionID);

    // --- SPI 回调 ---
    void OnFrontConnected() override;
    void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    
    // 资金与持仓查询回调
    void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

    // 报单成交回报
    void OnRtnOrder(CThostFtdcOrderField *pOrder) override;
    void OnRtnTrade(CThostFtdcTradeField *pTrade) override;
    void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo) override;
    void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    
    // 撤单回调
    void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo) override;


    // 队列查询接口
    void queueRateQuery(const std::string& instrumentID);

private:
    void queryLoop();
    void updateLocalPosition(CThostFtdcTradeField *pTrade);
    void updateLocalAccount(CThostFtdcTradeField *pTrade);

    CThostFtdcTraderApi* td_api_ = nullptr;
    Publisher& pub_;

    std::string broker_id_;
    std::string user_id_;
    std::string password_;
    std::string td_front_;
    std::string app_id_;
    std::string auth_code_;

    int front_id_ = 0;
    int session_id_ = 0;
    std::string current_trading_day_; // Added
    std::atomic<int> next_order_ref_{1};

    // 缓存 (恢复)
    std::map<std::string, InstrumentData> instrument_cache_;
    std::map<std::string, PositionData> position_cache_;
    AccountData account_cache_;

    // 查询队列 (线程安全)
    std::deque<std::string> high_priority_queue_; // Subscribed / Positions
    std::deque<std::string> low_priority_queue_;  // Others (if needed)
    std::unordered_set<std::string> queried_set_; // Avoid dup query in session
    std::mutex queue_mtx_;
    std::condition_variable queue_cv_;
    std::thread query_thread_;
    std::atomic<bool> running_{false};

    // Request ID mapping for async queries
    std::map<int, std::string> request_map_;
    std::atomic<int> next_req_id_{1000};
    std::mutex req_mtx_;
    
    // Order Ref to Strategy ID mapping
    std::map<std::string, std::string> order_strategy_map_;
    std::mutex order_strategy_mtx_;
    
    // Day Orders/Trades Cache
    std::vector<CThostFtdcOrderField> order_cache_;
    std::vector<CThostFtdcTradeField> trade_cache_;

public:
    // 推送所有缓存的持仓和资金
    void pushCachedPositions();
    
    // 推送缓存的所有合约信息（用于前端重连）
    void pushCachedInstruments();
    
    // 推送当日所有委托和成交（用于前端重连）
    void pushCachedOrdersAndTrades();

    void loadInstrumentsFromDB(); // Added
    void loadDayOrdersFromDB(); // Added
    void syncSubscribedInstruments(); // Added

    // Helper for Strategy
    bool getInstrumentData(const std::string& id, InstrumentData& out_data);
};

} // namespace atrad
