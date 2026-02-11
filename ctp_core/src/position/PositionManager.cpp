
#include "../../include/position/PositionManager.h"
#include <iostream>
#include <cstring>
#include <algorithm>

namespace atrader {
namespace core {

/**
 * 辅助函数: 判断是今日还是昨日 (简单的字符串比较)
 * 实际应用中可以传入 CurrentTradingDay 进行比较
 */
static bool IsYesterday(const char* tradeDate, const char* tradingDay) {
    if (!tradeDate || !tradingDay) return false;
    return std::strcmp(tradeDate, tradingDay) != 0;
}

// === 公共接口 实现 ===

void PositionManager::UpdateInstrument(const CThostFtdcInstrumentField& instrument) {
    std::lock_guard<std::mutex> lock(m_mtx);
    // 判断是否存在，不存在则新建
    auto& pos = positions_[instrument.InstrumentID];
    if (!pos) {
        pos = std::make_shared<ContractPosition>();
        pos->instrumentID = instrument.InstrumentID;
        pos->exchangeID = instrument.ExchangeID;
    }
    pos->multiplier = instrument.VolumeMultiple;
    pos->priceTick = instrument.PriceTick;
    // 默认 preSettlementPrice = lastPrice if not known, or ideally query from MarketData
}

std::shared_ptr<ContractPosition> PositionManager::GetPosition(const std::string& instrumentID) {
    std::lock_guard<std::mutex> lock(m_mtx);
    auto it = positions_.find(instrumentID);
    if (it != positions_.end()) {
        return it->second;
    }
    return nullptr;
}

std::unordered_map<std::string, std::shared_ptr<ContractPosition>>& PositionManager::GetAllPositions() {
    return positions_;
}

void PositionManager::UpdateFromTrade(const CThostFtdcTradeField& trade) {
    std::lock_guard<std::mutex> lock(m_mtx);

    // 1. 获取持仓对象
    auto& pos = positions_[trade.InstrumentID];
    if (!pos) {
        pos = std::make_shared<ContractPosition>();
        pos->instrumentID = trade.InstrumentID;
        pos->exchangeID = trade.ExchangeID;
        // 乘数默认为1，如果在 UpdateInstrument 获取不到的话
        pos->multiplier = 1;
    }

    /*
     * 逻辑分流:
     * Open -> handleOpen
     * Close / CloseToday / CloseYesterday -> handleClose
     */
    if (trade.OffsetFlag == THOST_FTDC_OF_Open) {
        if (trade.Direction == THOST_FTDC_D_Buy) {
            handleOpen(pos, trade, true); // 买开 -> 多头增加
        } else {
            handleOpen(pos, trade, false); // 卖开 -> 空头增加
        }
    } else {
        // Close
        if (trade.Direction == THOST_FTDC_D_Sell) {
            handleClose(pos, trade, true); // 卖平 -> 多头减少
        } else {
            handleClose(pos, trade, false); // 买平 -> 空头减少
        }
    }
}

// === 核心逻辑: 处理开仓(增加明细) ===
void PositionManager::handleOpen(std::shared_ptr<ContractPosition> pos, const CThostFtdcTradeField& trade, bool isLong) {
    DirectionPosition& dirPos = isLong ? pos->longPos : pos->shortPos;

    // 1. 创建新明细节点
    // 默认新开仓一定是 "Today" (current date)
    // 除非这是个历史恢复操作，但在 Trade 回报里通常都是 Today
    TradeDetailNode node(
        trade.TradeID,
        trade.TradeDate, // OpenDate
        trade.Price,
        trade.Volume,
        false // isYesterday = false for new open
    );

    std::lock_guard<std::mutex> lock(dirPos.m_mtx);

    // 2. 将节点放入队列尾部 (FIFO)
    dirPos.detailQueue.push_back(node);

    // 3. 更新统计数据
    dirPos.totalPos += trade.Volume;
    dirPos.todayPos += trade.Volume;
    
    // 更新持仓均价 (简单的加权平均，或者每次都遍历队列重算，这里为了性能采用增量更新)
    // AvgPrice = (OldAvg * OldVol + NewPrice * NewVol) / TotalVol
    // 注意: totalPos 已经加过了，要排除
    double oldTotalVal = dirPos.avgPrice * (dirPos.totalPos - trade.Volume);
    dirPos.avgPrice = (oldTotalVal + trade.Price * trade.Volume) / dirPos.totalPos;

    // 计算手续费 (粗略估算，具体费率需查表)
    // dirPos.commission += ... (需外部费率表)
}


// === 核心逻辑: 处理平仓(消耗明细) ===
// 支持 SHFE 特殊逻辑: CloseToday, CloseYesterday
void PositionManager::handleClose(std::shared_ptr<ContractPosition> pos, const CThostFtdcTradeField& trade, bool isLong) {
    DirectionPosition& dirPos = isLong ? pos->longPos : pos->shortPos;
    std::lock_guard<std::mutex> lock(dirPos.m_mtx);

    int remainVol = trade.Volume;
    double realizedPnL = 0.0;
    
    // 平仓类型标志
    bool isCloseToday = (trade.OffsetFlag == THOST_FTDC_OF_CloseToday);
    bool isCloseYd = (trade.OffsetFlag == THOST_FTDC_OF_CloseYesterday);
    // 普通 Close (非上期所默认优先平昨，上期所普通Close一般指平昨)
    
    // 遍历队列寻找匹配的可平持仓
    // 使用 erase 必须小心迭代器失效问题
    for (auto it = dirPos.detailQueue.begin(); it != dirPos.detailQueue.end(); ) {
        if (remainVol <= 0) break;

        // 1. 检查是否符合平仓条件 (SHFE 规则)
        // CloseToday: 必须是今仓 (!isYesterday)
        // CloseYesterday: 必须是昨仓 (isYesterday)
        if (isCloseToday && it->isYesterday) {
            ++it; continue; // 跳过昨仓
        }
        if (isCloseYd && !it->isYesterday) {
            ++it; continue; // 跳过今仓
        }

        // 2. 扣减逻辑
        int deduct = std::min(remainVol, it->volume);
        
        // 3. 计算平仓盈亏 (根据开仓价和当前平仓价)
        // 多头盈亏 = (平仓价 - 开仓价) * 手数 * 乘数
        // 空头盈亏 = (开仓价 - 平仓价) * 手数 * 乘数
        double pnl = 0.0;
        if (isLong) {
            pnl = (trade.Price - it->openPrice) * deduct * pos->multiplier;
        } else {
            pnl = (it->openPrice - trade.Price) * deduct * pos->multiplier;
        }
        realizedPnL += pnl;

        // 4. 执行扣减
        it->volume -= deduct;
        remainVol -= deduct;

        // 5. 更新统计
        dirPos.totalPos -= deduct;
        if (it->isYesterday) {
            dirPos.ydPos -= deduct;
        } else {
            dirPos.todayPos -= deduct;
        }

        // 6. 清理已平完的节点 (除非我们需要保留历史记录，一般内存持仓只保留未平)
        if (it->volume <= 0) {
            it = dirPos.detailQueue.erase(it); // 返回下一个迭代器
        } else {
            ++it;
        }
    }

    // 累计平仓盈亏
    dirPos.closeProfit += realizedPnL;

    // 重新计算持仓均价 (剩余持仓的加权平均)
    // 平仓后，avgPrice 实际上不应该变 (因为平掉的部分不影响剩下部分的成本)，除非是按照移动加权平均法
    // 但在 FIFO 逻辑下，avgPrice 应该等于剩余队列的加权平均
    if (dirPos.totalPos > 0) {
        double costSum = 0.0;
        for (const auto& node : dirPos.detailQueue) {
            costSum += node.openPrice * node.volume;
        }
        dirPos.avgPrice = costSum / dirPos.totalPos;
    } else {
        dirPos.avgPrice = 0.0;
    }

    if (remainVol > 0) {
        std::cerr << "[PositionManager] Warning: Over-close detected! Remaining " << remainVol << " vol not found in queue." << std::endl;
        // 此时可能发生了数据不一致，或者是在启动前就有的持仓没有加载进来
        // 在实盘中，这里可能需要强制将 totalPos 减去剩余量，甚至允许负持仓(如果允许的话)
    }
}

} // namespace core
} // namespace atrader
