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

std::unordered_map<std::string, std::shared_ptr<InstrumentPosition>> PositionManager::GetAllPositions() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return positions_;  // 返回快照副本，外部遍历时不再需要持锁
}

void PositionManager::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    positions_.clear();
}

/**
 * @brief 核心持仓更新逻辑 (FIFO 逐笔盈亏)
 * 
 * 开仓: 创建 OpenDetail 入队
 * 平仓: 从队头按 FIFO 逐笔扣减，用每笔实际开仓价计算盈亏
 *        SHFE/INE 平今/平昨会精确匹配 is_today 标记
 * 
 * @return 本次平仓的实现盈亏 (开仓返回 0)
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

    // 2. 获取合约规则
    bool isSHFE = false;
    double multiple = 1.0;
    
    auto itMeta = instruments_meta_.find(trade.InstrumentID);
    if (itMeta != instruments_meta_.end()) {
        // 使用历史持仓
        // #define THOST_FTDC_PDT_UseHistory '1'
        // #define THOST_FTDC_PDT_NoUseHistory '2'
        isSHFE = (itMeta->second.position_date_type == THOST_FTDC_PDT_UseHistory);
        multiple = itMeta->second.volume_multiple;
    } else {
        if (std::string(trade.ExchangeID) == "SHFE" || std::string(trade.ExchangeID) == "INE") {
            isSHFE = true;
        }
    }

    // 判断当前成交是否今仓：比较开仓日期与交易日
    bool isToday = trading_day_.empty() || (std::string(trade.TradeDate) == trading_day_);

    // 3. 开仓逻辑
    if (trade.OffsetFlag == THOST_FTDC_OF_Open) {
        OpenDetail detail(trade.Price, trade.Volume, trade.TradeDate, isToday);
        
        if (trade.Direction == THOST_FTDC_D_Buy) {
            // 买开 → 多头增加
            pos->LongPosition += trade.Volume;
            if (isToday) {
                pos->LongTodayPosition += trade.Volume;
            } else {
                pos->LongYdPosition += trade.Volume;
            }
            pos->LongPositionCost += trade.Price * trade.Volume * multiple;
            pos->LongOpenCost += trade.Price * trade.Volume * multiple;
            pos->LongDetails.push_back(detail);
        } else {
            // 卖开 → 空头增加
            pos->ShortPosition += trade.Volume;
            if (isToday) {
                pos->ShortTodayPosition += trade.Volume;
            } else {
                pos->ShortYdPosition += trade.Volume;
            }
            pos->ShortPositionCost += trade.Price * trade.Volume * multiple;
            pos->ShortOpenCost += trade.Price * trade.Volume * multiple;
            pos->ShortDetails.push_back(detail);
        }
    }
    // 4. 平仓逻辑 (FIFO)
    else {
        double closePrice = trade.Price;
        int remainToClose = trade.Volume;

        if (trade.Direction == THOST_FTDC_D_Sell) {
            // 卖平 → 减少多头
            pos->LongPosition -= trade.Volume;
            
            // 确定要平的是今仓还是昨仓
            bool closeToday = (trade.OffsetFlag == THOST_FTDC_OF_CloseToday);
            bool closeYd = (trade.OffsetFlag == THOST_FTDC_OF_CloseYesterday);
            
            // FIFO 扣减：从 LongDetails 队列头部逐笔匹配
            auto& details = pos->LongDetails;
            auto it = details.begin();
            while (it != details.end() && remainToClose > 0) {
                // SHFE 需要精确匹配今/昨
                if (isSHFE) {
                    if (closeToday && !it->is_today) { ++it; continue; }
                    if (closeYd && it->is_today) { ++it; continue; }
                }
                
                int matchVol = std::min(remainToClose, it->volume);
                
                // 逐笔盈亏: (平仓价 - 开仓价) × 手数 × 乘数
                double pnl = (closePrice - it->price) * matchVol * multiple;
                totalPnl += pnl;
                
                // 扣减成本
                double costReduced = it->price * matchVol * multiple;
                pos->LongPositionCost -= costReduced;
                
                it->volume -= matchVol;
                remainToClose -= matchVol;
                
                if (it->volume <= 0) {
                    it = details.erase(it);
                } else {
                    ++it;
                }
            }
            
            pos->LongCloseProfit += totalPnl;
            
            // 更新今昨仓计数
            if (isSHFE) {
                if (closeToday) {
                    pos->LongTodayPosition -= trade.Volume;
                } else {
                    pos->LongYdPosition -= trade.Volume;
                }
            } else {
                // 非SHFE：优先平昨
                if (pos->LongYdPosition >= trade.Volume) {
                    pos->LongYdPosition -= trade.Volume;
                } else {
                    int remain = trade.Volume - pos->LongYdPosition;
                    pos->LongYdPosition = 0;
                    pos->LongTodayPosition -= remain;
                }
            }
        }
        else {
            // 买平 → 减少空头
            pos->ShortPosition -= trade.Volume;
            
            bool closeToday = (trade.OffsetFlag == THOST_FTDC_OF_CloseToday);
            bool closeYd = (trade.OffsetFlag == THOST_FTDC_OF_CloseYesterday);
            
            auto& details = pos->ShortDetails;
            auto it = details.begin();
            while (it != details.end() && remainToClose > 0) {
                if (isSHFE) {
                    if (closeToday && !it->is_today) { ++it; continue; }
                    if (closeYd && it->is_today) { ++it; continue; }
                }
                
                int matchVol = std::min(remainToClose, it->volume);
                
                // 空头盈亏: (开仓价 - 平仓价) × 手数 × 乘数
                double pnl = (it->price - closePrice) * matchVol * multiple;
                totalPnl += pnl;
                
                double costReduced = it->price * matchVol * multiple;
                pos->ShortPositionCost -= costReduced;
                
                it->volume -= matchVol;
                remainToClose -= matchVol;
                
                if (it->volume <= 0) {
                    it = details.erase(it);
                } else {
                    ++it;
                }
            }
            
            pos->ShortCloseProfit += totalPnl;
            
            if (isSHFE) {
                if (closeToday) {
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
