#include "api/TraderHandler.h"
#include "network/Publisher.h"
#include "protocol/message_schema.h"
#include <iostream>
#include <cstring>
#include <filesystem>
#include <thread>
#include <chrono>
#include "storage/DBManager.h"

namespace atrad {

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
    auth();
}

void TraderHandler::auth() {
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
        std::cout << "[Td] Auth Success. Logging in..." << std::endl;
        login();
    } else {
        std::cerr << "[Td] Auth Failed: " << (pRspInfo ? pRspInfo->ErrorMsg : "Unknown") << std::endl;
    }
}

void TraderHandler::login() {
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
        
        loadInstrumentsFromDB();
        syncSubscribedInstruments();

        confirmSettlement();
    } else {
        std::cerr << "[Td] Login Failed: " << (pRspInfo ? pRspInfo->ErrorMsg : "Unknown") << std::endl;
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
        std::cout << "[Td] Settlement Confirmed. Querying Account & Position..." << std::endl;
        qryAccount();
    }
}

void TraderHandler::qryAccount() {
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
        
        // 更新缓存
        account_cache_ = data;

        pub_.publishAccount(data);
    }
    if (bIsLast) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        qryPosition();
    }
}

// ... (qryPosition, OnRspQryInvestorPosition, qryInstrument 等保持不变) ...

void TraderHandler::pushCachedPositions() {
    std::cout << "[Td] Pushing cached state (Account + " << position_cache_.size() << " Positions)..." << std::endl;
    
    // 1. 推送资金快照
    // 简单的判断：如果 balance 为 0 可能还没查回来，就不推了，或者推了也没事
    pub_.publishAccount(account_cache_);

    // 2. 推送持仓快照
    for (const auto& [key, data] : position_cache_) {
        pub_.publishPosition(data);
        if (instrument_cache_.find(data.instrument_id) != instrument_cache_.end()) {
            pub_.publishInstrument(instrument_cache_[data.instrument_id]);
        }
    }
}

void TraderHandler::pushCachedInstruments() {
    if (instrument_cache_.empty()) return;
    std::cout << "[Td] Pushing cached instruments (" << instrument_cache_.size() << ")..." << std::endl;
    for (const auto& [id, data] : instrument_cache_) {
        pub_.publishInstrument(data);
    }
    for (const auto& [id, data] : instrument_cache_) {
        pub_.publishInstrument(data);
    }
}

bool TraderHandler::getInstrumentData(const std::string& id, InstrumentData& out_data) {
    if (instrument_cache_.find(id) != instrument_cache_.end()) {
        out_data = instrument_cache_[id];
        return true;
    }
    return false;
}

void TraderHandler::qryPosition() {
    CThostFtdcQryInvestorPositionField req;
    std::memset(&req, 0, sizeof(req));
    std::strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    std::strncpy(req.InvestorID, user_id_.c_str(), sizeof(req.InvestorID));
    td_api_->ReqQryInvestorPosition(&req, 0);
}

void TraderHandler::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pPos, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pPos && pPos->Position > 0) {
        PositionData data;
        std::strncpy(data.instrument_id, pPos->InstrumentID, sizeof(data.instrument_id));
        data.direction = pPos->PosiDirection;
        data.position = pPos->Position;
        data.today_position = pPos->TodayPosition;
        data.yd_position = pPos->YdPosition;
        data.position_cost = pPos->PositionCost;
        data.pos_profit = pPos->PositionProfit;
        
        // 更新缓存
        std::string key = std::string(data.instrument_id) + "_" + std::to_string(data.direction);
        position_cache_[key] = data;

        pub_.publishPosition(data);
        
        // 查出持仓后，开启合约信息查询链
        // 优化：将查询任务放入队列，避免此处直接调用导致流控或阻塞 CTP 线程
        // 由 queueRateQuery 内部检查缓存新鲜度，此处无条件调用以确保数据校验
        queueRateQuery(pPos->InstrumentID);
    }

    if (bIsLast) {
        std::cout << "[Td] Position Sync Completed. Total Cached: " << position_cache_.size() << std::endl;
    }
}


void TraderHandler::qryInstrument(const std::string& instrument_id) {
    CThostFtdcQryInstrumentField req;
    std::memset(&req, 0, sizeof(req));
    std::strncpy(req.InstrumentID, instrument_id.c_str(), sizeof(req.InstrumentID));
    td_api_->ReqQryInstrument(&req, 0);
}

void TraderHandler::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) {
        std::cerr << "[Td Error] QryInstrument Failed: " << pRspInfo->ErrorMsg << std::endl;
        return;
    }

    if (pInstrument) {
        // [Debug] 打印每一个收到的合约，看看 ru2605 是否出现以及属性是什么
        // std::cout << "[Td Debug] Recv Instrument: " << pInstrument->InstrumentID 
        //           << " Class:" << pInstrument->ProductClass << std::endl;

        // 只处理期货 (ProductClass == '1')
        // 增加日志：如果是因为类型不对被过滤，打印出来
        if (pInstrument->ProductClass != THOST_FTDC_PC_Futures) {
            //  std::cout << "[Td Warn] Filtered Instrument (Not Future): " << pInstrument->InstrumentID 
            //            << " Class: " << pInstrument->ProductClass << std::endl;
            // 恢复过滤：只保留期货，过滤掉期权(2)等其他类型
            return; 
        }

        InstrumentData& data = instrument_cache_[pInstrument->InstrumentID];
        std::strncpy(data.instrument_id, pInstrument->InstrumentID, sizeof(data.instrument_id));
        std::strncpy(data.exchange_id, pInstrument->ExchangeID, sizeof(data.exchange_id));
        
        // 记得加上 InstrumentName!
        std::strncpy(data.instrument_name, pInstrument->InstrumentName, sizeof(data.instrument_name));
        
        std::strncpy(data.product_id, pInstrument->ProductID, sizeof(data.product_id));
        std::strncpy(data.underlying_instr_id, pInstrument->UnderlyingInstrID, sizeof(data.underlying_instr_id));
        data.strike_price = pInstrument->StrikePrice;
        
        data.volume_multiple = pInstrument->VolumeMultiple;
        data.price_tick = pInstrument->PriceTick;

        // Mark Trading Day
        std::strncpy(data.trading_day, current_trading_day_.c_str(), sizeof(data.trading_day) - 1);
        
        std::cout << "[Td] Saved Instrument: " << data.instrument_id << " (" << data.instrument_name << ")" << std::endl;

        // 1. 保存到数据库 (全量保存)
        DBManager::instance().saveInstrument(data);

        // 2. 直接推送到前端 (不再需要白名单，因为我们只查关注的)
        pub_.publishInstrument(data);
    } else {
        // pInstrument is null
         std::cout << "[Td Warn] OnRspQryInstrument received NULL data." << std::endl;
    }
}

void TraderHandler::qryMarginRate(const std::string& instrument_id) {
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

        InstrumentData& data = instrument_cache_[pMargin->InstrumentID];
        data.long_margin_ratio_by_money = pMargin->LongMarginRatioByMoney;
        data.long_margin_ratio_by_volume = pMargin->LongMarginRatioByVolume;
        data.short_margin_ratio_by_money = pMargin->ShortMarginRatioByMoney;
        data.short_margin_ratio_by_volume = pMargin->ShortMarginRatioByVolume;
        
        // 收到 Margin，仅更新缓存，不急着推送，等 Comm 一起推
        // pub_.publishInstrument(data);
        
        std::strncpy(data.trading_day, current_trading_day_.c_str(), sizeof(data.trading_day) - 1);
        DBManager::instance().saveInstrument(data);
    } else {
        std::cerr << "[Td Error] Margin Resp is NULL or Error" << std::endl;
    }
}

void TraderHandler::qryCommissionRate(const std::string& instrument_id) {
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

        InstrumentData& data = instrument_cache_[target_id];
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

int TraderHandler::insertOrder(const std::string& instrument, double price, int volume, char direction, char offset, const std::string& strategy_id) {
    CThostFtdcInputOrderField req;
    std::memset(&req, 0, sizeof(req));
    std::strncpy(req.BrokerID, broker_id_.c_str(), sizeof(req.BrokerID));
    std::strncpy(req.InvestorID, user_id_.c_str(), sizeof(req.InvestorID));
    std::strncpy(req.InstrumentID, instrument.c_str(), sizeof(req.InstrumentID));
    std::string ref = std::to_string(next_order_ref_++);
    std::strncpy(req.OrderRef, ref.c_str(), sizeof(req.OrderRef));
    req.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
    req.Direction = direction; 
    req.CombOffsetFlag[0] = offset;
    req.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    req.LimitPrice = price;
    req.VolumeTotalOriginal = volume;
    req.TimeCondition = THOST_FTDC_TC_GFD;
    req.VolumeCondition = THOST_FTDC_VC_AV;
    req.ContingentCondition = THOST_FTDC_CC_Immediately;
    req.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
    req.IsAutoSuspend = 0;
    req.UserForceClose = 0;
    
    // 存储 OrderRef 到 strategy_id 的映射
    if (!strategy_id.empty()) {
        std::lock_guard<std::mutex> lock(order_strategy_mtx_);
        order_strategy_map_[ref] = strategy_id;
    }
    
    return td_api_->ReqOrderInsert(&req, 0);
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
        
        // 1. 推送给前端
        pub_.publishOrder(pOrder);
        
        // 2. 保存到数据库（带 strategy_id）
        DBManager::instance().saveOrder(pOrder, strategy_id);
    }
}

void TraderHandler::OnRtnTrade(CThostFtdcTradeField *pTrade) {
    if (pTrade) {
        std::cout << "[Td] Trade Update: " << pTrade->TradeID << " Price: " << pTrade->Price << std::endl;
        
        // 查找 strategy_id
        std::string strategy_id;
        {
            std::lock_guard<std::mutex> lock(order_strategy_mtx_);
            auto it = order_strategy_map_.find(pTrade->OrderRef);
            if (it != order_strategy_map_.end()) {
                strategy_id = it->second;
            }
        }
        
        // 1. 推送给前端
        pub_.publishTrade(pTrade);

        // 2. 保存到数据库（带 strategy_id）
        DBManager::instance().saveTrade(pTrade, strategy_id);
    }
    
    // 实时更新本地缓存
    updateLocalPosition(pTrade);
    updateLocalAccount(pTrade);
}

void TraderHandler::updateLocalPosition(CThostFtdcTradeField *pTrade) {
    std::string instrument = pTrade->InstrumentID;
    
    // 确定持仓方向：买开/卖平 -> 多头变化；卖开/买平 -> 空头变化
    // CTP 持仓方向: '2' Buy (多头), '3' Sell (空头)
    char pos_direction;
    if (pTrade->Direction == THOST_FTDC_D_Buy) { // 买
        if (pTrade->OffsetFlag == THOST_FTDC_OF_Open) pos_direction = THOST_FTDC_PD_Long; // 买开 -> 多头
        else pos_direction = THOST_FTDC_PD_Short; // 买平 -> 空头(减)
    } else { // 卖
        if (pTrade->OffsetFlag == THOST_FTDC_OF_Open) pos_direction = THOST_FTDC_PD_Short; // 卖开 -> 空头
        else pos_direction = THOST_FTDC_PD_Long; // 卖平 -> 多头(减)
    }

    std::string key = instrument + "_" + std::to_string(pos_direction);
    
    // 如果是开仓，可能需要新建
    if (position_cache_.find(key) == position_cache_.end()) {
        PositionData empty_pos;
        std::strncpy(empty_pos.instrument_id, instrument.c_str(), sizeof(empty_pos.instrument_id));
        empty_pos.direction = pos_direction;
        empty_pos.position = 0;
        empty_pos.today_position = 0;
        empty_pos.yd_position = 0;
        empty_pos.position_cost = 0;
        position_cache_[key] = empty_pos;
    }

    PositionData& pos = position_cache_[key];
    
    // 获取合约乘数
    int mult = 1;
    if (instrument_cache_.count(instrument)) mult = instrument_cache_[instrument].volume_multiple;

    if (pTrade->OffsetFlag == THOST_FTDC_OF_Open) {
        // 开仓：增加持仓，增加成本
        pos.position += pTrade->Volume;
        pos.today_position += pTrade->Volume;
        pos.position_cost += (pTrade->Price * pTrade->Volume * mult);
    } else {
        // 平仓：减少持仓，减少成本 (简单按比例减少，实际 CTP 算法更复杂)
        if (pos.position > 0) {
            double cost_release = (pos.position_cost / pos.position) * pTrade->Volume;
            pos.position -= pTrade->Volume;
            pos.position_cost -= cost_release;
            
            // 区分平今平和昨 (CTP Trade 不一定直接告诉你是今是昨，这里简单处理)
            if (pTrade->OffsetFlag == THOST_FTDC_OF_CloseToday) {
                pos.today_position -= pTrade->Volume;
            } else {
                // 优先平昨
                if (pos.yd_position >= pTrade->Volume) pos.yd_position -= pTrade->Volume;
                else pos.today_position -= (pTrade->Volume - pos.yd_position); // 剩下的平今
            }
        }
    }
    
    // 广播更新
    pub_.publishPosition(pos);
    std::cout << "[Td] Local Pos Updated: " << instrument << " Dir:" << pos.direction << " Vol:" << pos.position << std::endl;
}

void TraderHandler::updateLocalAccount(CThostFtdcTradeField *pTrade) {
    // 资金更新非常复杂(涉及保证金冻结释放、手续费扣除、平仓盈亏结算)，
    // 完全准确的推算很难。这里仅做“手续费扣除”和“保证金估算”的简单演示。
    // 生产环境建议：成交后还是触发一次 qryAccount 做校准，或者接受几秒的资金延迟。
    
    // 简单扣除手续费 (假设万分之1)
    double commission = pTrade->Price * pTrade->Volume * 10 * 0.0001; 
    account_cache_.commission += commission;
    account_cache_.balance -= commission;
    account_cache_.available -= commission;
    
    pub_.publishAccount(account_cache_);
}

void TraderHandler::OnRspOrderInsert(CThostFtdcInputOrderField *pInput, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
    if (pRspInfo && pRspInfo->ErrorID != 0) std::cerr << "[Td] Order Insert Error: " << pRspInfo->ErrorMsg << std::endl;
}

void TraderHandler::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInput, CThostFtdcRspInfoField *pRspInfo) {
    if (pRspInfo && pRspInfo->ErrorID != 0) std::cerr << "[Td] ErrRtn Insert: " << pRspInfo->ErrorMsg << std::endl;
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
             std::cout << "[Td] Cache Hit: " << instrumentID << " (Skip)" << std::endl;
             return; // 命中缓存，直接结束，不入队
         }
    }

    // 2. 缓存未命中，入队查询
    if (queried_set_.find(instrumentID) == queried_set_.end()) {
        std::cout << "[Td] Enqueue: " << instrumentID << std::endl;
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
             pub_.publishInstrument(i);
             count++;
        }
    }
    std::cout << "[Td] Loaded " << instrs.size() << " instruments from DB. Valid for Today:" << count << std::endl;
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
                     std::cout << "[Td] QueryLoop DoubleCheck Hit: " << iid << " (Drop)" << std::endl;
                     continue; // 直接跳过，不查
                }
            }

            std::cout << "[Td] QueryLoop Processing: " << iid << std::endl;
            
            // CTP 限流: 每秒 1 次请求安全
            
            // 1. 先查基础信息 (VolumeMultiple, ProductID 等)
            qryInstrument(iid);
            std::this_thread::sleep_for(std::chrono::milliseconds(1200));

            // 2. 查保证金率
            qryMarginRate(iid);
            std::this_thread::sleep_for(std::chrono::milliseconds(1200));
            
            // 3. 查手续费率
            qryCommissionRate(iid);
            std::this_thread::sleep_for(std::chrono::milliseconds(1200));
        }
    }
}

void TraderHandler::syncSubscribedInstruments() {
    auto subs = DBManager::instance().loadSubscriptions();
    int count = 0;
    for (const auto& iid : subs) {
        queueRateQuery(iid);
        count++;
    }
    std::cout << "[Td] Synced " << count << " subscribed instruments." << std::endl;
}

} // namespace atrad
