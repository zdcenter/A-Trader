#pragma once

#include "position/PositionManager.h"

#include "ThostFtdcTraderApi.h"
#include "protocol/message_schema.h"
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <filesystem>
#include <iostream>
#include <map>
#include <chrono>
#include <iomanip>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>

namespace QuantLabs {

class Publisher;

using atrader::core::PositionManager;

class TraderHandler : public CThostFtdcTraderSpi {
public:
    explicit TraderHandler(std::map<std::string, std::string> config, Publisher& pub);
    virtual ~TraderHandler();

    void connect();
    void reqUserLogin();
    void reqAuthenticate();
    void confirmSettlement();
    void join();
    
    // Core functionality utilizing PositionManager
    PositionManager& getPositionManager() { return m_posManager; }

    // 查询接口
    void reqQueryBrokerTradingParams();
    void reqQueryTradingAccount();
    void reqQueryPosition();

    void qryInstrument(const std::string& instrument_id);
    void qryAllInstruments(); // 全量查询合约
    void qryMarginRate(const std::string& instrument_id);
    void qryCommissionRate(const std::string& instrument_id);
    void qryPositionDetail(); // 查询持仓明细（用于FIFO匹配）

    // 下单接口
    int insertOrder(const std::string& instrument, double price, int volume, char direction, char offset, char priceType = '2', const std::string& strategy_id = "");
    
    // 撤单 Action
    int cancelOrder(const std::string& instrument, const std::string& orderSysID, const std::string& orderRef, const std::string& exchangeID, int frontID, int sessionID);

    // --- SPI 回调 ---
    void OnFrontConnected() override;
    void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
	///请求查询经纪公司交易参数响应
	virtual void OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField* pBrokerTradingParams, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

    // 资金与持仓查询回调
    void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;
    void OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) override;

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
    void updateLocalAccount(CThostFtdcTradeField *pTrade, double commission, double realized_pnl);
    
    // New: Position Manager
    PositionManager m_posManager;

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
    // Removed duplicates
    
    // Order Ref Management
    std::string last_ref_time_str_;
    std::atomic<int> order_ref_seq_{0};
    std::mutex ref_mtx_;
    
    // High performance cache
    long long cached_second_ = 0;       // 缓存的秒时间戳
    char cached_ref_prefix_[10] = {0};  // 缓存的 "DDHHMMSS" 字符串

    // 缓存
    std::map<std::string, InstrumentMeta> instrument_cache_;
    AccountData account_cache_;

    // 查询队列 (线程安全)
    std::deque<std::string> high_priority_queue_; // Subscribed / Positions
    std::deque<std::string> low_priority_queue_;  // Others (if needed)
    // 去重
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
    std::vector<TradeData> trade_cache_;

public:
    // 推送所有缓存的持仓和资金
    void pushCachedPositions();
    
    // 推送缓存的所有合约信息（用于前端重连）
    void pushCachedInstruments();
    
    // 推送当日所有委托和成交（用于前端重连）
    void pushCachedOrdersAndTrades();

    void loadInstrumentsFromDB();
    void loadDayOrdersFromDB();
    void syncSubscribedInstruments();

    // Helper for Strategy
    bool getInstrumentMeta(const std::string& id, InstrumentMeta& out_data);

private:
};

} // namespace QuantLabs
