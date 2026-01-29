#pragma once

#include <string>
#include <cstdint>

namespace atrad {

/**
 * @brief 行情数据结构 (Tick Data)
 * 封装 CTP 深度行情，用于跨进程传输
 */
struct TickData {
    char instrument_id[64];      // 合约代码
    double last_price;           // 最新价
    int volume;                  // 当日成交量
    double open_interest;        // 持仓量
    double turnover;             // 成交金额
    
    // 扩展字段
    double pre_settlement_price; // 昨结算价
    double pre_close_price;      // 昨收盘
    double upper_limit_price;    // 涨停板
    double lower_limit_price;    // 跌停板
    double open_price;           // 开盘价
    double highest_price;        // 最高价
    double lowest_price;         // 最低价
    double close_price;          // 收盘价
    double settlement_price;     // 结算价
    double average_price;        // 均价
    char action_day[16];         // 业务日期
    char trading_day[16];        // 交易日

    // 买卖五档
    double bid_price1;
    int bid_volume1;
    double ask_price1;
    int ask_volume1;

    double bid_price2;
    int bid_volume2;
    double ask_price2;
    int ask_volume2;

    double bid_price3;
    int bid_volume3;
    double ask_price3;
    int ask_volume3;

    double bid_price4;
    int bid_volume4;
    double ask_price4;
    int ask_volume4;

    double bid_price5;
    int bid_volume5;
    double ask_price5;
    int ask_volume5;

    char update_time[16];        // 更新时间 (HH:mm:ss)
    int update_millisec;         // 更新毫秒
};

// 持仓数据
struct PositionData {
    char instrument_id[64];
    char exchange_id[16];   // 交易所代码
    char direction;        // '0' 多, '1' 空
    int position;          // 总持仓
    int today_position;    // 今仓
    int yd_position;       // 昨仓
    double position_cost;  // 持仓成本
    double pos_profit;     // 持仓盈亏
};

// 逐笔持仓明细数据 (对应 CThostFtdcInvestorPositionDetailField)
struct PositionDetailData {
    char trade_id[64];           // 原始开仓成交号
    char instrument_id[64];      // 合约代码
    char exchange_id[16];        // 交易所代码
    char direction;              // 持仓方向: '0' Buy/Long, '1' Sell/Short
    double open_price;           // 开仓价格
    int volume;                  // 剩余持仓量
    char open_date[16];          // 开仓日期
    double margin;               // 占用保证金
    double settlement_price;     // 昨结算价 (用于计算逐日盯市盈亏)
    double close_profit_by_date; // 逐日盯市平仓盈亏
    double close_profit_by_trade;// 逐笔对冲平仓盈亏
    double position_profit_by_date;  // 逐日盯市持仓盈亏
    double position_profit_by_trade; // 逐笔对冲持仓盈亏
};

// 成交数据 (扩展版，包含 CTP 原始数据 + 分析数据)
struct TradeData {
    // CTP Fields
    char instrument_id[64];
    char trade_id[64];
    char order_ref[13]; // CTP is 13
    char exchange_id[16];
    char trade_date[16];
    char trade_time[16];
    char direction;        // '0' Buy, '1' Sell
    char offset_flag;      // '0' Open, '1' Close, etc.
    double price;
    int volume;
    char order_sys_id[64]; // Added for completeness

    // Extended Fields
    double commission;     // 手续费
    double close_profit;   // 平仓盈亏
    char strategy_id[32];  // 策略ID
};

// 账户资金数据
struct AccountData {
    double balance;        // 总权益
    double available;      // 可用资金
    double margin;         // 占用保证金
    double frozen_margin;  // 冻结保证金
    double commission;     // 手续费
};

// 合约属性数据
struct InstrumentData {
    char instrument_id[64];
    char instrument_name[64]; // Added
    char exchange_id[16];
    char product_id[16];            // 品种ID
    char underlying_instr_id[64];   // 标的合约ID
    double strike_price;            // 执行价
    
    int volume_multiple;   // 合约乘数
    double price_tick;     // 最小变动价位

    // 保证金率
    double long_margin_ratio_by_money;
    double long_margin_ratio_by_volume;
    double short_margin_ratio_by_money;
    double short_margin_ratio_by_volume;

    // 手续费率
    double open_ratio_by_money;
    double open_ratio_by_volume;
    double close_ratio_by_money;
    double close_ratio_by_volume;
    double close_today_ratio_by_money;
    double close_today_ratio_by_volume;
    
    // Trading Day for validity check
    char trading_day[16];
};

/**
 * @brief 策略控制指令类型
 */
enum class StrategyCommandType : uint8_t {
    Start = 1,          // 启动策略
    Stop = 2,           // 停止策略
    UpdateParams = 3,   // 更新参数
    Reload = 4          // 重新加载所有策略
};

/**
 * @brief 策略控制结构
 */
struct StrategyControl {
    char strategy_id[64];        // 策略名称/ID
    StrategyCommandType type;    // 指令类型
    char json_payload[1024];     // 具体的参数内容 (JSON 格式)
};

/**
 * @brief 报单指令结构 (简化版)
 */
struct OrderRequest {
    char instrument_id[64];      // 合约
    double price;                // 价格
    int volume;                  // 数量
    char direction;              // '0': 买, '1': 卖 (遵循 CTP 习惯或自定义)
    char offset_flag;            // '0': 开仓, '1': 平仓, '3': 平今
    
    // Added: 策略ID，用于标识订单来源
    char strategy_id[32]; 
};

// 比较条件类型
enum class CompareType : int {
    GreaterThan = 0,        // >
    GreaterOrEqual = 1,     // >=
    LessThan = 2,           // <
    LessOrEqual = 3         // <=
};

// 条件单结构
struct ConditionOrderRequest {
    // 触发条件
    char instrument_id[64];
    double trigger_price;
    CompareType compare_type; 

    // 报单参数
    char direction;         // CTP direction char
    char offset_flag;       // CTP offset flag char
    char price_type;        // '1': Limit, '2': Market (custom mapping)
    double limit_price;     
    int tick_offset;        // 相对多少跳
    int volume;
    
    char strategy_id[32];
    
    // 状态
    int status;             // 0: Pending, 1: Triggered, 2: Cancelled
    uint64_t request_id;    // 唯一ID
};

namespace CmdType {
    const std::string ConditionOrderInsert = "req_condition_order_insert";
    const std::string ConditionOrderCancel = "req_condition_order_cancel";
    const std::string ConditionOrderModify = "req_condition_order_modify"; // 修改条件单
    const std::string Order = "ORDER";
    const std::string Subscribe = "SUBSCRIBE";
    const std::string Unsubscribe = "UNSUBSCRIBE";
    const std::string Ping = "PING";
    const std::string Pong = "PONG";
    const std::string SyncState = "SYNC_STATE";
    const std::string OrderAction = "ORDER_ACTION"; // Added
    const std::string ConditionOrderQuery = "req_condition_order_query"; // Added
    const std::string StrategyQuery = "req_strategy_query"; // Added

    // Returns / Pushes
    const std::string RtnOrder = "rtn_order";
    const std::string RtnTrade = "rtn_trade";
    const std::string RtnPosition = "rtn_position";
    const std::string RtnAccount = "rtn_account";
    const std::string RtnInstrument = "rtn_instrument"; // sync
    const std::string RtnConditionOrder = "rtn_condition_order"; // Added
    const std::string RtnStrategyList = "rtn_strategy_list"; // Added
}

} // namespace atrad
