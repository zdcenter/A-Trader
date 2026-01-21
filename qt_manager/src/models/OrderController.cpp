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
    j["type"] = "ORDER";
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
    j["type"] = "SUBSCRIBE";
    j["id"] = id.toStdString();
    QString jsonStr = QString::fromStdString(j.dump());
    qDebug() << "[OrderController] Emitting orderSent:" << jsonStr;
    emit orderSent(jsonStr);
}

void OrderController::unsubscribe(const QString& id) {
    if (id.isEmpty()) return;
    nlohmann::json j;
    j["type"] = "UNSUBSCRIBE";
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

void OrderController::updateConnectionStatus(bool core, bool ctp) {
    if (_coreConnected != core || _ctpConnected != ctp) {
        _coreConnected = core;
        _ctpConnected = ctp;
        emit connectionChanged();
    }
}



void OrderController::sendCommand(const QString& cmd) {
    emit orderSent(cmd);
}

} // namespace atrad
