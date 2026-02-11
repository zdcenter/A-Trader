#ifndef POSITION_STRUCTS_H
#define POSITION_STRUCTS_H

#include <string>
#include <vector>
#include <mutex>

// 适配A-Trader，不直接依赖CTP头文件，使用前向声明
// 或者根据A-Trader的工程习惯，包含ThostFtdcUserApiDataType.h
#include "../lib/ThostFtdcUserApiDataType.h"
#include "../lib/ThostFtdcUserApiStruct.h"

namespace atrader {
namespace core {

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
struct InstrumentMeta {
    std::string InstrumentID;
    std::string ExchangeID;
    double VolumeMultiple;      // 合约乘数
    double PriceTick;           // 最小变动价位
    char PositionDateType;      // 持仓日期类型: '1' 使用历史; '2' 不使用
                                // THOST_FTDC_PDT_UseHistory ('1'): SHFE/INE 模式
                                // THOST_FTDC_PDT_NoUseHistory ('2'): DCE/CZCE/CFFEX 模式
};

} // namespace core
} // namespace atrader

#endif // POSITION_STRUCTS_H
