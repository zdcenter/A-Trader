#include "models/OrderController.h"
#include <QDebug>
#include <cstring>

namespace atrad {

OrderController::OrderController(QObject *parent) : QObject(parent) {
    connect(this, &OrderController::orderParamsChanged, this, &OrderController::recalculate);
}

// 新增 onTick 实现
void OrderController::onTick(const QString& json) {
    if (_instrumentId.isEmpty() || _isManualPrice) return;
    
    try {
        auto j = nlohmann::json::parse(json.toStdString());
        
        std::string id;
        if (j.contains("instrument_id")) id = j["instrument_id"];
        else if (j.contains("id")) id = j["id"];
        
        if (id == _instrumentId.toStdString()) {
            double lastPrice = 0.0;
            if (j.contains("last_price")) lastPrice = j["last_price"];
            else if (j.contains("price")) lastPrice = j["price"]; // compat
            
            if (lastPrice > 0.0001 && std::abs(_price - lastPrice) > 0.0001) {
                _price = lastPrice;
                emit orderParamsChanged();
            }
            
            // 解析 5 档行情
            _bidPrices.clear(); _bidVolumes.clear();
            _askPrices.clear(); _askVolumes.clear();
            
            // Bid 1-5
            for(int i=1; i<=5; ++i) {
               std::string pKey = "bid_price" + std::to_string(i);
               std::string vKey = "bid_volume" + std::to_string(i);
               // compat
               std::string oldP = "b" + std::to_string(i);
               std::string oldV = "bv" + std::to_string(i);

               if (j.contains(pKey)) _bidPrices.append(j[pKey].get<double>());
               else if (j.contains(oldP)) _bidPrices.append(j[oldP].get<double>());
               else _bidPrices.append(0.0);

               if (j.contains(vKey)) _bidVolumes.append(j[vKey].get<int>());
               else if (j.contains(oldV)) _bidVolumes.append(j[oldV].get<int>());
               else _bidVolumes.append(0);
            }

            // Ask 1-5
            for(int i=1; i<=5; ++i) {
               std::string pKey = "ask_price" + std::to_string(i);
               std::string vKey = "ask_volume" + std::to_string(i);
               std::string oldP = "a" + std::to_string(i);
               std::string oldV = "av" + std::to_string(i);

               if (j.contains(pKey)) _askPrices.append(j[pKey].get<double>());
               else if (j.contains(oldP)) _askPrices.append(j[oldP].get<double>());
               else _askPrices.append(0.0);

               if (j.contains(vKey)) _askVolumes.append(j[vKey].get<int>());
               else if (j.contains(oldV)) _askVolumes.append(j[oldV].get<int>());
               else _askVolumes.append(0);
            }
            
            emit marketDataChanged();
        }
    } catch (...) {}
}

void OrderController::updateInstrument(const QString& json) {
    try {
        auto j = nlohmann::json::parse(json.toStdString());
        // JSON key is now "instrument_id"
        // Wait, if json parsing fails on key lookup it throws (operator[]) or return null/default (value)
        // Better use .value() for safety or verify key exists.
        
        QString idStr;
        if (j.contains("instrument_id")) idStr = QString::fromStdString(j["instrument_id"]);
        else if (j.contains("id")) idStr = QString::fromStdString(j["id"]); // fallback compatibility?
        
        if (idStr.isEmpty()) return;
        
        InstrumentData info;
        std::memset(&info, 0, sizeof(info)); 

        std::string s_id = j.value("instrument_id", "");
        std::string s_name = j.value("instrument_name", "");
        std::string s_exch = j.value("exchange_id", "");
        std::string s_pid = j.value("product_id", "");
        
        strncpy(info.instrument_id, s_id.c_str(), sizeof(info.instrument_id) - 1);
        strncpy(info.instrument_name, s_name.c_str(), sizeof(info.instrument_name) - 1);
        strncpy(info.exchange_id, s_exch.c_str(), sizeof(info.exchange_id) - 1);
        strncpy(info.product_id, s_pid.c_str(), sizeof(info.product_id) - 1);

        info.volume_multiple = j.value("volume_multiple", 1);
        info.price_tick = j.value("price_tick", 0.0);

        info.long_margin_ratio_by_money = j.value("long_margin_ratio_by_money", 0.0); 
        info.short_margin_ratio_by_money = j.value("short_margin_ratio_by_money", 0.0);
        
        info.open_ratio_by_money = j.value("open_ratio_by_money", 0.0);
        info.open_ratio_by_volume = j.value("open_ratio_by_volume", 0.0);
        
        qDebug() << "[OrderController] Instrument Updated:" << idStr 
                 << "Mult:" << info.volume_multiple 
                 << "MarginRatio:" << info.long_margin_ratio_by_money;

        _instrument_dict[idStr] = info;
        
        if (idStr == _instrumentId) {
            recalculate();
        } else if (!_instrumentId.isEmpty() && _instrument_dict.contains(_instrumentId)) {
            // 如果更新的是当前合约的 ProductID (例如当前是 p2605，收到 p 的更新)，也需要重算
            if (idStr == QString::fromUtf8(_instrument_dict[_instrumentId].product_id)) {
                qDebug() << "[OrderController] Product ID updated (" << idStr << "), recalculating for " << _instrumentId;
                recalculate();
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "[OrderController] JSON Parse Error:" << e.what();
    }
}

void OrderController::recalculate() {
    if (_instrumentId.isEmpty() || !_instrument_dict.contains(_instrumentId)) {
        _estimatedMargin = 0.0;
        _estimatedCommission = 0.0;
        emit calculationChanged();
        return;
    }

    auto info = _instrument_dict[_instrumentId];
    
    QString pid = QString::fromUtf8(info.product_id);
    bool hasProduct = !pid.isEmpty() && _instrument_dict.contains(pid);

    // 1. 独立降级策略：保证金
    if (info.long_margin_ratio_by_money < 0.0000001 && info.long_margin_ratio_by_volume < 0.0000001) {
        if (hasProduct) {
             const auto& pInfo = _instrument_dict[pid];
             if (pInfo.long_margin_ratio_by_money > 0) info.long_margin_ratio_by_money = pInfo.long_margin_ratio_by_money;
             if (pInfo.long_margin_ratio_by_volume > 0) info.long_margin_ratio_by_volume = pInfo.long_margin_ratio_by_volume;
        }
    }

    // 2. 独立降级策略：手续费
    if (info.open_ratio_by_money < 0.0000001 && info.open_ratio_by_volume < 0.0000001) {
        if (hasProduct) {
            const auto& pInfo = _instrument_dict[pid];
            if (pInfo.open_ratio_by_money > 0) info.open_ratio_by_money = pInfo.open_ratio_by_money;
            if (pInfo.open_ratio_by_volume > 0) info.open_ratio_by_volume = pInfo.open_ratio_by_volume;
        }
    }
    
    // 3. 计算预估保证金 (支持按手数和按金额混合计费)
    // 公式: (按手数费率 + 按金额费率 * 价格 * 合约乘数) * 手数
    _estimatedMargin = (info.long_margin_ratio_by_volume + 
                       info.long_margin_ratio_by_money * _price * info.volume_multiple) * _volume;

    // 4. 计算预估手续费
    // 公式: (按手数费率 + 按金额费率 * 价格 * 合约乘数) * 手数
    _estimatedCommission = (info.open_ratio_by_volume + 
                           info.open_ratio_by_money * _price * info.volume_multiple) * _volume;

    emit calculationChanged();
}

void OrderController::sendOrder(const QString& direction, const QString& offset) {
    if (_instrumentId.isEmpty() || _volume <= 0) return;

    nlohmann::json j;
    j["type"] = atrad::CmdType::Order;
    j["id"] = _instrumentId.toStdString();
    j["price"] = _price;
    j["vol"] = _volume;
    j["dir"] = direction.toStdString();
    j["off"] = offset.toStdString();

    emit orderSent(QString::fromStdString(j.dump()));
    qDebug() << "Order Published:" << _instrumentId << direction << offset;
}

void OrderController::subscribe(const QString& id) {
    if (id.isEmpty()) return;
    qDebug() << "[OrderController] subscribe called with:" << id;
    nlohmann::json j;
    j["type"] = atrad::CmdType::Subscribe;
    j["id"] = id.toStdString();
    QString jsonStr = QString::fromStdString(j.dump());
    qDebug() << "[OrderController] Emitting orderSent:" << jsonStr;
    emit orderSent(jsonStr);
}

void OrderController::unsubscribe(const QString& id) {
    if (id.isEmpty()) return;
    nlohmann::json j;
    j["type"] = atrad::CmdType::Unsubscribe;
    j["id"] = id.toStdString();
    emit orderSent(QString::fromStdString(j.dump()));
}



void OrderController::onPositionReceived(const QString& json) {
    try {
        auto j = nlohmann::json::parse(json.toStdString());
        QString id;
        if (j.contains("instrument_id")) id = QString::fromStdString(j["instrument_id"]);
        
        if (id.isEmpty()) return;

        char dir = 0;
        if(j.contains("direction")) {
            if (j["direction"].is_string()) dir = j["direction"].get<std::string>()[0]; 
            else dir = (char)j["direction"].get<int>();
        }

        int pos = 0;
        if(j.contains("position")) pos = j["position"];

        int td = 0;
        if(j.contains("today_position")) td = j["today_position"];

        int yd = 0;
        if(j.contains("yd_position")) yd = j["yd_position"];

        // Update cache
        if (!_pos_cache.contains(id)) {
            _pos_cache[id] = PosSummary{};
        }
        
        auto& summary = _pos_cache[id];
        
        if (dir == '2' || dir == '0') { // Long
            summary.longTotal = pos;
            summary.longTd = td;
            summary.longYd = yd; 
        } else { // Short (1 or 3)
            summary.shortTotal = pos;
            summary.shortTd = td;
            summary.shortYd = yd;
        }

        if (id == _instrumentId) {
            _currentPos = summary;
            emit positionChanged();
        }

    } catch (...) {}
}

void OrderController::updateCurrentPos(const QString& id) {
    if (_pos_cache.contains(id)) {
        _currentPos = _pos_cache[id];
    } else {
        _currentPos = PosSummary{};
    }
    emit positionChanged();
}

void OrderController::cancelConditionOrder(const QString& requestId) {
    nlohmann::json req;
    req["type"] = atrad::CmdType::ConditionOrderCancel;
    
    nlohmann::json data;
    data["request_id"] = requestId.toULongLong();
    req["data"] = data;
    
    emit orderSent(QString::fromStdString(req.dump()));
}

void OrderController::queryConditionOrders() {
    nlohmann::json req;
    req["type"] = atrad::CmdType::ConditionOrderQuery;
    emit orderSent(QString::fromStdString(req.dump()));
}

void OrderController::queryStrategies() {
    nlohmann::json req;
    req["type"] = atrad::CmdType::StrategyQuery;
    emit orderSent(QString::fromStdString(req.dump()));
}

void OrderController::modifyConditionOrder(const QString& requestId, double triggerPrice, double limitPrice, int volume) {
    nlohmann::json req;
    req["type"] = atrad::CmdType::ConditionOrderModify;
    req["data"]["request_id"] = requestId.toULongLong();
    req["data"]["trigger_price"] = triggerPrice;
    req["data"]["limit_price"] = limitPrice;
    req["data"]["volume"] = volume;
    
    qDebug() << "[OrderController] Modifying Condition Order:" << requestId 
             << "trigger=" << triggerPrice << "limit=" << limitPrice << "vol=" << volume;
    
    emit orderSent(QString::fromStdString(req.dump()));
}

void OrderController::onConditionOrderReturn(const QString& json) {
    try {
        auto j = nlohmann::json::parse(json.toStdString());
        
        std::string type;
        if (j.contains("type")) type = j["type"];

        // Handle Strategy List Response
        if (type == atrad::CmdType::RtnStrategyList) {
            if (j.contains("data") && j["data"].is_array()) {
                _strategyList.clear();
                
                // Optional: Add an "empty" option if needed, but ComboBox handles empty well
                QVariantMap empty;
                empty["id"] = "";
                empty["name"] = "无策略 (None)";
                _strategyList.append(empty);

                auto arr = j["data"];
                for (const auto& item : arr) {
                    QVariantMap map;
                    map["id"] = QString::fromStdString(item["id"]);
                    map["name"] = QString::fromStdString(item["name"]);
                    _strategyList.append(map);
                }
                emit strategyListChanged();
                qDebug() << "[OrderController] Loaded " << _strategyList.size() << " strategies.";
            }
            return;
        }

        // Case 1: List Response (from condition order query)
        // If it doesn't have explicit type RtnConditionOrder but has data array and NO type... (old behavior)
        // OR type is correct.
        if (j.contains("data") && j["data"].is_array()) {
            _conditionOrderList.clear();
            auto arr = j["data"];
            // 倒序插入，使最新的单子在最前面
            for (auto it = arr.rbegin(); it != arr.rend(); ++it) {
                const auto& item = *it;
                QVariantMap map;
                map["request_id"] = QString::number(item["request_id"].get<uint64_t>());
                map["instrument_id"] = QString::fromStdString(item["instrument_id"]);
                map["trigger_price"] = item["trigger_price"].get<double>();
                map["compare_type"] = item["compare_type"].get<int>();
                map["status"] = item["status"].get<int>();
                map["direction"] = QString::fromStdString(item["direction"]);
                map["offset_flag"] = QString::fromStdString(item["offset_flag"]);
                map["volume"] = item["volume"].get<int>();
                map["limit_price"] = item["limit_price"].get<double>();
                map["strategy_id"] = QString::fromStdString(item["strategy_id"]);
                _conditionOrderList.append(map);
            }
            emit conditionOrderListChanged();
            qDebug() << "[OrderController] Loaded " << _conditionOrderList.size() << " condition orders.";
            return;
        }

        // Case 2: Single Push (from RtnConditionOrder)
        // Check if it's wrapped in "data" or flat
        auto data = j.contains("data") ? j["data"] : j;
        
        uint64_t reqId = data["request_id"].get<uint64_t>();
        QString reqIdStr = QString::number(reqId);
        int newStatus = data["status"].get<int>();
        
        // Check if exists
        bool found = false;
        for (int i=0; i<_conditionOrderList.size(); ++i) {
            QVariantMap map = _conditionOrderList[i].toMap();
            if (map["request_id"].toString() == reqIdStr) {
                // Update or Remove? 
                // Creating a new map to update
                // Always update status, don't remove.
                int oldStatus = map["status"].toInt();
                map["status"] = newStatus;
                
                // 更新价格和数量（如果后端返回了这些字段）
                if (data.contains("trigger_price")) {
                    map["trigger_price"] = data["trigger_price"].get<double>();
                }
                if (data.contains("limit_price")) {
                    map["limit_price"] = data["limit_price"].get<double>();
                }
                if (data.contains("volume")) {
                    map["volume"] = data["volume"].get<int>();
                }
                
                _conditionOrderList[i] = map;
                found = true;
                
                // 发送声音提示（仅在状态改变时）
                if (oldStatus != newStatus) {
                    if (newStatus == 1) {
                        // 触发
                        emit conditionOrderSound("triggered");
                        qDebug() << "[OrderController] Condition Order Triggered:" << reqIdStr;
                    } else if (newStatus == 2) {
                        // 取消
                        emit conditionOrderSound("cancelled");
                        qDebug() << "[OrderController] Condition Order Cancelled:" << reqIdStr;
                    }
                }
                break;
            }
        }
        
        if (!found && newStatus == 0) {
            // New Order
            QVariantMap map;
            map["request_id"] = reqIdStr;
            map["instrument_id"] = QString::fromStdString(data["instrument_id"]);
            map["trigger_price"] = data["trigger_price"].get<double>();
            map["compare_type"] = data["compare_type"].get<int>();
            map["status"] = data["status"].get<int>();
            map["direction"] = QString::fromStdString(data["direction"]);
            map["offset_flag"] = QString::fromStdString(data["offset_flag"]);
            map["volume"] = data["volume"].get<int>();
            map["limit_price"] = data["limit_price"].get<double>();
            map["strategy_id"] = QString::fromStdString(data["strategy_id"]);
            _conditionOrderList.prepend(map);  // 插入到最前面
        }
        
        emit conditionOrderListChanged();
        
    } catch (const std::exception& e) {
        qDebug() << "[OrderController] ConditionOrder Return Parse Error:" << e.what();
    }
}

void OrderController::updateConnectionStatus(bool core, bool ctp) {
    if (_coreConnected != core || _ctpConnected != ctp) {
        _coreConnected = core;
        _ctpConnected = ctp;
        emit connectionChanged();
        
        if (core) {
            _conditionOrderList.clear();
            emit conditionOrderListChanged();
            queryConditionOrders();
            queryStrategies(); // Added
        }
    }
}



void OrderController::sendCommand(const QString& cmd) {
    emit orderSent(cmd);
}

void OrderController::sendConditionOrder(const QString& dataJson) {
    try {
        nlohmann::json data = nlohmann::json::parse(dataJson.toStdString());
        nlohmann::json req;
        // Use the constant from message_schema.h
        req["type"] = atrad::CmdType::ConditionOrderInsert; 
        req["data"] = data;
        emit orderSent(QString::fromStdString(req.dump()));
        qDebug() << "[OrderController] Condition Order Wrapped & Sent";
    } catch (const std::exception& e) {
        qDebug() << "[OrderController] Failed to parse Condition Order Data:" << e.what();
    }
}

double OrderController::getInstrumentPriceTick(const QString& instrumentId) const {
    if (instrumentId.isEmpty() || !_instrument_dict.contains(instrumentId)) {
        return 1.0; // 默认返回1.0
    }
    double tick = _instrument_dict[instrumentId].price_tick;
    return (tick < 1e-6) ? 1.0 : tick;
}

} // namespace atrad
