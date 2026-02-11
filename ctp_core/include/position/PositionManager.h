
#ifndef POSITION_MANAGER_H
#define POSITION_MANAGER_H

#include <unordered_map>
#include <mutex>

#include "PositionStructs.h"

namespace atrader {
namespace core {

/**
 * @class PositionManager
 * @brief 高级持仓管理器：基于先进先出(FIFO)队列的持仓明细管理
 */
class PositionManager {
public:
    PositionManager() = default;
    ~PositionManager() = default;

    // === 核心逻辑 ===
    /**
     * @brief 基于成交回报更新持仓
     * @param trade CTP Trade Field
     */
    void UpdateFromTrade(const CThostFtdcTradeField& trade);

    /**
     * @brief 更新合约元数据 (乘数、价格跳动等)，必须先调用此函数才能正确计算盈亏
     */
    void UpdateInstrument(const CThostFtdcInstrumentField& instrument);

    /**
     * @brief 获取指定合约的完整持仓对象
     */
    std::shared_ptr<ContractPosition> GetPosition(const std::string& instrumentID);

    /**
     * @brief 获取所有持仓
     */
    std::unordered_map<std::string, std::shared_ptr<ContractPosition>>& GetAllPositions();

private:
    // === 内部辅助函数 ===

    /**
     * @brief 处理开仓 (Open) -> 增加队列
     * @param pos 指向合约持仓的指针
     * @param trade 成交明细
     * @param isLong 是否多头
     */
    void handleOpen(std::shared_ptr<ContractPosition> pos, const CThostFtdcTradeField& trade, bool isLong);

    /**
     * @brief 处理平仓 (Close) -> 消耗队列
     * @param pos 指向合约持仓的指针
     * @param trade 成交明细
     * @param isLong 是否多头 (注意: 卖平是减少多头，买平是减少空头)
     */
    void handleClose(std::shared_ptr<ContractPosition> pos, const CThostFtdcTradeField& trade, bool isLong);

private:
    // 数据存储: InstrumentID -> ContractPosition (改为 SharedPtr 以便线程安全传递)
    std::unordered_map<std::string, std::shared_ptr<ContractPosition>> positions_;
    
    // 线程安全互斥锁
    std::mutex m_mtx; 
};

} // namespace core
} // namespace atrader

#endif // POSITION_MANAGER_H
