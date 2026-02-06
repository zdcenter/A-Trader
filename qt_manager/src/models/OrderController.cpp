#include "models/OrderController.h"
#include <QDebug>
#include <cstring>
#include <QJsonArray>
#include <QJsonDocument>

namespace QuantLabs {

OrderController::OrderController(QObject *parent) : QObject(parent) {
    connect(this, &OrderController::orderParamsChanged, this, &OrderController::recalculate);
}

// 新增 onTick 实现
void OrderController::onTick(const QJsonObject& j) {
    if (_instrumentId.isEmpty() || _isManualPrice) return;
    
    try {
        std::string id;
        if (j.contains("instrument_id")) id = j["instrument_id"].toString().toStdString();
        else if (j.contains("id")) id = j["id"].toString().toStdString();
        
        if (id == _instrumentId.toStdString()) {
            double lastPrice = 0.0;
            if (j.contains("last_price")) lastPrice = j["last_price"].toDouble();
            else if (j.contains("price")) lastPrice = j["price"].toDouble(); // compat
            
            if (lastPrice > 0.0001 && std::abs(_price - lastPrice) > 0.0001) {
                _price = lastPrice;
                emit orderParamsChanged();
            }
            
            // 解析 5 档行情
            _bidPrices.clear(); _bidVolumes.clear();
            _askPrices.clear(); _askVolumes.clear();
            
            // Bid 1-5
            for(int i=1; i<=5; ++i) {
               QString pKey = "bid_price" + QString::number(i);
               QString vKey = "bid_volume" + QString::number(i);
               // compat
               QString oldP = "b" + QString::number(i);
               QString oldV = "bv" + QString::number(i);

               if (j.contains(pKey)) _bidPrices.append(j[pKey].toDouble());
               else if (j.contains(oldP)) _bidPrices.append(j[oldP].toDouble());
               else _bidPrices.append(0.0);

               if (j.contains(vKey)) _bidVolumes.append(j[vKey].toInt());
               else if (j.contains(oldV)) _bidVolumes.append(j[oldV].toInt());
               else _bidVolumes.append(0);
            }

            // Ask 1-5
            for(int i=1; i<=5; ++i) {
               QString pKey = "ask_price" + QString::number(i);
               QString vKey = "ask_volume" + QString::number(i);
               QString oldP = "a" + QString::number(i);
               QString oldV = "av" + QString::number(i);

               if (j.contains(pKey)) _askPrices.append(j[pKey].toDouble());
               else if (j.contains(oldP)) _askPrices.append(j[oldP].toDouble());
               else _askPrices.append(0.0);

               if (j.contains(vKey)) _askVolumes.append(j[vKey].toInt());
               else if (j.contains(oldV)) _askVolumes.append(j[oldV].toInt());
               else _askVolumes.append(0);
            }
            
            emit marketDataChanged();
        }
    } catch (...) {}
}

void OrderController::updateInstrument(const QJsonObject& j) {
    try {
        QString idStr;
        if (j.contains("instrument_id")) idStr = j["instrument_id"].toString();
        else if (j.contains("id")) idStr = j["id"].toString(); 
        
        if (idStr.isEmpty()) return;
        
        // 增量更新逻辑：先获取现有数据
        InstrumentData info;
        if (_instrument_dict.contains(idStr)) {
            info = _instrument_dict[idStr];
        } else {
            std::memset(&info, 0, sizeof(info)); 
        }

        if (j.contains("instrument_id")) {
             std::string s = j["instrument_id"].toString().toStdString();
             strncpy(info.instrument_id, s.c_str(), sizeof(info.instrument_id) - 1);
        }
        if (j.contains("instrument_name")) {
             std::string s = j["instrument_name"].toString().toStdString();
             strncpy(info.instrument_name, s.c_str(), sizeof(info.instrument_name) - 1);
        }
        if (j.contains("exchange_id")) {
             std::string s = j["exchange_id"].toString().toStdString();
             strncpy(info.exchange_id, s.c_str(), sizeof(info.exchange_id) - 1);
        }
        
        if (j.contains("product_id")) {
            std::string s = j["product_id"].toString().toStdString();
            strncpy(info.product_id, s.c_str(), sizeof(info.product_id) - 1);
        }

        // 关键字段 PriceTick
        if (j.contains("price_tick")) {
            double v = j["price_tick"].toDouble();
            if (v > 1e-9) info.price_tick = v; // 仅当有效时更新
        }
        
        if (j.contains("volume_multiple")) info.volume_multiple = j["volume_multiple"].toInt();

        // 费率
        if (j.contains("long_margin_ratio_by_money")) info.long_margin_ratio_by_money = j["long_margin_ratio_by_money"].toDouble();
        if (j.contains("long_margin_ratio_by_volume")) info.long_margin_ratio_by_volume = j["long_margin_ratio_by_volume"].toDouble();
        if (j.contains("short_margin_ratio_by_money")) info.short_margin_ratio_by_money = j["short_margin_ratio_by_money"].toDouble();
        if (j.contains("short_margin_ratio_by_volume")) info.short_margin_ratio_by_volume = j["short_margin_ratio_by_volume"].toDouble();
        
        if (j.contains("open_ratio_by_money")) info.open_ratio_by_money = j["open_ratio_by_money"].toDouble();
        if (j.contains("open_ratio_by_volume")) info.open_ratio_by_volume = j["open_ratio_by_volume"].toDouble();
        if (j.contains("close_ratio_by_money")) info.close_ratio_by_money = j["close_ratio_by_money"].toDouble();
        if (j.contains("close_ratio_by_volume")) info.close_ratio_by_volume = j["close_ratio_by_volume"].toDouble();
        if (j.contains("close_today_ratio_by_money")) info.close_today_ratio_by_money = j["close_today_ratio_by_money"].toDouble();
        if (j.contains("close_today_ratio_by_volume")) info.close_today_ratio_by_volume = j["close_today_ratio_by_volume"].toDouble();

        qDebug() << "[OrderController] Instrument Updated:" << idStr 
                 << "Tick:" << info.price_tick 
                 << "Margin:" << info.long_margin_ratio_by_money;

        _instrument_dict[idStr] = info;
        
        if (idStr == _instrumentId) {
            recalculate();
            emit orderParamsChanged();
        } else if (!_instrumentId.isEmpty() && _instrument_dict.contains(_instrumentId)) {
            if (idStr == QString::fromUtf8(_instrument_dict[_instrumentId].product_id)) {
                recalculate();
            }
        }
    } catch (const std::exception& e) {
        qDebug() << "[OrderController] Error:" << e.what();
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



void OrderController::sendOrder(const QString& direction, const QString& offset, const QString& priceType) {
    if (_instrumentId.isEmpty() || _volume <= 0) return;

    nlohmann::json j;
    j["type"] = QuantLabs::CmdType::Order;
    j["id"] = _instrumentId.toStdString();
    
    // Map Direction
    if (direction == "BUY") j["dir"] = "0"; // CTP Buy
    else if (direction == "SELL") j["dir"] = "1"; // CTP Sell
    else j["dir"] = direction.toStdString();
    
    // Map Offset
    if (offset == "OPEN") j["off"] = "0"; // CTP Open
    else if (offset == "CLOSE") j["off"] = "1"; // CTP Close
    else if (offset == "CLOSETODAY") j["off"] = "3"; // CTP CloseToday
    else j["off"] = offset.toStdString();

    // Map Price Type & Price
    // Map Price Type & Price
    if (priceType == "MARKET") {
        j["price_type"] = "1"; // CTP AnyPrice
        
        // Pass the manually set price (calculated in QML) for simulation
        // If _price is 0, try to fallback to opponent price
        double finalPrice = _price;
        if (finalPrice < 0.0001) {
             bool isBuy = (direction == "BUY" || direction == "0" || direction == "2");
             if (isBuy && !_askPrices.isEmpty() && _askPrices[0].toDouble() > 0.0001) {
                 finalPrice = _askPrices[0].toDouble();
             } else if (!isBuy && !_bidPrices.isEmpty() && _bidPrices[0].toDouble() > 0.0001) {
                 finalPrice = _bidPrices[0].toDouble();
             }
        }
        j["price"] = finalPrice;
    } else if (priceType == "OPPONENT") {
        j["price_type"] = "2"; // CTP LimitPrice
        
        double targetPrice = _price;
        bool isBuy = (direction == "BUY" || direction == "0" || direction == "2");
        
        if (isBuy) {
            // Buying -> Ask Price
            if (!_askPrices.isEmpty() && _askPrices[0].toDouble() > 0.0001) {
                targetPrice = _askPrices[0].toDouble();
            }
        } else {
            // Selling -> Bid Price
            if (!_bidPrices.isEmpty() && _bidPrices[0].toDouble() > 0.0001) {
                targetPrice = _bidPrices[0].toDouble();
            }
        }
        j["price"] = targetPrice;
    } else {
        j["price_type"] = "2"; // CTP LimitPrice
        j["price"] = _price;
    }

    j["vol"] = _volume;

    emit orderSent(QString::fromStdString(j.dump()));
    qDebug() << "Order Published:" << _instrumentId << direction << offset << priceType;
}

void OrderController::cancelOrder(const QString& instrumentId, const QString& orderSysId, const QString& orderRef, const QString& exchangeId, int frontId, int sessionId) {
    nlohmann::json j;
    j["type"] = QuantLabs::CmdType::OrderAction; // Use OrderAction type (usually 'action')
    j["id"] = instrumentId.toStdString();
    
    nlohmann::json data;
    data["ActionFlag"] = '0'; // Delete
    data["InstrumentID"] = instrumentId.toStdString();
    data["OrderSysID"] = orderSysId.toStdString();
    data["OrderRef"] = orderRef.toStdString();
    data["ExchangeID"] = exchangeId.toStdString(); 
    data["FrontID"] = frontId;
    data["SessionID"] = sessionId;
    
    // 我们需要在 Core 侧解析这个结构，或者简化协议。
    // 为了保持一致，我们构造一个简单协议，类似 Order insert
    // { "type": 2 (Action), "data": { ... } }
    
    j["data"] = data;
    
    emit orderSent(QString::fromStdString(j.dump()));
    qDebug() << "[OrderController] Cancel Order Sent:" << instrumentId << orderRef;
}

void OrderController::subscribe(const QString& id) {
    if (id.isEmpty()) return;
    qDebug() << "[OrderController] subscribe called with:" << id;
    nlohmann::json j;
    j["type"] = QuantLabs::CmdType::Subscribe;
    j["id"] = id.toStdString();
    QString jsonStr = QString::fromStdString(j.dump());
    qDebug() << "[OrderController] Emitting orderSent:" << jsonStr;
    emit orderSent(jsonStr);
}

void OrderController::unsubscribe(const QString& id) {
    if (id.isEmpty()) return;
    nlohmann::json j;
    j["type"] = QuantLabs::CmdType::Unsubscribe;
    j["id"] = id.toStdString();
    emit orderSent(QString::fromStdString(j.dump()));
}




void OrderController::onPositionReceived(const QJsonObject& j) {
    try {
        QString id;
        if (j.contains("instrument_id")) id = j["instrument_id"].toString();
        
        if (id.isEmpty()) return;

        char dir = 0;
        if(j.contains("direction")) {
            QJsonValue v = j["direction"];
            if (v.isString()) dir = v.toString().toStdString()[0];
            else dir = (char)v.toInt();
        }

        int pos = 0;
        if(j.contains("position")) pos = j["position"].toInt();

        int td = 0;
        if(j.contains("today_position")) td = j["today_position"].toInt();

        int yd = 0;
        if(j.contains("yd_position")) yd = j["yd_position"].toInt();

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
    req["type"] = QuantLabs::CmdType::ConditionOrderCancel;
    
    nlohmann::json data;
    data["request_id"] = requestId.toULongLong();
    req["data"] = data;
    
    emit orderSent(QString::fromStdString(req.dump()));
}

void OrderController::queryConditionOrders() {
    nlohmann::json req;
    req["type"] = QuantLabs::CmdType::ConditionOrderQuery;
    emit orderSent(QString::fromStdString(req.dump()));
}

void OrderController::queryStrategies() {
    nlohmann::json req;
    req["type"] = QuantLabs::CmdType::StrategyQuery;
    emit orderSent(QString::fromStdString(req.dump()));
}

void OrderController::modifyConditionOrder(const QString& requestId, double triggerPrice, double limitPrice, int volume) {
    nlohmann::json req;
    req["type"] = QuantLabs::CmdType::ConditionOrderModify;
    req["data"]["request_id"] = requestId.toULongLong();
    req["data"]["trigger_price"] = triggerPrice;
    req["data"]["limit_price"] = limitPrice;
    req["data"]["volume"] = volume;
    
    qDebug() << "[OrderController] Modifying Condition Order:" << requestId 
             << "trigger=" << triggerPrice << "limit=" << limitPrice << "vol=" << volume;
    
    emit orderSent(QString::fromStdString(req.dump()));
}

void OrderController::onConditionOrderReturn(const QJsonObject& j) {
    try {
        std::string type;
        if (j.contains("type")) type = j["type"].toString().toStdString();

        // Handle Strategy List Response
        if (type == QuantLabs::CmdType::RtnStrategyList) {
            if (j.contains("data") && j["data"].isArray()) {
                _strategyList.clear();
                
                QVariantMap empty;
                empty["id"] = "";
                empty["name"] = "无策略 (None)";
                _strategyList.append(empty);

                QJsonArray arr = j["data"].toArray();
                for (const auto& val : arr) {
                    QJsonObject item = val.toObject();
                    QVariantMap map;
                    map["id"] = item["id"].toString();
                    map["name"] = item["name"].toString();
                    _strategyList.append(map);
                }
                emit strategyListChanged();
                qDebug() << "[OrderController] Loaded " << _strategyList.size() << " strategies.";
            }
            return;
        }

        // Case 1: List Response (from condition order query)
        if (j.contains("data") && j["data"].isArray()) {
            _conditionOrderList.clear();
            QJsonArray arr = j["data"].toArray();
            // 倒序插入
            for (int i = arr.size() - 1; i >= 0; --i) {
                QJsonObject item = arr[i].toObject();
                QVariantMap map;
                // Use toVariant().toULongLong() for 64-bit int
                map["request_id"] = QString::number(item["request_id"].toVariant().toULongLong());
                map["instrument_id"] = item["instrument_id"].toString();
                map["trigger_price"] = item["trigger_price"].toDouble();
                map["compare_type"] = item["compare_type"].toInt();
                map["status"] = item["status"].toInt();
                map["direction"] = item["direction"].toString();
                map["offset_flag"] = item["offset_flag"].toString();
                map["volume"] = item["volume"].toInt();
                map["limit_price"] = item["limit_price"].toDouble();
                map["strategy_id"] = item["strategy_id"].toString();
                _conditionOrderList.append(map);
            }
            emit conditionOrderListChanged();
            qDebug() << "[OrderController] Loaded " << _conditionOrderList.size() << " condition orders.";
            return;
        }

        // Case 2: Single Push (from RtnConditionOrder)
        QJsonObject data;
        if (j.contains("data")) data = j["data"].toObject();
        else data = j;
        
        uint64_t reqId = data["request_id"].toVariant().toULongLong();
        QString reqIdStr = QString::number(reqId);
        int newStatus = data["status"].toInt();
        
        // Check if exists
        bool found = false;
        for (int i=0; i<_conditionOrderList.size(); ++i) {
            QVariantMap map = _conditionOrderList[i].toMap();
            if (map["request_id"].toString() == reqIdStr) {
                int oldStatus = map["status"].toInt();
                map["status"] = newStatus;
                
                if (data.contains("trigger_price")) map["trigger_price"] = data["trigger_price"].toDouble();
                if (data.contains("limit_price")) map["limit_price"] = data["limit_price"].toDouble();
                if (data.contains("volume")) map["volume"] = data["volume"].toInt();
                
                _conditionOrderList[i] = map;
                found = true;
                
                if (oldStatus != newStatus) {
                    if (newStatus == 1) { // 触发
                        emit conditionOrderSound("triggered");
                        qDebug() << "[OrderController] Condition Order Triggered:" << reqIdStr;
                    } else if (newStatus == 2) { // 取消
                        emit conditionOrderSound("cancelled");
                        qDebug() << "[OrderController] Condition Order Cancelled:" << reqIdStr;
                    }
                }
                break;
            }
        }
        
        if (!found) { 
            QVariantMap map;
            map["request_id"] = reqIdStr;
            map["instrument_id"] = data["instrument_id"].toString();
            map["trigger_price"] = data["trigger_price"].toDouble();
            map["compare_type"] = data["compare_type"].toInt();
            map["status"] = data["status"].toInt();
            map["direction"] = data["direction"].toString();
            map["offset_flag"] = data["offset_flag"].toString();
            map["volume"] = data["volume"].toInt();
            map["limit_price"] = data["limit_price"].toDouble();
            map["strategy_id"] = data["strategy_id"].toString();
            _conditionOrderList.prepend(map);
        }
        
        emit conditionOrderListChanged();
        
    } catch (const std::exception& e) {
        qDebug() << "[OrderController] Error:" << e.what();
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
        req["type"] = QuantLabs::CmdType::ConditionOrderInsert; 
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

} // namespace QuantLabs
