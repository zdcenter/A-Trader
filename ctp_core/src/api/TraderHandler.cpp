#include "api/TraderHandler.h"
#include "network/Publisher.h"
#include "protocol/message_schema.h"
#include <iostream>
#include <cstring>
#include <filesystem>
#include <thread>
#include <chrono>
#include "storage/DBManager.h"
#include "utils/Encoding.h"

namespace QuantLabs {

TraderHandler::TraderHandler(std::map<std::string, std::string> config, Publisher& pub) : pub_(pub) {
    broker_id_ = config["broker_id"];
    user_id_ = config["user_id"];
    password_ = config["password"];
    td_front_ = config["td_front"];
    app_id_ = config["app_id"];
    auth_code_ = config["auth_code"];

    std::string flow_dir = "./flow/td/" + broker_id_ + "/" + user_id_ + "/";
    if (!std::filesystem::exists(flow_dir)) {
        std::filesystem::create_directories(flow_dir);
    }

    td_api_ = CThostFtdcTraderApi::CreateFtdcTraderApi(flow_dir.c_str());
    td_api_->RegisterSpi(this);
    td_api_->SubscribePublicTopic(THOST_TERT_QUICK);
    td_api_->SubscribePrivateTopic(THOST_TERT_QUICK);
    td_api_->RegisterFront(const_cast<char*>(td_front_.c_str()));
    
    std::cout << "[Td] Initializing Trader API..." << std::endl;
    td_api_->Init();

    // 启动查询队列线程
    running_ = true;
    query_thread_ = std::thread(&TraderHandler::queryLoop, this);
}

TraderHandler::~TraderHandler() {
    running_ = false;
    queue_cv_.notify_all();
    if (query_thread_.joinable()) query_thread_.join();

    if (td_api_) {
        td_api_->RegisterSpi(nullptr);
        td_api_->Release();
        td_api_ = nullptr;
    }
}

void TraderHandler::join() {
    if (td_api_) td_api_->Join();
}

void TraderHandler::OnFrontConnected() {
    std::cout << "[Td] Connected. Authenticating..." << std::endl;
    reqAuthenticate();
}

void TraderHandler::reqAuthenticate() {
    CThostFtdcReqAuthenticateField req;
    std::memset(&req, 0, sizeof(req));
    std::strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    std::strncpy(req.UserID, user_id_.c_str(), sizeof(req.UserID));
    std::strncpy(req.AppID, app_id_.c_str(), sizeof(req.AppID));
    std::strncpy(req.AuthCode, auth_code_.c_str(), sizeof(req.AuthCode));
    td_api_->ReqAuthenticate(&req, 0);
}

void TraderHandler::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pAuth, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID == 0) {
        std::cout << "[Td] Authenticate Success. Requesting User Login..." << std::endl;
        reqUserLogin();
    } else {
        std::cerr << "[Td] Authenticate Failed: " << (pRspInfo ? utils::gbk_to_utf8(pRspInfo->ErrorMsg) : "Unknown") << std::endl;
    }
}

void TraderHandler::reqUserLogin() {
    CThostFtdcReqUserLoginField req;
    std::memset(&req, 0, sizeof(req));
    std::strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    std::strncpy(req.UserID, user_id_.c_str(), sizeof(req.UserID));
    std::strncpy(req.Password, password_.c_str(), sizeof(req.Password));
    td_api_->ReqUserLogin(&req, 0);
}

void TraderHandler::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID == 0) {
        front_id_ = pRspUserLogin->FrontID;
        session_id_ = pRspUserLogin->SessionID;
        current_trading_day_ = pRspUserLogin->TradingDay;
        std::cout << "[Td] Login Success. Day:" << current_trading_day_ << " Confirming..." << std::endl;
        
        // Load global settings (Current Strategy)
        std::string saved_strategy = DBManager::instance().getSetting("current_strategy_id");
        if (!saved_strategy.empty()) {
             std::lock_guard<std::mutex> lock(order_strategy_mtx_);
             current_strategy_id_ = saved_strategy;
             std::cout << "[Td] Loaded Current Strategy: " << current_strategy_id_ << std::endl;
        }

        m_posManager.SetTradingDay(current_trading_day_);
        loadInstrumentsFromDB();
        syncSubscribedInstruments();

        // [数据恢复] 加载当日的报单和成交记录
        {
            auto orders = DBManager::instance().loadOrders(current_trading_day_);
            std::cout << "[Td] Restored " << orders.size() << " orders from DB." << std::endl;
            // 逐个推送给前端
            for (const auto& o : orders) pub_.publishOrder(&o);

            auto trades = DBManager::instance().loadTrades(current_trading_day_);
            std::cout << "[Td] Restored " << trades.size() << " trades from DB." << std::endl;
            for (const auto& t : trades) pub_.publishTrade(t);
        }

        confirmSettlement();
    } else {
        std::cerr << "[Td] Login Failed: " << (pRspInfo ? utils::gbk_to_utf8(pRspInfo->ErrorMsg) : "Unknown") << std::endl;
    }
}

void TraderHandler::confirmSettlement() {
    CThostFtdcSettlementInfoConfirmField req;
    std::memset(&req, 0, sizeof(req));
    std::strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    std::strncpy(req.InvestorID, user_id_.c_str(), sizeof(req.InvestorID));
    td_api_->ReqSettlementInfoConfirm(&req, 0);
}

void TraderHandler::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID == 0) {
        // 统计缓存中当天有效的合约数量
        int today_count = 0;
        for (const auto& [id, meta] : instrument_cache_) {
            if (std::string(meta.trading_day) == current_trading_day_) {
                today_count++;
            }
        }

        // 如果当天缓存足够（说明今天已经做过全量查询），直接跳过
        if (today_count >= 100) {
            std::cout << "[Td] Settlement Confirmed. Today's instrument cache valid (" 
                      << today_count << "). Skipping full query. Querying BrokerParams..." << std::endl;
            reqQueryBrokerTradingParams();
        } else {
            // 新交易日或首次启动，需要全量查询
            std::cout << "[Td] Settlement Confirmed. Cache outdated (today:" << today_count 
                      << "). Waiting 1s then querying ALL instruments..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1100));
            qryAllInstruments();
        }
    }
}

void TraderHandler::qryAllInstruments() {
    CThostFtdcQryInstrumentField req;
    std::memset(&req, 0, sizeof(req));
    // Empty InstrumentID means ALL
    int ret = td_api_->ReqQryInstrument(&req, 0);
    std::cout << "[Td] ReqQryInstrument ret=" << ret 
              << " (0=OK, -1=Network, -2=Pending, -3=FlowControl)" << std::endl;
}

void TraderHandler::qryInstrument(const std::string& instrument_id) {
    CThostFtdcQryInstrumentField req;
    std::memset(&req, 0, sizeof(req));
    std::strncpy(req.InstrumentID, instrument_id.c_str(), sizeof(req.InstrumentID));
    td_api_->ReqQryInstrument(&req, 0);
}

void TraderHandler::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        std::cerr << "[Td Error] QryInstrument Failed: " << utils::gbk_to_utf8(pRspInfo->ErrorMsg) << std::endl;
        return;
    }

    if (pInstrument) {
        // 只处理期货 (ProductClass == '1')
        // 注意：不能用 return 过滤！否则最后一条如果是期权，bIsLast 检查会被跳过
        if (pInstrument->ProductClass == THOST_FTDC_PC_Futures) {

            InstrumentMeta& data = instrument_cache_[pInstrument->InstrumentID];
            std::strncpy(data.instrument_id, pInstrument->InstrumentID, sizeof(data.instrument_id));
            std::strncpy(data.exchange_id, pInstrument->ExchangeID, sizeof(data.exchange_id));
            std::strncpy(data.instrument_name, pInstrument->InstrumentName, sizeof(data.instrument_name));
            std::strncpy(data.product_id, pInstrument->ProductID, sizeof(data.product_id));
            std::strncpy(data.underlying_instr_id, pInstrument->UnderlyingInstrID, sizeof(data.underlying_instr_id));
            data.strike_price = pInstrument->StrikePrice;
            data.volume_multiple = pInstrument->VolumeMultiple;
            data.price_tick = pInstrument->PriceTick;
            std::strncpy(data.trading_day, current_trading_day_.c_str(), sizeof(data.trading_day) - 1);

#ifdef _DEBUG        
            std::cout << "[Td] Saved Instrument: " << data.instrument_id << " (" << data.instrument_name << ")" << std::endl;
#endif  
            m_posManager.UpdateInstrument(*pInstrument);
            DBManager::instance().saveInstrument(data);
            pub_.publishInstrument(data);
        }
        // 非期货合约：静默跳过，不 return，让 bIsLast 正常检查
    }

    // Chain execution: when ALL instruments are loaded, query Account
    if (bIsLast) {
        std::cout << "[Td] All Instruments Loaded (" << instrument_cache_.size() << "). Querying Account..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Brief pause
        reqQueryBrokerTradingParams();
    }
}


void TraderHandler::reqQueryBrokerTradingParams()
{
	CThostFtdcQryBrokerTradingParamsField req = { 0 };
    std::memset(&req, 0, sizeof(req));
    std::strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    std::strncpy(req.InvestorID, user_id_.c_str(), sizeof(req.InvestorID));
    std::strncpy(req.CurrencyID, "CNY", sizeof(req.CurrencyID));

	td_api_->ReqQryBrokerTradingParams(&req, 0);

}

void TraderHandler::OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField* pBrokerTradingParams, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast)
{
	if (pBrokerTradingParams)
	{
		// simnow MarginPriceType = 4 开仓价算保证金

#ifdef _DEBUG
		std::cout << "[Td] BrokerID:" << pBrokerTradingParams->BrokerID << std::endl
			<< "保证金价格类型(MarginPriceType):" << pBrokerTradingParams->MarginPriceType << std::endl
			<< "盈亏算法(Algorithm):" << pBrokerTradingParams->Algorithm << std::endl
			<< "可用是否包含平仓盈利(AvailIncludeCloseProfit):" << pBrokerTradingParams->AvailIncludeCloseProfit << std::endl
			<< "币种代码(CurrencyID):" << pBrokerTradingParams->CurrencyID << std::endl
			<< "期权权利金价格类型(OptionRoyaltyPriceType):" << pBrokerTradingParams->OptionRoyaltyPriceType
			<< std::endl;
#endif

	}
	if (bIsLast)
	{
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
		reqQueryTradingAccount();
	}
}

void TraderHandler::reqQueryTradingAccount() {
    CThostFtdcQryTradingAccountField req;
    std::memset(&req, 0, sizeof(req));
    std::strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    std::strncpy(req.InvestorID, user_id_.c_str(), sizeof(req.InvestorID));
    td_api_->ReqQryTradingAccount(&req, 0);
}

void TraderHandler::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pAccount) {
        AccountData data;
        data.balance = pAccount->Balance;
        data.available = pAccount->Available;
        data.margin = pAccount->CurrMargin;
        data.frozen_margin = pAccount->FrozenMargin;
        data.commission = pAccount->Commission;
        data.close_profit = pAccount->CloseProfit;
        
        // 更新缓存
        account_cache_ = data;

        pub_.publishAccount(data);
    }
    if (bIsLast) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        reqQueryPosition();
    }
}


void TraderHandler::reqQueryPosition() {
    CThostFtdcQryInvestorPositionField req;
    std::memset(&req, 0, sizeof(req));
    std::strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    std::strncpy(req.InvestorID, user_id_.c_str(), sizeof(req.InvestorID));
    td_api_->ReqQryInvestorPosition(&req, 0);
}

void TraderHandler::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pPos, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pPos && pPos->Position > 0) {
        // [修正] 汇总查询不再更新持仓缓存，完全依赖持仓明细 (PositionDetail)
        // 仅利用汇总查询的结果来触发费率查询，确保我们要交易的合约费率已加载
        queueRateQuery(pPos->InstrumentID);
    }

    if (bIsLast) {
        std::cout << "[Td] Position Query Completed. Total Cached: " << m_posManager.GetAllPositions().size() << std::endl;
        std::cout << "[Td] Waiting for PositionDetail query to recalculate accurate positions..." << std::endl;
        
        // 不在这里推送！等待 PositionDetail 查询完成后，基于明细重新计算并推送
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        qryPositionDetail();
    }
}


void TraderHandler::qryPositionDetail() {
    // Clear old state before reloading (Avoid doubling positions on retry)
    m_posManager.Clear();
    
    CThostFtdcQryInvestorPositionDetailField req;
    std::memset(&req, 0, sizeof(req));
    std::strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    std::strncpy(req.InvestorID, user_id_.c_str(), sizeof(req.InvestorID));
    std::cout << "[Td] Querying Investor Position Detail for Manager..." << std::endl;
    td_api_->ReqQryInvestorPositionDetail(&req, 0);
}

void TraderHandler::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField *pDetail, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        std::cerr << "[Td Error] Qry Position Detail Failed: " << utils::gbk_to_utf8(pRspInfo->ErrorMsg) << std::endl;
        return;
    }

    if (pDetail) {

#ifdef _DEBUG
		std::cerr << "[Td] 合约代码：" << pDetail->InstrumentID << std::endl;
		std::cerr << "[Td] 经纪公司代码(BrokerID)：\t" << pDetail->BrokerID << std::endl;
		std::cerr << "[Td] 投资者代码(InvestorID)：\t" << pDetail->InvestorID << std::endl;
		std::cerr << "[Td] 投机套保标记(HedgeFlag)：\t" << pDetail->HedgeFlag << std::endl;
		std::cerr << "[Td] 买卖(Direction)：\t" << pDetail->Direction << std::endl;
		std::cerr << "[Td] 成交类型(TradeType)：\t" << pDetail->TradeType << std::endl;
		std::cerr << "[Td] 开仓日期(OpenDate)：\t" << pDetail->OpenDate << std::endl;
		std::cerr << "[Td] 交易日(TradingDay)：\t" << pDetail->TradingDay << std::endl;
		std::cerr << "[Td] 成交编号(TradeID)：\t" << pDetail->TradeID << std::endl;
		std::cerr << "[Td] 数量(Volume)：\t" << pDetail->Volume << std::endl;
		std::cerr << "[Td] 平仓量(CloseVolume)：\t" << pDetail->CloseVolume << std::endl;
		std::cerr << "[Td] 开仓价(OpenPrice)：\t" << pDetail->OpenPrice << std::endl;
		std::cerr << "[Td] 逐日盯市平仓盈亏(CloseProfitByDate)：\t" << pDetail->CloseProfitByDate << std::endl;
		std::cerr << "[Td] 逐笔对冲平仓盈亏(CloseProfitByTrade)：\t" << pDetail->CloseProfitByTrade << std::endl;
		std::cerr << "[Td] 逐日盯市持仓盈亏(PositionProfitByDate)：\t" << pDetail->PositionProfitByDate << std::endl;
		std::cerr << "[Td] 逐笔对冲持仓盈亏(PositionProfitByTrade：\t" << pDetail->PositionProfitByTrade << std::endl;
		std::cerr << "[Td] 投资者保证金(Margin)：\t" << pDetail->Margin << std::endl;
		std::cerr << "[Td] 交易所保证金(ExchMargin)：\t" << pDetail->ExchMargin << std::endl;
		std::cerr << "[Td] 保证金率(MarginRateByMoney)：\t" << pDetail->MarginRateByMoney << std::endl;
		std::cerr << "[Td] 昨结算价(LastSettlementPrice)：\t" << pDetail->LastSettlementPrice << std::endl;
		std::cerr << "[Td] 结算价(SettlementPrice)：\t" << pDetail->SettlementPrice << std::endl;

#endif // _DEBUG
        // 过滤已平仓明细 (CTP 有时会推送 Volume=0 的记录)
        if (pDetail->Volume > 0) {
            
            // --- PositionManager Integration ---
            // Construct a synthetic TradeField to feed into PositionManager
            CThostFtdcTradeField trade = {0};
            // 合约ID
            std::strncpy(trade.InstrumentID, pDetail->InstrumentID, sizeof(trade.InstrumentID));
            // 交易所ID
            std::strncpy(trade.ExchangeID, pDetail->ExchangeID, sizeof(trade.ExchangeID));
            // 成交ID
            std::strncpy(trade.TradeID, pDetail->TradeID, sizeof(trade.TradeID));
            // 成交日期
            std::strncpy(trade.TradeDate, pDetail->OpenDate, sizeof(trade.TradeDate));
            // 开仓价
            trade.Price = pDetail->OpenPrice;
            // 持仓量
            trade.Volume = pDetail->Volume;
            
            // Map Direction
            // CTP Detail uses '0'(Buy)/'1'(Sell)
            if (pDetail->Direction == THOST_FTDC_D_Buy) { //'0'
                 trade.Direction = THOST_FTDC_D_Buy; //'0'
            } else {
                 trade.Direction = THOST_FTDC_D_Sell; //'1'
            }
            trade.OffsetFlag = THOST_FTDC_OF_Open; 
            
            // Push to Manager (FIFO 队列会自动根据 trading_day_ 判断今/昨仓)
            m_posManager.UpdateFromTrade(trade);
            // -----------------------------------
        }
    }

    if (bIsLast) {
        std::cout << "[Td] Position Detail Query Completed. Manager synced. Current Positions:" << std::endl;
        
        for (const auto& [id, pos] : m_posManager.GetAllPositions()) {
            if (pos->LongPosition > 0) 
                 std::cout << "  - [L] " << id << ": " << pos->LongPosition << " (Td:" << pos->LongTodayPosition << ", Yd:" << pos->LongYdPosition << ") Cost:" << pos->LongPositionCost << std::endl;
            if (pos->ShortPosition > 0)
                 std::cout << "  - [S] " << id << ": " << pos->ShortPosition << " (Td:" << pos->ShortTodayPosition << ", Yd:" << pos->ShortYdPosition << ") Cost:" << pos->ShortPositionCost << std::endl;
        }

        // 不需要再重新计算了，PositionManager 已经在实时计算了
        // 可以选择在这里推送一次全量快照
        pushCachedPositions();
    }
}

void TraderHandler::pushCachedPositions() {
    // std::lock_guard<std::mutex> lock(position_mtx_); // REMOVED
    // 获取 PositionManager 的全量持仓
    auto positions = m_posManager.GetAllPositions();
    
    std::cout << "[Td] Pushing cached state (Account + " << positions.size() << " Instruments)..." << std::endl;
    
    // 1. 推送资金快照
    pub_.publishAccount(account_cache_);

    // 生成本次快照的批次号
    int64_t seq = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // 2. 推送持仓快照（需要扁平化）
    // PositionManager 的结构是 All-in-One (Long/Short Combined)
    // 我们需要把它们拆成两条记录 (Long, Short) 推送给前端
    
    for (const auto& [instID, posPtr] : positions) {
        if (!posPtr) continue;
        
        // 查找合约乘数
        int mult = 1;
        if (instrument_cache_.count(instID)) {
            mult = instrument_cache_[instID].volume_multiple;
            if (mult <= 0) mult = 1;
        }

        // --- 多头 ---
        if (posPtr->LongPosition > 0 || posPtr->LongFrozenMargin > 0) {
            PositionData data = {0};
            std::strncpy(data.instrument_id, posPtr->InstrumentID.c_str(), sizeof(data.instrument_id) - 1);
            std::strncpy(data.exchange_id, posPtr->ExchangeID.c_str(), sizeof(data.exchange_id) - 1);
            data.direction = THOST_FTDC_PD_Long; // '2'
            data.position = posPtr->LongPosition;
            data.today_position = posPtr->LongTodayPosition;
            data.yd_position = posPtr->LongYdPosition;
            data.position_cost = posPtr->LongPositionCost; 
            data.open_cost = posPtr->LongOpenCost;
            data.pos_profit = posPtr->LongPositionProfit;
            data.close_profit = posPtr->LongCloseProfit;
            data.margin = posPtr->Margin;  // TODO: 分多空保证金
            data.volume_multiple = mult;

            pub_.publishPosition(data, seq);
        }
        
        // --- 空头 ---
        if (posPtr->ShortPosition > 0 || posPtr->ShortFrozenMargin > 0) {
            PositionData data = {0};
            std::strncpy(data.instrument_id, posPtr->InstrumentID.c_str(), sizeof(data.instrument_id) - 1);
            std::strncpy(data.exchange_id, posPtr->ExchangeID.c_str(), sizeof(data.exchange_id) - 1);
            data.direction = THOST_FTDC_PD_Short; // '3'
            data.position = posPtr->ShortPosition;
            data.today_position = posPtr->ShortTodayPosition;
            data.yd_position = posPtr->ShortYdPosition;
            data.position_cost = posPtr->ShortPositionCost;
            data.open_cost = posPtr->ShortOpenCost;
            data.pos_profit = posPtr->ShortPositionProfit;
            data.close_profit = posPtr->ShortCloseProfit;
            data.margin = posPtr->Margin;  // TODO: 分多空保证金
            data.volume_multiple = mult;

            pub_.publishPosition(data, seq);
        }

        // 顺便推一下合约信息，防止前端只有持仓没有名字
        if (instrument_cache_.count(instID)) {
             pub_.publishInstrument(instrument_cache_[instID]);
        }
    }
}

void TraderHandler::pushCachedOrdersAndTrades() {
    if (current_trading_day_.empty()) return;
    
    // 从 DB 加载当日委托和成交
    auto orders = DBManager::instance().loadOrders(current_trading_day_);
    if (!orders.empty()) {
        std::cout << "[Td] SyncState: Pushing " << orders.size() << " restored orders..." << std::endl;
        for (const auto& o : orders) pub_.publishOrder(&o);
    }

    auto trades = DBManager::instance().loadTrades(current_trading_day_);
    if (!trades.empty()) {
        std::cout << "[Td] SyncState: Pushing " << trades.size() << " restored trades..." << std::endl;
        for (const auto& t : trades) pub_.publishTrade(t);
    }
}

void TraderHandler::pushCachedInstruments() {
    if (instrument_cache_.empty()) return;
    std::cout << "[Td] Pushing cached instruments (" << instrument_cache_.size() << ")..." << std::endl;
    for (const auto& [id, data] : instrument_cache_) {
        pub_.publishInstrument(data);
    }
}

bool TraderHandler::getInstrumentMeta(const std::string& id, InstrumentMeta& out_data) {
    if (instrument_cache_.find(id) != instrument_cache_.end()) {
        out_data = instrument_cache_[id];
        return true;
    }
    return false;
}


void TraderHandler::qryMarginRate(const std::string& instrument_id) {
    if (instrument_id.empty()) {
        std::cerr << "[Td Error] qryMarginRate failed: InstrumentID REQUIRED." << std::endl;
        return; 
    }
    CThostFtdcQryInstrumentMarginRateField req;
    std::memset(&req, 0, sizeof(req));
    std::strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    std::strncpy(req.InvestorID, user_id_.c_str(), sizeof(req.InvestorID));
    std::strncpy(req.InstrumentID, instrument_id.c_str(), sizeof(req.InstrumentID));
    req.HedgeFlag = THOST_FTDC_HF_Speculation;
    td_api_->ReqQryInstrumentMarginRate(&req, 0);
}

void TraderHandler::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pMargin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pMargin) {
        std::cout << "[Td Debug] Margin Resp: " << pMargin->InstrumentID 
                  << " LongMoney:" << pMargin->LongMarginRatioByMoney << std::endl;

        InstrumentMeta& data = instrument_cache_[pMargin->InstrumentID];
        data.long_margin_ratio_by_money = pMargin->LongMarginRatioByMoney;
        data.long_margin_ratio_by_volume = pMargin->LongMarginRatioByVolume;
        data.short_margin_ratio_by_money = pMargin->ShortMarginRatioByMoney;
        data.short_margin_ratio_by_volume = pMargin->ShortMarginRatioByVolume;
        
        // 收到 Margin，仅更新缓存，不急着推送，等 Comm 一起推
        
        std::strncpy(data.trading_day, current_trading_day_.c_str(), sizeof(data.trading_day) - 1);
        std::strncpy(data.trading_day, current_trading_day_.c_str(), sizeof(data.trading_day) - 1);
        DBManager::instance().saveInstrument(data);
        
        // --- Sync to PositionManager ---
        m_posManager.UpdateInstrumentMeta(data);
    } else {
        std::cerr << "[Td Error] Margin Resp is NULL or Error" << std::endl;
    }
}

void TraderHandler::qryCommissionRate(const std::string& instrument_id) {
    if (instrument_id.empty()) {
        std::cerr << "[Td Error] qryCommissionRate failed: InstrumentID REQUIRED." << std::endl;
        return; 
    }
    CThostFtdcQryInstrumentCommissionRateField req;
    std::memset(&req, 0, sizeof(req));
    std::strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    std::strncpy(req.InvestorID, user_id_.c_str(), sizeof(req.InvestorID));
    std::strncpy(req.InstrumentID, instrument_id.c_str(), sizeof(req.InstrumentID));
    
    int req_id = next_req_id_++;
    {
        std::lock_guard<std::mutex> lock(req_mtx_);
        request_map_[req_id] = instrument_id;
    }
    
    td_api_->ReqQryInstrumentCommissionRate(&req, req_id);
}

void TraderHandler::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pComm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    std::string requested_iid;
    {
        std::lock_guard<std::mutex> lock(req_mtx_);
        if (request_map_.find(nRequestID) != request_map_.end()) {
            requested_iid = request_map_[nRequestID];
            // Don't erase yet if bIsLast is false, but usually QryCommission is 1-shot.
            if (bIsLast) request_map_.erase(nRequestID);
        }
    }

    if (pComm) {
        // 关键修复：CTP 有时会返回品种代码 (如 "rb") 而不是合约代码 ("rb2605")。
        // 我们必须优先使用我们请求时记录的具体合约代码 ("rb2605") 来更新缓存，
        // 否则前端找不到 rb2605 的费率数据。
        std::string target_id = requested_iid;
        
        // 如果没办法匹配到请求ID，才回退到使用返回的ID
        if (target_id.empty()) target_id = pComm->InstrumentID;

        if (target_id.empty()) {
             std::cerr << "[Td Error] Unknown InstrumentID for Commission Rate" << std::endl;
             return;
        }

        // 严防死守：如果最终 ID 还是空的，绝对不处理
        if (target_id.empty() || target_id == "") return;

        std::cout << "[Td Debug] Comm Resp: " << pComm->InstrumentID 
                  << " (Mapped to: " << target_id << ")"
                  << " OpenMoney:" << pComm->OpenRatioByMoney << std::endl;

        InstrumentMeta& data = instrument_cache_[target_id];
        // 如果 target_id 和 requested_iid 不一致（或者是纯新增），确保 instrument_id 字段也被正确填充
        if (std::strlen(data.instrument_id) == 0) {
             std::strncpy(data.instrument_id, target_id.c_str(), sizeof(data.instrument_id) - 1);
        }

        data.open_ratio_by_money = pComm->OpenRatioByMoney;
        data.open_ratio_by_volume = pComm->OpenRatioByVolume;
        data.close_ratio_by_money = pComm->CloseRatioByMoney;
        data.close_ratio_by_volume = pComm->CloseRatioByVolume;
        data.close_today_ratio_by_money = pComm->CloseTodayRatioByMoney;
        data.close_today_ratio_by_volume = pComm->CloseTodayRatioByVolume;
        
        // 收到 Comm，推送一次
        std::strncpy(data.trading_day, current_trading_day_.c_str(), sizeof(data.trading_day) - 1);
        DBManager::instance().saveInstrument(data);
        
        // --- Sync to PositionManager ---
        m_posManager.UpdateInstrumentMeta(data);

        pub_.publishInstrument(data);

        // [补救措施] 如果收到了费率，但发现本地连名字都没有，说明基础查询可能丢了，尝试补查一次
        if (std::strlen(data.instrument_name) == 0) {
             static std::unordered_set<std::string> retry_set;
             if (retry_set.find(target_id) == retry_set.end()) {
                 std::cout << "[Td] Retrying QryInstrument for incomplete: " << target_id << std::endl;
                 retry_set.insert(target_id);
                 qryInstrument(target_id); // 立即补发一次
             }
        }
    } else {
        std::cerr << "[Td Error] Comm Resp is NULL or Error" << std::endl;
    }
}


int TraderHandler::insertOrder(const std::string& instrument, double price, int volume, char direction, char offset, char priceType, const std::string& strategy_id) {
    if (!td_api_) return -1;

    // Use Current Strategy if not specified
    std::string actual_strategy_id = strategy_id;
    if (actual_strategy_id.empty()) {
        std::lock_guard<std::mutex> lock(order_strategy_mtx_);
        actual_strategy_id = current_strategy_id_;
    }
    
    CThostFtdcInputOrderField order = {0};
    
    std::strncpy(order.BrokerID, broker_id_.c_str(), sizeof(order.BrokerID));
    std::strncpy(order.InvestorID, user_id_.c_str(), sizeof(order.InvestorID));
    std::strncpy(order.InstrumentID, instrument.c_str(), sizeof(order.InstrumentID));
    
    // 极致性能高频 OrderRef: DDHHMMSS(Cache) + uuu(Realtime) -> 11位
    // 零堆内存分配，秒级缓存避免 localtime_r 开销
    char ref[13];
    {
        std::lock_guard<std::mutex> lock(ref_mtx_);
        
        // 1. 获取当前毫秒时间戳
        auto now = std::chrono::system_clock::now();
        auto ms_part = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
        long long total_ms = ms_part.count();
        long long current_sec = total_ms / 1000;
        int current_ms = total_ms % 1000;
        
        // 2. 只有秒数变了才重新计算 DDHHMMSS (耗时的部分)
        if (current_sec != cached_second_) {
            std::time_t t = current_sec;
            struct tm tm;
            localtime_r(&t, &tm);
            // 格式化前8位: DDHHMMSS 到缓存
            std::sprintf(cached_ref_prefix_, "%02d%02d%02d%02d", 
                tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            cached_second_ = current_sec;
        }

        // 3. 处理极高频 (1ms内多单) 的唯一性
        static int last_ms_check = -1;
        static int ms_offset = 0;
        
        if (current_ms == last_ms_check) {
            ms_offset++; // 毫秒内碰撞，逻辑增加
        } else {
            last_ms_check = current_ms;
            ms_offset = 0;
        }
        
        // 加上偏移量，保证单调递增
        int final_ms = current_ms + ms_offset;
        
        // 极端兜底: CTP Ref 长度限制，必须保持3位。
        if (final_ms > 999) final_ms = 999; 

        // 4. 极速拼接: Prefix(8) + MS(3) -> Ref(11)
        std::memcpy(ref, cached_ref_prefix_, 8);
        
        // 快速 int 转 3位 char
        ref[8] = '0' + (final_ms / 100);
        ref[9] = '0' + ((final_ms / 10) % 10);
        ref[10] = '0' + (final_ms % 10);
        ref[11] = '\0';
    }
    
    std::strncpy(order.OrderRef, ref, sizeof(order.OrderRef));
    // Auto-adjust OffsetFlag for SHFE/INE (Smart Close)
    char finalOffset = offset;
    
    // 1. Get Exchange ID
    std::string exchId;
    if (instrument_cache_.count(instrument)) {
        exchId = instrument_cache_[instrument].exchange_id;
    }

    if (exchId == "SHFE" || exchId == "INE") {
        // Retrieve position from Manager
        auto posPtr = m_posManager.GetPosition(instrument);
        if (posPtr) {
            // Determine which direction we are closing
            // If Order is Buy (0), we are closing Short position.
            // If Order is Sell (1), we are closing Long position.
            bool closingLong = (direction == THOST_FTDC_D_Sell);
            
            int ydPos = closingLong ? posPtr->LongYdPosition : posPtr->ShortYdPosition;
            int todayPos = closingLong ? posPtr->LongTodayPosition : posPtr->ShortTodayPosition;
            
            // Logic:
            // If User sends Close (usually implies CloseYd on SHFE), but Yd is 0 and Today > 0 -> Switch to CloseToday
            if (offset == THOST_FTDC_OF_Close && ydPos <= 0 && todayPos > 0) {
                 std::cout << "[Td] SmartClose: Auto-Switch to CloseToday for " << instrument << std::endl;
                 finalOffset = THOST_FTDC_OF_CloseToday;
            }
            // If User sends CloseToday, but Today is 0 and Yd > 0 -> Switch to Close (Yd)
            else if (offset == THOST_FTDC_OF_CloseToday && todayPos <= 0 && ydPos > 0) {
                 std::cout << "[Td] SmartClose: Auto-Switch to CloseYesterday for " << instrument << std::endl;
                 finalOffset = THOST_FTDC_OF_Close; // On SHFE, Close usually means CloseYd
            }
        }
    }

    order.OrderPriceType = priceType;
    order.Direction = direction; 
    order.CombOffsetFlag[0] = finalOffset;
    order.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    
    if (priceType == THOST_FTDC_OPT_AnyPrice) {
        // [FIX] SHFE/INE rejects AnyPrice. Use LimitPrice with actual price + IOC for market behavior.
        // Frontend passes lastPrice, which is a reasonable base for immediate execution.
        order.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
        order.LimitPrice = price; // Use the price passed from frontend (lastPrice)
        order.TimeCondition = THOST_FTDC_TC_IOC; 
        order.VolumeCondition = THOST_FTDC_VC_AV;
        
        std::cout << "[Td] Simulating Market Order (IOC): " << instrument 
                  << " Dir:" << direction << " Price:" << order.LimitPrice << std::endl;
    } else {
        order.LimitPrice = price;
        order.TimeCondition = THOST_FTDC_TC_GFD;
        order.VolumeCondition = THOST_FTDC_VC_AV;
    }

    order.VolumeTotalOriginal = volume;
    order.ContingentCondition = THOST_FTDC_CC_Immediately;
    order.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    order.IsAutoSuspend = 0;
    order.UserForceClose = 0;
    
    // 记录 OrderRef -> StrategyID 映射
    {
        std::lock_guard<std::mutex> lock(order_strategy_mtx_);
        order_strategy_map_[ref] = actual_strategy_id;
    }

    int ret = td_api_->ReqOrderInsert(&order, next_req_id_++);
    if (ret != 0) {
        std::cerr << "[Td Error] ReqOrderInsert Failed: " << ret << std::endl;
    }
    return ret;
}

int TraderHandler::cancelOrder(const std::string& instrument, const std::string& orderSysID, const std::string& orderRef, const std::string& exchangeID, int frontID, int sessionID) {
    CThostFtdcInputOrderActionField req;
    std::memset(&req, 0, sizeof(req));
    
    std::strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    std::strncpy(req.InvestorID, user_id_.c_str(), sizeof(req.InvestorID));
    
    // ActionFlag: Delete
    req.ActionFlag = THOST_FTDC_AF_Delete;
    
    // 自动补全 ExchangeID
    std::string finalExchangeID = exchangeID;
    if (finalExchangeID.empty() && !instrument.empty()) {
        if (instrument_cache_.count(instrument)) {
            finalExchangeID = instrument_cache_[instrument].exchange_id;
            std::cout << "[Td] Auto-filled ExchangeID for Cancel: " << finalExchangeID << std::endl;
        }
    }
    
    // 如果有 OrderSysID，优先使用 (ExchangeID + OrderSysID)
    if (!orderSysID.empty()) {
        std::string finalSysID = orderSysID;
        
        // [FIX] SHFE/INE/CFFEX/CZCE usually require 12-digit right-aligned SysID with spaces
        // If we have a short ID (like "8657"), we must pad it to 12 chars: "        8657"
        // Also added DCE (Dalian Commodity Exchange) as it often requires padding or exact match of spaces.
        if (finalExchangeID == "SHFE" || finalExchangeID == "INE" || finalExchangeID == "CFFEX" || finalExchangeID == "CZCE" || finalExchangeID == "DCE") {
            if (finalSysID.length() > 0 && finalSysID.length() < 12) {
                std::stringstream ss;
                ss << std::setw(12) << finalSysID; 
                finalSysID = ss.str();
                std::cout << "[Td] Padded SysID for " << finalExchangeID << ": '" << finalSysID << "'" << std::endl;
            }
        }
        
        std::strncpy(req.ExchangeID, finalExchangeID.c_str(), sizeof(req.ExchangeID));
        std::strncpy(req.OrderSysID, finalSysID.c_str(), sizeof(req.OrderSysID));
        std::strncpy(req.InstrumentID, instrument.c_str(), sizeof(req.InstrumentID));
        
        // Also fill Front/Session/Ref just in case some brokers need them as redundant info
        if (frontID > 0 && sessionID > 0) {
             req.FrontID = frontID;
             req.SessionID = sessionID;
             if (!orderRef.empty()) std::strncpy(req.OrderRef, orderRef.c_str(), sizeof(req.OrderRef));
        }
    } else {
        // 否则使用 FrontID + SessionID + OrderRef + InstrumentID
        req.FrontID = frontID;
        req.SessionID = sessionID;
        std::strncpy(req.OrderRef, orderRef.c_str(), sizeof(req.OrderRef));
        std::strncpy(req.InstrumentID, instrument.c_str(), sizeof(req.InstrumentID));
    }
    
    std::cout << "[Td] ReqOrderAction: Inst:" << instrument 
              << " SysID:" << req.OrderSysID 
              << " Ref:" << req.OrderRef 
              << " Exch:" << req.ExchangeID 
              << " Front:" << req.FrontID 
              << " Session:" << req.SessionID << std::endl;
              
    return td_api_->ReqOrderAction(&req, next_req_id_++);
}

void TraderHandler::setCurrentStrategy(const std::string& strategy_id) {
    if (strategy_id.empty()) return;
    {
        std::lock_guard<std::mutex> lock(order_strategy_mtx_);
        current_strategy_id_ = strategy_id;
    }
    DBManager::instance().setSetting("current_strategy_id", strategy_id);
    std::cout << "[Td] Set Current Strategy => " << strategy_id << std::endl;
}

std::string TraderHandler::getCurrentStrategy() const {
    // If called from main thread while worker updates, we might see partial string (rare on modern arch for short string SSO).
    // Safe enough for UI display.
    return current_strategy_id_;
}

void TraderHandler::OnRtnOrder(CThostFtdcOrderField *pOrder) {
    if (pOrder) {
        std::cout << "[Td] Order Update: " << pOrder->OrderSysID << " Status: " << pOrder->OrderStatus << std::endl;
        
        // 查找 strategy_id
        std::string strategy_id;
        {
            std::lock_guard<std::mutex> lock(order_strategy_mtx_);
            auto it = order_strategy_map_.find(pOrder->OrderRef);
            if (it != order_strategy_map_.end()) {
                strategy_id = it->second;
            }
        }
        
        // [关键修复] 如果 CTP 回报没带日期，强制补全当前交易日，确保 DB 查询能过滤出来
        if (std::strlen(pOrder->InsertDate) == 0 && !current_trading_day_.empty()) {
             std::strncpy(pOrder->InsertDate, current_trading_day_.c_str(), sizeof(pOrder->InsertDate) - 1);
        }

        // 1. 推送给前端
        pub_.publishOrder(pOrder);
        
        // 2. 保存到数据库（带 strategy_id）
        DBManager::instance().saveOrder(pOrder, strategy_id, current_trading_day_);
    }
}

void TraderHandler::OnRtnTrade(CThostFtdcTradeField *pTrade) {
    if (pTrade) {
        std::cout << "[Td] Trade Update: " << pTrade->TradeID << " Price: " << pTrade->Price << std::endl;
        // --- PositionManager Integration ---
        double realized_pnl = m_posManager.UpdateFromTrade(*pTrade);
        // -----------------------------------

        // 查找 strategy_id
        std::string strategy_id;
        {
            std::lock_guard<std::mutex> lock(order_strategy_mtx_);
            auto it = order_strategy_map_.find(pTrade->OrderRef);
            if (it != order_strategy_map_.end()) {
                strategy_id = it->second;
            }
        }
        
        // 计算手续费和平仓盈亏
        double commission = 0.0;
        double close_profit = realized_pnl;
        
        if (instrument_cache_.count(pTrade->InstrumentID)) {
            const auto& instr = instrument_cache_[pTrade->InstrumentID];
            double price = pTrade->Price;
            int vol = pTrade->Volume;
            int mult = instr.volume_multiple > 0 ? instr.volume_multiple : 1;
            
            // 1. Commission
            double r_money = 0.0;
            double r_vol = 0.0;
            
            if (pTrade->OffsetFlag == THOST_FTDC_OF_Open) {
                r_money = instr.open_ratio_by_money;
                r_vol = instr.open_ratio_by_volume;
            } else if (pTrade->OffsetFlag == THOST_FTDC_OF_CloseToday) {
                r_money = instr.close_today_ratio_by_money;
                r_vol = instr.close_today_ratio_by_volume;
            } else {
                r_money = instr.close_ratio_by_money;
                r_vol = instr.close_ratio_by_volume;
            }
            commission = (price * vol * mult * r_money) + (vol * r_vol);
            
            // 2. Close Profit (FIFO Logic) - Removed Legacy Code
            // PositionManager now handles position state. Only P&L calculation logic removed.


        }

        // 1. 推送给前端
        pub_.publishTrade(pTrade, commission, close_profit);

        // [Debug Log] 打印成交更新后的持仓
        auto pos = m_posManager.GetPosition(pTrade->InstrumentID);
        if (pos) {
             std::cout << "[Td] Post-Trade Position: " << pos->InstrumentID 
                       << " L:" << pos->LongPosition << "(Td:" << pos->LongTodayPosition << ", Yd:" << pos->LongYdPosition << ")"
                       << " S:" << pos->ShortPosition << "(Td:" << pos->ShortTodayPosition << ", Yd:" << pos->ShortYdPosition << ")" 
                       << " PnL:" << realized_pnl << std::endl;
        }

        // 2. 保存到数据库（带 strategy_id, commission, close_profit, trading_day）
        DBManager::instance().saveTrade(pTrade, strategy_id, commission, close_profit, current_trading_day_);
        // 3. 更新缓存 (用于重连同步)
        bool exists = false;
        for (const auto& t : trade_cache_) {
            // TradeID + Direction + OrderSysID usually unique enough, or just TradeID if Exchange is unique
            if (std::strcmp(t.trade_id, pTrade->TradeID) == 0 && std::strcmp(t.exchange_id, pTrade->ExchangeID) == 0) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            TradeData td;
            std::memset(&td, 0, sizeof(td));
            std::strncpy(td.instrument_id, pTrade->InstrumentID, sizeof(td.instrument_id)-1);
            std::strncpy(td.trade_id, pTrade->TradeID, sizeof(td.trade_id)-1);
            std::strncpy(td.order_sys_id, pTrade->OrderSysID, sizeof(td.order_sys_id)-1);
            td.direction = pTrade->Direction;
            td.offset_flag = pTrade->OffsetFlag;
            td.price = pTrade->Price;
            td.volume = pTrade->Volume;
            std::strncpy(td.trade_time, pTrade->TradeTime, sizeof(td.trade_time)-1);
            std::strncpy(td.trade_date, pTrade->TradeDate, sizeof(td.trade_date)-1);
            std::strncpy(td.exchange_id, pTrade->ExchangeID, sizeof(td.exchange_id)-1);
            
            td.commission = commission;
            td.close_profit = close_profit;
            std::strncpy(td.strategy_id, strategy_id.c_str(), sizeof(td.strategy_id)-1);

            trade_cache_.push_back(td);
        }
        
        // 4. 更新本地持仓和资金
        // [FIX] Duplicate Call removed here to prevent double-counting.
        // It is called at the end of function (Lines 880+) for ALL trades.
        
        // 实时更新本地缓存
        updateLocalPosition(pTrade);
        updateLocalAccount(pTrade, commission, close_profit);
    }
}

// 实时更新本地缓存 (Replaced by PositionManager)
void TraderHandler::updateLocalPosition(CThostFtdcTradeField *pTrade) {
    auto posPtr = m_posManager.GetPosition(pTrade->InstrumentID);
    if (!posPtr) return;

    // 查合约乘数
    int mult = 1;
    if (instrument_cache_.count(pTrade->InstrumentID)) {
        mult = instrument_cache_[pTrade->InstrumentID].volume_multiple;
        if (mult <= 0) mult = 1;
    }

    // 判断受影响的方向
    bool checkLong = false;
    bool checkShort = false;

    if (pTrade->Direction == THOST_FTDC_D_Buy) {
        if (pTrade->OffsetFlag == THOST_FTDC_OF_Open) checkLong = true;
        else checkShort = true;
    } else {
        if (pTrade->OffsetFlag == THOST_FTDC_OF_Open) checkShort = true;
        else checkLong = true;
    }

    if (checkLong) {
        PositionData data = {0};
        std::strncpy(data.instrument_id, posPtr->InstrumentID.c_str(), sizeof(data.instrument_id) - 1);
        std::strncpy(data.exchange_id, posPtr->ExchangeID.c_str(), sizeof(data.exchange_id) - 1);
        data.direction = THOST_FTDC_PD_Long; 
        data.position = posPtr->LongPosition;
        data.today_position = posPtr->LongTodayPosition;
        data.yd_position = posPtr->LongYdPosition;
        data.position_cost = posPtr->LongPositionCost;
        data.open_cost = posPtr->LongOpenCost;
        data.pos_profit = posPtr->LongPositionProfit;
        data.close_profit = posPtr->LongCloseProfit;
        data.margin = posPtr->Margin;
        data.volume_multiple = mult;
        
        pub_.publishPosition(data);
    }

    if (checkShort) {
        PositionData data = {0};
        std::strncpy(data.instrument_id, posPtr->InstrumentID.c_str(), sizeof(data.instrument_id) - 1);
        std::strncpy(data.exchange_id, posPtr->ExchangeID.c_str(), sizeof(data.exchange_id) - 1);
        data.direction = THOST_FTDC_PD_Short;
        data.position = posPtr->ShortPosition;
        data.today_position = posPtr->ShortTodayPosition;
        data.yd_position = posPtr->ShortYdPosition;
        data.position_cost = posPtr->ShortPositionCost;
        data.open_cost = posPtr->ShortOpenCost;
        data.pos_profit = posPtr->ShortPositionProfit;
        data.close_profit = posPtr->ShortCloseProfit;
        data.margin = posPtr->Margin;
        data.volume_multiple = mult;
        
        pub_.publishPosition(data);
    }
}

void TraderHandler::updateLocalAccount(CThostFtdcTradeField *pTrade, double commission, double realized_pnl) {
    // 使用实际计算的手续费和平仓盈亏更新本地资金缓存
    // 注意：这是近似值，精确值需要通过 reqQueryTradingAccount 校准
    account_cache_.commission += commission;
    account_cache_.close_profit += realized_pnl;
    account_cache_.balance += realized_pnl - commission;
    account_cache_.available += realized_pnl - commission;
    
    pub_.publishAccount(account_cache_);
}

void TraderHandler::OnRspOrderInsert(CThostFtdcInputOrderField *pInput, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) std::cerr << "[Td] Order Insert Error: " << utils::gbk_to_utf8(pRspInfo->ErrorMsg) << std::endl;
}

void TraderHandler::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInput, CThostFtdcRspInfoField *pRspInfo) {
    if (pRspInfo && pRspInfo->ErrorID != 0) std::cerr << "[Td] ErrRtn Insert: " << utils::gbk_to_utf8(pRspInfo->ErrorMsg) << std::endl;
}

// 撤单报错
void TraderHandler::OnRspOrderAction(CThostFtdcInputOrderActionField *pInput, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) std::cerr << "[Td] RspOrderAction Error: " << utils::gbk_to_utf8(pRspInfo->ErrorMsg) << " (" << pRspInfo->ErrorID << ")" << std::endl;
}

void TraderHandler::OnErrRtnOrderAction(CThostFtdcOrderActionField *pAction, CThostFtdcRspInfoField *pRspInfo) {
    if (pRspInfo && pRspInfo->ErrorID != 0) std::cerr << "[Td] ErrRtn Action: " << utils::gbk_to_utf8(pRspInfo->ErrorMsg) << " (" << pRspInfo->ErrorID << ")" << std::endl;
}


/**
 * @brief 将合约加入费率查询队列 (线程安全)
 * 
 * 功能说明:
 * 1. 白名单管理: 自动将该合约加入 instrument_whitelist_，确保后续 OnRspQryInstrument 收到该合约信息时
 *    会推送给前端（因为我们只关心订阅或持有持仓的合约）。
 * 2. 队列管理: 将合约ID放入高优先级队列 high_priority_queue_。
 * 3. 去重: 使用 queried_set_ 确保同一个 Session 内不重复查询同一合约的费率，避免浪费请求资源。
 * 4. 唤醒: 通知后台 queryLoop 线程开始工作。
 */
void TraderHandler::queueRateQuery(const std::string& instrumentID) {
    if (instrumentID.empty()) return;
    
    std::unique_lock<std::mutex> lock(queue_mtx_); 
    
    // 1. 检查缓存
    if (instrument_cache_.count(instrumentID)) {
         auto& d = instrument_cache_[instrumentID];
         bool fresh = (std::string(d.trading_day) == current_trading_day_);
         bool hasData = (d.price_tick > 0);

         // [Debug]
         // std::cout << "[Debug Cache] " << instrumentID << " Fresh:" << fresh << " HasData:" << hasData << std::endl;
         
         if (fresh && hasData) {
             pub_.publishInstrument(d);
             // std::cout << "[Td] Cache Hit: " << instrumentID << " (Skip)" << std::endl;
             return; // 命中缓存，直接结束，不入队
         }
    }

    // 2. 缓存未命中，入队查询
    if (queried_set_.find(instrumentID) == queried_set_.end()) {
        // std::cout << "[Td] Enqueue: " << instrumentID << std::endl;
        high_priority_queue_.push_back(instrumentID);
        queried_set_.insert(instrumentID);
        queue_cv_.notify_one();
    }
}



void TraderHandler::loadInstrumentsFromDB() {
    auto instrs = DBManager::instance().loadAllInstruments();
    int count = 0;
    for (const auto& i : instrs) {
        instrument_cache_[i.instrument_id] = i;
        if (std::string(i.trading_day) == current_trading_day_) {
             count++;
        }
    }
    std::cout << "[Td] Loaded " << instrs.size() << " instruments from DB. Valid for Today:" << count << std::endl;
    
    // --- PositionManager Init from Cache ---
    for (const auto& i : instrs) {
         CThostFtdcInstrumentField instr = {0};
         std::strncpy(instr.InstrumentID, i.instrument_id, sizeof(instr.InstrumentID));
         std::strncpy(instr.ExchangeID, i.exchange_id, sizeof(instr.ExchangeID));
         instr.VolumeMultiple = i.volume_multiple;
         instr.PriceTick = i.price_tick;
         // Assume cache is good enough for PositionDateType or infer it
         // Ideally DB should store PositionDateType but we don't have it yet in struct
         // Can infer from ExchangeID
         if (std::string(i.exchange_id) == "SHFE" || std::string(i.exchange_id) == "INE") {
             instr.PositionDateType = THOST_FTDC_PDT_UseHistory; 
         } else {
             instr.PositionDateType = THOST_FTDC_PDT_NoUseHistory;
         }
         
         m_posManager.UpdateInstrument(instr);
    }
    // ---------------------------------------
}

void TraderHandler::loadDayOrdersFromDB() {
    auto day_orders = DBManager::instance().loadOrders(current_trading_day_);
    {
        std::lock_guard<std::mutex> lock(ref_mtx_);
        order_cache_ = day_orders; 
    }
    pushCachedOrdersAndTrades();
}


/**
 * @brief 费率查询工作线程循环
 * 
 * 功能说明:
 * 1. 负责从队列中取出合约ID。
 * 2. 执行 CTP 费率查询 (保证金率 & 手续费率)。
 * 3. 核心作用: 【流控 (Rate Limiting)】。
 *    CTP 对查询请求（特别是费率查询）有严格的每秒请求数限制（通常 1次/秒）。
 *    如果并发大量查询（如启动时查几十个订阅合约），CTP 前置机可能会直接断开连接或报错。
 *    因此，这里必须串行执行，并在每次查询之间强制 sleep 1.1秒以上。
 */
void TraderHandler::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        std::cerr << "[Td Error] OnRspError: " << pRspInfo->ErrorMsg 
                  << " (ID:" << pRspInfo->ErrorID << ")" << std::endl;
        if (pRspInfo->ErrorID == 90) {
            std::cerr << "!!! CTP RATE LIMIT REACHED (流控) !!!" << std::endl;
        }
    }
}
void TraderHandler::queryLoop() {
    // [重要优化] 启动时先睡 5 秒，避开主线程查资金和持仓的高峰期，防止 ru2605 等首个合约被流控
    std::cout << "[Td] QueryLoop started. Waiting 5s for startup API calm down..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    while (running_) {
        std::string iid;
        {
            std::unique_lock<std::mutex> lock(queue_mtx_);
            // 等待直到队列不为空 或 线程停止
            queue_cv_.wait(lock, [this]() {
                return !running_ || !high_priority_queue_.empty() || !low_priority_queue_.empty();
            });

            if (!running_) break;

            if (!high_priority_queue_.empty()) {
                iid = high_priority_queue_.front();
                high_priority_queue_.pop_front();
            } else if (!low_priority_queue_.empty()) {
                iid = low_priority_queue_.front();
                low_priority_queue_.pop_front();
            }
        }

        if (!iid.empty()) {
            // [终极防护] 从队列取出后，执行前再检查一次缓存！
            // 防止在 DB 加载前就有请求入队，导致 DB 加载后依然执行了陈旧的查询请求
            if (instrument_cache_.count(iid)) {
                auto& d = instrument_cache_[iid];
                if (std::string(d.trading_day) == current_trading_day_ && d.price_tick > 0) {
                     // std::cout << "[Td] QueryLoop DoubleCheck Hit: " << iid << " (Drop)" << std::endl;
                     continue; // 直接跳过，不查
                }
            }

            std::cout << "[Td] QueryLoop Processing: " << iid << std::endl;
            
            // CTP 限流: 每秒 1 次请求安全
            
            // 1. 基础信息已在启动时全量查询，此处不再重复查询以免浪费流控
            // qryInstrument(iid); 
            // std::this_thread::sleep_for(std::chrono::milliseconds(1200));

            // 2. 查保证金率
            qryMarginRate(iid);
            std::this_thread::sleep_for(std::chrono::milliseconds(1200));
            
            // 3. 查手续费率
            qryCommissionRate(iid);
            std::this_thread::sleep_for(std::chrono::milliseconds(1200));
        }
    }
}

// -------------------------------------------------------------

void TraderHandler::syncSubscribedInstruments() {
    auto subs = DBManager::instance().loadSubscriptions();
    int count = 0;
    for (const auto& iid : subs) {
        queueRateQuery(iid);
        count++;
    }
    std::cout << "[Td] Synced " << count << " subscribed instruments." << std::endl;
}

} // namespace QuantLabs
