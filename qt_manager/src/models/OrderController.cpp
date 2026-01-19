#include "models/OrderController.h"
#include <QDebug>

namespace atrad {

OrderController::OrderController(QObject *parent) : QObject(parent) {
    connect(this, &OrderController::orderParamsChanged, this, &OrderController::recalculate);
}

// 新增 onTick 实现
void OrderController::onTick(const QString& json) {
    if (_instrumentId.isEmpty() || _isManualPrice) return;
    
    try {
        auto j = nlohmann::json::parse(json.toStdString());
        std::string id = j["id"];
        if (id == _instrumentId.toStdString()) {
            double lastPrice = 0.0;
            if (j.contains("price")) lastPrice = j["price"];
            else if (j.contains("last_price")) lastPrice = j["last_price"];
            else if (j.contains("lp")) lastPrice = j["lp"];
            
            if (lastPrice > 0.0001 && std::abs(_price - lastPrice) > 0.0001) {
                _price = lastPrice;
                emit orderParamsChanged();
            }
            
            // 解析 5 档行情
            _bidPrices.clear(); _bidVolumes.clear();
            _askPrices.clear(); _askVolumes.clear();
            
            // Bid 1-5
            for(int i=1; i<=5; ++i) {
               std::string pKey = "b" + std::to_string(i);
               std::string vKey = "bv" + std::to_string(i);
               if (j.contains(pKey)) _bidPrices.append(j[pKey].get<double>()); else _bidPrices.append(0.0);
               if (j.contains(vKey)) _bidVolumes.append(j[vKey].get<int>()); else _bidVolumes.append(0);
            }

            // Ask 1-5
            for(int i=1; i<=5; ++i) {
               std::string pKey = "a" + std::to_string(i);
               std::string vKey = "av" + std::to_string(i);
               if (j.contains(pKey)) _askPrices.append(j[pKey].get<double>()); else _askPrices.append(0.0);
               if (j.contains(vKey)) _askVolumes.append(j[vKey].get<int>()); else _askVolumes.append(0);
            }
            
            emit marketDataChanged();
        }
    } catch (...) {}
}

void OrderController::updateInstrument(const QString& json) {
    try {
        auto j = nlohmann::json::parse(json.toStdString());
        QString id = QString::fromStdString(j["id"]);
        
        InstrumentInfo info;
        // 使用 value() 避免键不存在时抛异常
        info.multiple = j.value("mult", 1);
        info.longMarginRatio = j.value("l_m_money", 0.1); // 默认 10% 防止为 0
        info.shortMarginRatio = j.value("s_m_money", 0.1);
        info.openRatioByMoney = j.value("o_r_money", 0.0001);
        info.openRatioByVol = j.value("o_r_vol", 0.0);
        
        qDebug() << "[OrderController] Instrument Updated:" << id 
                 << "Mult:" << info.multiple 
                 << "MarginRatio:" << info.longMarginRatio;

        _instrument_dict[id] = info;
        
        if (id == _instrumentId) {
            recalculate();
        }
    } catch (...) {}
}

void OrderController::recalculate() {
    if (_instrumentId.isEmpty() || !_instrument_dict.contains(_instrumentId)) {
        _estimatedMargin = 0.0;
        _estimatedCommission = 0.0;
        emit calculationChanged();
        return;
    }

    const auto& info = _instrument_dict[_instrumentId];
    
    // 1. 计算预估保证金 = 价格 * 数量 * 乘数 * 保证金率
    // 简单起见，这里假设是多头保证金率
    _estimatedMargin = _price * _volume * info.multiple * info.longMarginRatio;

    // 2. 计算预估手续费
    if (info.openRatioByMoney > 0) {
        _estimatedCommission = _price * _volume * info.multiple * info.openRatioByMoney;
    } else {
        _estimatedCommission = _volume * info.openRatioByVol;
    }

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
