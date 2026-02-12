#include "position/PositionManager.h"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace atrader {
namespace core {

void PositionManager::UpdateInstrument(const CThostFtdcInstrumentField& instrument) {
    std::lock_guard<std::mutex> lock(m_mutex);
    // 更新合约元数据
    InstrumentMeta& meta = instruments_meta_[instrument.InstrumentID];
    std::strncpy(meta.instrument_id, instrument.InstrumentID, sizeof(meta.instrument_id) - 1);
    std::strncpy(meta.exchange_id, instrument.ExchangeID, sizeof(meta.exchange_id) - 1);
    meta.volume_multiple = instrument.VolumeMultiple;
    meta.price_tick = instrument.PriceTick;
    meta.position_date_type = instrument.PositionDateType; 

    // 初始化持仓对象 (如果不存在)
    if (!positions_.count(instrument.InstrumentID)) { 
        positions_[instrument.InstrumentID] = std::make_shared<InstrumentPosition>();
        positions_[instrument.InstrumentID]->InstrumentID = instrument.InstrumentID;
        positions_[instrument.InstrumentID]->ExchangeID = instrument.ExchangeID;
    }
}

void PositionManager::UpdateInstrumentMeta(const InstrumentMeta& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    InstrumentMeta& meta = instruments_meta_[data.instrument_id];
    // 全量更新元数据
    std::memcpy(&meta, &data, sizeof(InstrumentMeta));
}

std::shared_ptr<InstrumentPosition> PositionManager::GetPosition(const std::string& instrumentID) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = positions_.find(instrumentID);
    if (it != positions_.end()) {
        return it->second;
    }
    return nullptr;
}

std::unordered_map<std::string, std::shared_ptr<InstrumentPosition>>& PositionManager::GetAllPositions() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return positions_;
}

void PositionManager::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    positions_.clear();
}

/**
 * @brief 核心持仓更新逻辑 (基于简单的聚合结构)
 */
double PositionManager::UpdateFromTrade(const CThostFtdcTradeField& trade) {
    std::lock_guard<std::mutex> lock(m_mutex);
    double totalPnl = 0.0;

    // 1. 获取/创建持仓对象
    auto it = positions_.find(trade.InstrumentID);
    if (it == positions_.end()) {
        positions_[trade.InstrumentID] = std::make_shared<InstrumentPosition>();
        positions_[trade.InstrumentID]->InstrumentID = trade.InstrumentID;
        positions_[trade.InstrumentID]->ExchangeID = trade.ExchangeID;
    }
    auto pos = positions_[trade.InstrumentID];

    // 2. 获取合约规则 (SHFE/Others)
    bool isSHFE = false;
    double multiple = 1.0;
    
    auto itMeta = instruments_meta_.find(trade.InstrumentID);
    if (itMeta != instruments_meta_.end()) {
        // THOST_FTDC_PDT_UseHistory ('1') -> SHFE/INE Mode
        isSHFE = (itMeta->second.position_date_type == THOST_FTDC_PDT_UseHistory);
        multiple = itMeta->second.volume_multiple;
    } else {
        // Fallback: Check ExchangeID string
        // CTP ExchangeID is char[9]
        if (std::string(trade.ExchangeID) == "SHFE" || std::string(trade.ExchangeID) == "INE") {
            isSHFE = true;
        }
    }

    // 3. 更新逻辑
    // OffsetFlag: Open, Close, CloseToday, CloseYesterday
    // Direction: Buy(0), Sell(1)
    
    // 开仓?
    if (trade.OffsetFlag == THOST_FTDC_OF_Open) {
        if (trade.Direction == THOST_FTDC_D_Buy) {
            // 买开 -> 多头增加 (今仓)
            pos->LongPosition += trade.Volume;
            pos->LongTodayPosition += trade.Volume;
            // Cost Update: Cost += Price * Vol * Mult
            pos->LongPositionCost += trade.Price * trade.Volume * multiple;
            pos->LongOpenCost += trade.Price * trade.Volume * multiple;
            // 简单平均价可以根据 Cost / Pos 算出，无需单独存
        } else {
            // 卖开 -> 空头增加 (今仓)
            pos->ShortPosition += trade.Volume;
            pos->ShortTodayPosition += trade.Volume;
            pos->ShortPositionCost += trade.Price * trade.Volume * multiple;
            pos->ShortOpenCost += trade.Price * trade.Volume * multiple;
        }
    } 
    // 平仓? (Close, CloseToday, CloseYesterday)
    else {
        // 卖平 -> 减少多头
        if (trade.Direction == THOST_FTDC_D_Sell) {
             double originalVol = pos->LongPosition;
             pos->LongPosition -= trade.Volume;
             
             // 盈亏计算: (Price - OpenCostAvg) * Vol * Mult? 
             // 简单起见，这里只更新持仓量。盈亏通常由 QryInvestorPosition 返回或者单独计算。
             // 这里我们至少要更新成本 (按比例扣减)
             double costToDeduct = 0.0;
             double avgCost = 0.0;
             if (originalVol > 0) {
                 avgCost = pos->LongPositionCost / originalVol;
                 costToDeduct = avgCost * trade.Volume;
             }
             pos->LongPositionCost -= costToDeduct;
             
             // Realized PnL: (Sell Price - Avg Cost) * Volume * Multiplier
             // Note: costToDeduct = (Avg Price * Multiplier) * Volume
             double pnl = (trade.Price * trade.Volume * multiple) - costToDeduct;
             pos->LongCloseProfit += pnl;
             totalPnl += pnl;

             // 处理今昨
             if (isSHFE) {
                 if (trade.OffsetFlag == THOST_FTDC_OF_CloseToday) {
                     pos->LongTodayPosition -= trade.Volume;
                 } else {
                     // CloseYesterday or Close (on SHFE Close usually means CloseYd)
                     pos->LongYdPosition -= trade.Volume;
                 }
             } else {
                 // 非上期所：优先平昨
                 if (pos->LongYdPosition >= trade.Volume) {
                     pos->LongYdPosition -= trade.Volume;
                 } else {
                     // 昨仓不够，扣完昨仓扣今仓
                     int remain = trade.Volume - pos->LongYdPosition;
                     pos->LongYdPosition = 0;
                     pos->LongTodayPosition -= remain;
                 }
             }
        } 
        // 买平 -> 减少空头
        else {
             pos->ShortPosition -= trade.Volume;
             
             double originalVol = pos->ShortPosition + trade.Volume;
             double costToDeduct = 0.0;
             if (originalVol > 0) {
                 double avgCost = pos->ShortPositionCost / originalVol;
                 costToDeduct = avgCost * trade.Volume;
             }
             pos->ShortPositionCost -= costToDeduct;

             // Short Profit: (Entry Price - Exit/Buy Price) * Vol * Mult
             // costToDeduct = Entry Price * Vol * Mult
             // Exit Cost = trade.Price * Vol * Mult
             double pnl = costToDeduct - (trade.Price * trade.Volume * multiple);
             pos->ShortCloseProfit += pnl;
             totalPnl += pnl;
             
             if (isSHFE) {
                 if (trade.OffsetFlag == THOST_FTDC_OF_CloseToday) {
                     pos->ShortTodayPosition -= trade.Volume;
                 } else {
                     pos->ShortYdPosition -= trade.Volume;
                 }
             } else {
                 if (pos->ShortYdPosition >= trade.Volume) {
                     pos->ShortYdPosition -= trade.Volume;
                 } else {
                     int remain = trade.Volume - pos->ShortYdPosition;
                     pos->ShortYdPosition = 0;
                     pos->ShortTodayPosition -= remain;
                 }
             }
        }
    }
    return totalPnl;
}

} // namespace core
} // namespace atrader
