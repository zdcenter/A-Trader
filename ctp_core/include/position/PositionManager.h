#ifndef POSITION_MANAGER_H
#define POSITION_MANAGER_H

#include <unordered_map>
#include <mutex>
#include <memory> 
#include "PositionStructs.h"

namespace atrader {
namespace core {

class PositionManager {
public:
    PositionManager() = default;
    ~PositionManager() = default;

    // 更新逻辑
    double UpdateFromTrade(const CThostFtdcTradeField& trade);
    void UpdateInstrument(const CThostFtdcInstrumentField& instrument);
    void UpdateInstrumentMeta(const InstrumentMeta& meta); // 新增：更新完整元数据(含费率)

    // 查询接口
    std::shared_ptr<InstrumentPosition> GetPosition(const std::string& instrumentID);
    std::unordered_map<std::string, std::shared_ptr<InstrumentPosition>>& GetAllPositions();
    void Clear();

private:
    std::unordered_map<std::string, std::shared_ptr<InstrumentPosition>> positions_;
    std::unordered_map<std::string, InstrumentMeta> instruments_meta_;
    std::mutex m_mutex; 
};

} // namespace core
} // namespace atrader

#endif // POSITION_MANAGER_H
