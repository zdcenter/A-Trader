#pragma once

#include "protocol/message_schema.h"
#include "api/TraderHandler.h"
#include "ThostFtdcUserApiStruct.h"
#include <functional>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <string>

namespace QuantLabs {

class ConditionEngine {
public:
    explicit ConditionEngine(TraderHandler& trader);
    ~ConditionEngine();

    // Add a new condition order
    using StatusCallback = std::function<void(const ConditionOrderRequest&)>;
    void setStatusCallback(StatusCallback cb);

    void addConditionOrder(const ConditionOrderRequest& order);

    // Cancel/Remove
    bool removeConditionOrder(uint64_t request_id);
    
    // Modify condition order (only for pending orders)
    bool modifyConditionOrder(uint64_t request_id, double trigger_price, double limit_price, int volume);

    // Filter and check loop, intended to be called by MdHandler on Tick
    void onTick(const CThostFtdcDepthMarketDataField *pDepthMarketData);

private:
    TraderHandler& trader_;
    
    std::mutex mtx_;
    // Map instrument_id -> List of orders
    std::unordered_map<std::string, std::vector<ConditionOrderRequest>> order_book_;
    
    bool checkCondition(double last_price, const ConditionOrderRequest& order);
    void executeOrder(ConditionOrderRequest& order, const CThostFtdcDepthMarketDataField* pDepthData);

    StatusCallback status_callback_;
};

} // namespace QuantLabs
