#ifndef POSITION_STRUCTS_H
#define POSITION_STRUCTS_H

#include <string>
#include <vector>
#include <deque>
#include <cstring>
#include <mutex>

// 适配A-Trader，直接使用文件名，依赖 CMake 的 include_directories
#include "ThostFtdcUserApiDataType.h"
#include "ThostFtdcUserApiStruct.h"
#include "../../shared/protocol/message_schema.h"

namespace atrader {
namespace core {

/**
 * @brief 开仓明细 (FIFO 队列元素)
 * 每笔开仓记录一条，平仓时按先进先出扣减
 * 这是实现逐笔平仓盈亏的基础
 */
struct OpenDetail {
    double price;          // 开仓价格
    int    volume;         // 剩余未平手数（初始等于开仓手数，部分平仓后递减）
    char   open_date[16];  // 开仓日期 (判断今昨仓)
    bool   is_today;       // 是否今仓 (运行时由 PositionManager 维护)

    OpenDetail() : price(0.0), volume(0), is_today(true) {
        std::memset(open_date, 0, sizeof(open_date));
    }
    OpenDetail(double p, int v, const char* date, bool today)
        : price(p), volume(v), is_today(today) {
        std::memset(open_date, 0, sizeof(open_date));
        std::strncpy(open_date, date, sizeof(open_date) - 1);
    }
};

/**
 * @brief 持仓聚合结构体 (All-in-One Position Struct)
 * 聚合多空方向，不再区分多空为不同对象
 * 适配 SHFE/INE 的今昨仓区分逻辑
 */
struct InstrumentPosition {
    // === 基础信息 ===
    std::string InstrumentID;
    std::string ExchangeID;

    // === 多头数据 (Long) ===
    int LongPosition;           // 多头总持仓
    int LongTodayPosition;      // 多头今仓 (Today's Position)
    int LongYdPosition;         // 多头昨仓 (Yesterday's Position)
    
    double LongPositionCost;    // 多头持仓成本 (SHFE: 昨仓按昨结，今仓按开仓；其他:按开仓均价)
    double LongOpenCost;        // 多头开仓成本 (始终按开仓价)
    double LongFrozenMargin;    // 多头冻结保证金
    double LongPositionProfit;  // 多头持仓盈亏 (浮动盈亏)
    double LongCloseProfit;     // 多头平仓盈亏 (实现盈亏)
    double LongCommission;      // 多头手续费累计

    // === 空头数据 (Short) ===
    int ShortPosition;          // 空头总持仓
    int ShortTodayPosition;     // 空头今仓
    int ShortYdPosition;        // 空头昨仓

    double ShortPositionCost;   // 空头持仓成本
    double ShortOpenCost;       // 空头开仓成本
    double ShortFrozenMargin;   // 空头冻结保证金
    double ShortPositionProfit; // 空头持仓盈亏
    double ShortCloseProfit;    // 空头平仓盈亏
    double ShortCommission;     // 空头手续费累计
    
    // === 公共数据 ===
    double PreSettlementPrice;  // 昨结算价 (计算盈亏的关键基准)
    double Margin;              // 占用保证金 (Exchange Margin)
    double LastPrice;           // 最新价 (用于计算浮动盈亏)

    // === FIFO 开仓明细队列 (逐笔平仓盈亏的核心) ===
    std::deque<OpenDetail> LongDetails;   // 多头开仓明细 (先开的在前)
    std::deque<OpenDetail> ShortDetails;  // 空头开仓明细
    
    // === 构造函数 ===
    InstrumentPosition() {
        LongPosition = 0; LongTodayPosition = 0; LongYdPosition = 0;
        LongPositionCost = 0.0; LongOpenCost = 0.0; 
        LongFrozenMargin = 0.0; LongPositionProfit = 0.0; LongCloseProfit = 0.0;
        LongCommission = 0.0;

        ShortPosition = 0; ShortTodayPosition = 0; ShortYdPosition = 0;
        ShortPositionCost = 0.0; ShortOpenCost = 0.0;
        ShortFrozenMargin = 0.0; ShortPositionProfit = 0.0; ShortCloseProfit = 0.0;
        ShortCommission = 0.0;

        PreSettlementPrice = 0.0;
        Margin = 0.0;
        LastPrice = 0.0;
    }
};

/**
 * @brief 合约元数据缓存
 * 用于判断交易所规则 (如是否区分今昨仓)
 */
// Include message_schema.h for unified InstrumentMeta definition
// Include message_schema.h moved to top
// #include "../../shared/protocol/message_schema.h"

// Re-export QuantLabs::InstrumentMeta into atrader::core namespace for compatibility
using InstrumentMeta = QuantLabs::InstrumentMeta;

// REMOVED: Local InstrumentMeta definition
// struct InstrumentMeta { ... };

} // namespace core
} // namespace atrader

#endif // POSITION_STRUCTS_H
