#include "strategy/ConditionEngine.h"
#include "storage/DBManager.h" // Added
#include <iostream>
#include <cstring>
#include <algorithm>

namespace QuantLabs {

ConditionEngine::ConditionEngine(TraderHandler& trader) 
    : trader_(trader) {
}

ConditionEngine::~ConditionEngine() {
}

void ConditionEngine::setStatusCallback(StatusCallback cb) {
    status_callback_ = cb;
}

void ConditionEngine::addConditionOrder(const ConditionOrderRequest& order) {
    std::lock_guard<std::mutex> lock(mtx_);
    order_book_[order.instrument_id].push_back(order);
    
    // Persist to DB
    DBManager::instance().saveConditionOrder(order);

    // Push status
    if (status_callback_) status_callback_(order);

    std::cout << "[ConditionEngine] Added order for " << order.instrument_id 
              << ", Trigger: " << order.trigger_price 
              << ", Type: " << (int)order.compare_type << std::endl;
}

bool ConditionEngine::removeConditionOrder(uint64_t request_id) {
    std::lock_guard<std::mutex> lock(mtx_);
    bool found = false;
    for (auto& pair : order_book_) {
        auto& list = pair.second;
        
        // Use find_if first to get the object for callback
        auto it = std::find_if(list.begin(), list.end(), 
            [request_id](const ConditionOrderRequest& order) {
                return order.request_id == request_id;
            });

        if (it != list.end()) {
            // Found, create copy for callback
            ConditionOrderRequest o = *it;
            o.status = 2; // Cancelled
            
            // Remove from memory
            list.erase(it);
            found = true;
            
            // Update DB status to Cancelled (2)
            DBManager::instance().updateConditionOrderStatus(request_id, 2);
            
            // Push status
            if (status_callback_) status_callback_(o);

            break; 
        }
    }
    return found;
}

bool ConditionEngine::modifyConditionOrder(uint64_t request_id, double trigger_price, double limit_price, int volume) {
    std::lock_guard<std::mutex> lock(mtx_);
    bool found = false;
    
    // 遍历所有合约的条件单
    for (auto& [instrument, list] : order_book_) {
        for (auto& order : list) {
            if (order.request_id == request_id && order.status == 0) {
                // 只允许修改待触发的条件单
                order.trigger_price = trigger_price;
                order.limit_price = limit_price;
                order.volume = volume;
                found = true;
                
                // 更新数据库
                DBManager::instance().modifyConditionOrder(request_id, trigger_price, limit_price, volume);
                
                // 推送状态更新（可选）
                if (status_callback_) status_callback_(order);
                
                std::cout << "[ConditionEngine] Modified Order " << request_id 
                          << " (trigger=" << trigger_price << ", limit=" << limit_price << ", vol=" << volume << ")" << std::endl;
                return true;
            }
        }
    }
    
    if (!found) {
        std::cerr << "[ConditionEngine] Modify failed: Order " << request_id << " not found or already triggered/cancelled" << std::endl;
    }
    return found;
}

void ConditionEngine::onTick(const CThostFtdcDepthMarketDataField *pDepthMarketData) {
    if (!pDepthMarketData) return;
    
    std::string instrument = pDepthMarketData->InstrumentID;
    
    // Quick check without lock if possible? No, map access needs lock.
    // Just lock. Ticks are high freq, but map lookup is fast.
    std::lock_guard<std::mutex> lock(mtx_);
    
    auto map_it = order_book_.find(instrument);
    if (map_it == order_book_.end()) {
        return;
    }

    auto& orders = map_it->second;
    if (orders.empty()) return;

    double last_price = pDepthMarketData->LastPrice;

    for (auto it = orders.begin(); it != orders.end(); ) {
        if (checkCondition(last_price, *it)) {
            // Triggered
            executeOrder(*it, pDepthMarketData);
            
            // Remove triggered order
            it = orders.erase(it);
        } else {
            ++it;
        }
    }
}

bool ConditionEngine::checkCondition(double last_price, const ConditionOrderRequest& order) {
    switch (order.compare_type) {
        case CompareType::GreaterThan:
            return last_price > order.trigger_price;
        case CompareType::GreaterOrEqual:
            return last_price >= order.trigger_price;
        case CompareType::LessThan:
            return last_price < order.trigger_price;
        case CompareType::LessOrEqual:
            return last_price <= order.trigger_price;
        default:
            return false;
    }
}

void ConditionEngine::executeOrder(ConditionOrderRequest& order, const CThostFtdcDepthMarketDataField* pDepthData) {
    if (!pDepthData) return;
    double last_price = pDepthData->LastPrice;

    std::cout << "[ConditionEngine] Triggered! " << order.instrument_id 
              << " Last: " << last_price << " Trigger: " << order.trigger_price << std::endl;

    // Update status and persist
    order.status = 1; // Triggered
    DBManager::instance().saveConditionOrder(order);

    // Push status
    if (status_callback_) status_callback_(order);
    
    // 1. Get PriceTick
    InstrumentMeta instr_data;
    double price_tick = 1.0; 
    if (trader_.getInstrumentMeta(order.instrument_id, instr_data)) {
        price_tick = instr_data.price_tick;
    }
    if (price_tick < 1e-6) price_tick = 1.0; // Fallback

    // 2. Calculate Base Price
    double base_price = 0.0;
    
    // Check Price Type (char '0'..'3')
    if (order.price_type == '0') { // Fix
        base_price = order.limit_price;
    } else if (order.price_type == '1') { // Last
        base_price = last_price;
    } else if (order.price_type == '2') { // Opponent
        if (order.direction == THOST_FTDC_D_Buy) {
            // Helper: check if AskPrice1 is valid (usually < very big number)
            if (pDepthData->AskPrice1 < 1e16 && pDepthData->AskPrice1 > 0.0001) 
                 base_price = pDepthData->AskPrice1;
            else base_price = last_price;
        } else {
             if (pDepthData->BidPrice1 < 1e16 && pDepthData->BidPrice1 > 0.0001) 
                 base_price = pDepthData->BidPrice1;
             else base_price = last_price;
        }
    } else if (order.price_type == '3') { // Market
        if (order.direction == THOST_FTDC_D_Buy) {
            base_price = pDepthData->UpperLimitPrice;
        } else {
            base_price = pDepthData->LowerLimitPrice;
        }
    } else {
        // Fallback default
        base_price = order.limit_price; 
    }

    // 3. Apply Offset
    // Only apply offset if NOT Fix and NOT Market (usually)
    // But user might want Market+/- (doesn't make sense), or Last+/-.
    // We apply it for Last ('1') and Opp ('2').
    if (order.price_type == '1' || order.price_type == '2') {
        base_price += (order.tick_offset * price_tick);
    }

    std::cout << "[ConditionEngine] Executing: " << order.instrument_id 
              << " PriceType:" << order.price_type 
              << " Base:" << base_price 
              << " Vol:" << order.volume 
              << " Flag:" << order.offset_flag
              << " Tick:" << price_tick << std::endl;

    // 4. Send Order (带 strategy_id)
    trader_.insertOrder(order.instrument_id, base_price, order.volume, order.direction, order.offset_flag, '2', order.strategy_id);
}

} // namespace QuantLabs
