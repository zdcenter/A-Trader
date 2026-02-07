#include "models/PositionModel.h"

#include <QDebug>
#include <cstring>

namespace QuantLabs {

PositionModel::PositionModel(QObject *parent) : QAbstractListModel(parent) {}

int PositionModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return _position_data.count();
}

QVariant PositionModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= _position_data.count())
        return QVariant();

    const auto &item = _position_data.at(index.row());
    switch (role) {
        case IdRole: return item.instrumentId;
        // CTP Direction: '0'/'2' Buy, '1'/'3' Sell. Usually '2'/'3' for net pos.
        case DirectionRole: return (item.data.direction == '2' || item.data.direction == '0') ? "BUY" : "SELL";
        case PosRole: return item.data.position;
        case TodayPosRole: return item.data.today_position;
        case YdPosRole: return item.data.yd_position;
        case CostRole: return item.data.position_cost;
        case ProfitRole: return QString::number(item.profit, 'f', 2);
        case LastPriceRole: return item.lastPrice;
        case BidPrice1Role: return item.bidPrice1;
        case AskPrice1Role: return item.askPrice1;
        case PriceTickRole: return item.priceTick;
        case UpperLimitRole: return item.upperLimit;
        case LowerLimitRole: return item.lowerLimit;
        case AvgPriceRole: {
             double multiplier = 10.0;
             if (_instrument_dict.contains(item.instrumentId)) {
                 multiplier = _instrument_dict[item.instrumentId].volume_multiple;
                 if (multiplier < 1) multiplier = 10.0; 
             } else {
                 if (item.instrumentId.startsWith("rb")) multiplier = 10.0;
             }
             if (item.data.position > 0) {
                return QString::number(item.data.position_cost / (item.data.position * multiplier), 'f', 2);
             }
             return "0.00";
        }
        case ExchangeRole: {
             QString exId = QString::fromUtf8(item.data.exchange_id);
             if (!exId.isEmpty()) return exId;
             
             if (_instrument_dict.contains(item.instrumentId)) {
                 return QString::fromUtf8(_instrument_dict[item.instrumentId].exchange_id);
             }
             return "";
        }
        default: return QVariant();
    }
}

QHash<int, QByteArray> PositionModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "instrumentId";
    roles[DirectionRole] = "direction";
    roles[PosRole] = "position";
    roles[TodayPosRole] = "todayPosition";
    roles[YdPosRole] = "ydPosition";
    roles[CostRole] = "cost";
    roles[ProfitRole] = "profit";
    roles[LastPriceRole] = "lastPrice";
    roles[BidPrice1Role] = "bidPrice1";
    roles[AskPrice1Role] = "askPrice1";
    roles[PriceTickRole] = "priceTick";
    roles[UpperLimitRole] = "upperLimit";
    roles[LowerLimitRole] = "lowerLimit";
    roles[AvgPriceRole] = "avgPrice";
    roles[ExchangeRole] = "exchangeId";
    return roles;
}

void PositionModel::updatePosition(const QJsonObject& j) {
    try {
        QString id;
        if (j.contains("instrument_id")) id = j["instrument_id"].toString();
        else if (j.contains("id")) id = j["id"].toString();
        else return;

        // 检查是否为全量快照
        bool is_snapshot = false;
        if (j.contains("is_snapshot")) {
            is_snapshot = j["is_snapshot"].toBool();
        }

        // Parse direction
        char dir = '0';
        if (j.contains("direction")) {
            QJsonValue v = j["direction"];
            if (v.isString()) dir = v.toString().toStdString()[0];
            else dir = (char)v.toInt();
        } else if (j.contains("dir")) {
             QJsonValue v = j["dir"];
             if (v.isString()) dir = v.toString().toStdString()[0];
             else dir = (char)v.toInt();
        }
        
        // 检查快照批次号
        int64_t snapshot_seq = 0;
        if (j.contains("snapshot_seq")) {
            // QJsonValue doesn't have toLongLong, use toDouble or toVariant
            snapshot_seq = (int64_t)j["snapshot_seq"].toDouble();
        } else if (j.contains("is_snapshot")) {
             bool is_ss = j["is_snapshot"].toBool();
             if (is_ss) snapshot_seq = 1; 
        }

        // 如果是全量快照 (seq > 0)，且批次号发生变更，清空旧数据
        static int64_t last_snapshot_seq = 0;
        if (snapshot_seq > 0 && snapshot_seq != last_snapshot_seq) {
            qDebug() << "[PositionModel] New snapshot batch detected (Seq:" << snapshot_seq << "), clearing old data...";
            beginResetModel();
            _position_data.clear();
            _instrument_to_indices.clear();
            endResetModel();
            last_snapshot_seq = snapshot_seq;
        }
        
        int row = -1;
        // Search by ID and Direction
        // Optimization: Use _instrument_to_indices to narrow down search
        if (_instrument_to_indices.contains(id)) {
            const auto& indices = _instrument_to_indices[id];
            for (int idx : indices) {
                if (idx >= 0 && idx < _position_data.size()) {
                    if (_position_data[idx].data.direction == dir) {
                        row = idx;
                        break;
                    }
                }
            }
        }

        if (row != -1) {
            // Update existing
            auto& d = _position_data[row].data;
            if(j.contains("position")) d.position = j["position"].toInt(); else if(j.contains("pos")) d.position = j["pos"].toInt();
            if(j.contains("today_position")) d.today_position = j["today_position"].toInt(); else if(j.contains("td")) d.today_position = j["td"].toInt();
            if(j.contains("yd_position")) d.yd_position = j["yd_position"].toInt(); else if(j.contains("yd")) d.yd_position = j["yd"].toInt();
            if(j.contains("position_cost")) d.position_cost = j["position_cost"].toDouble(); else if(j.contains("cost")) d.position_cost = j["cost"].toDouble();
            if(j.contains("exchange_id")) {
                std::string s = j["exchange_id"].toString().toStdString();
                strncpy(d.exchange_id, s.c_str(), sizeof(d.exchange_id)-1);
            }
            
            // 如果持仓量 <= 0，从列表中删除
            if (d.position <= 0) {
                 beginRemoveRows(QModelIndex(), row, row);
                 _position_data.removeAt(row);
                 endRemoveRows();
                 
                 // 重建索引映射 (因为删除中间行会导致后续行号变化)
                 _instrument_to_indices.clear();
                 for(int i=0; i<_position_data.size(); ++i) {
                     _instrument_to_indices[QString::fromUtf8(_position_data[i].data.instrument_id)].append(i);
                 }
            } else {
                 calculateProfit(_position_data[row]);
                 emit dataChanged(index(row), index(row));
            }
        } else {
            // Insert new (only if pos > 0)
            int pos = 0;
            if(j.contains("position")) pos = j["position"].toInt(); else if(j.contains("pos")) pos = j["pos"].toInt();
            
            if (pos > 0) {
                beginInsertRows(QModelIndex(), _position_data.count(), _position_data.count());
                
                PositionItem item;
                std::memset(&item.data, 0, sizeof(item.data));
                item.instrumentId = id;
                strncpy(item.data.instrument_id, id.toStdString().c_str(), sizeof(item.data.instrument_id)-1);
                item.data.direction = dir;
                item.data.position = pos;
                
                if(j.contains("today_position")) item.data.today_position = j["today_position"].toInt(); else if(j.contains("td")) item.data.today_position = j["td"].toInt();
                if(j.contains("yd_position")) item.data.yd_position = j["yd_position"].toInt(); else if(j.contains("yd")) item.data.yd_position = j["yd"].toInt();
                if(j.contains("position_cost")) item.data.position_cost = j["position_cost"].toDouble(); else if(j.contains("cost")) item.data.position_cost = j["cost"].toDouble();
                if(j.contains("exchange_id")) {
                    std::string s = j["exchange_id"].toString().toStdString();
                    strncpy(item.data.exchange_id, s.c_str(), sizeof(item.data.exchange_id)-1);
                }
                
                item.lastPrice = 0.0;
                item.bidPrice1 = 0.0;
                item.askPrice1 = 0.0;
                item.priceTick = 0.0;
                item.upperLimit = 0.0;
                item.lowerLimit = 0.0;
                item.profit = 0.0;
                
                _position_data.append(item);
                _instrument_to_indices[id].append(_position_data.count() - 1);
                endInsertRows();
            }
        }
    } catch (...) {}
}

void PositionModel::updatePrice(const QJsonObject& j) {
    try {
        QString id;
        if (j.contains("instrument_id")) id = j["instrument_id"].toString();
        else if (j.contains("id")) id = j["id"].toString();
        else return;

        double lastPrice = 0.0;
        if (j.contains("last_price")) lastPrice = j["last_price"].toDouble();
        else if (j.contains("price")) lastPrice = j["price"].toDouble(); 
        else return;
        
        double bidPrice1 = 0.0;
        double askPrice1 = 0.0;
        if (j.contains("bid_price1")) bidPrice1 = j["bid_price1"].toDouble();
        if (j.contains("ask_price1")) askPrice1 = j["ask_price1"].toDouble();
        
        double upperLimit = 0.0;
        double lowerLimit = 0.0;
        if (j.contains("upper_limit_price")) upperLimit = j["upper_limit_price"].toDouble();
        if (j.contains("lower_limit_price")) lowerLimit = j["lower_limit_price"].toDouble();
        
        if (!_instrument_to_indices.contains(id)) return;

        for (int row : _instrument_to_indices[id]) {
            if (row < 0 || row >= _position_data.size()) continue;
            
            _position_data[row].lastPrice = lastPrice;
            _position_data[row].bidPrice1 = bidPrice1;
            _position_data[row].askPrice1 = askPrice1;
            _position_data[row].upperLimit = upperLimit;
            _position_data[row].lowerLimit = lowerLimit;
            
            // 从合约字典获取 priceTick
            if (_instrument_dict.contains(id)) {
                _position_data[row].priceTick = _instrument_dict[id].price_tick;
            }
            
            calculateProfit(_position_data[row]);
            emit dataChanged(index(row), index(row), {ProfitRole, CostRole, LastPriceRole, BidPrice1Role, AskPrice1Role, PriceTickRole, UpperLimitRole, LowerLimitRole});
        }

        double current_all_profit = 0.0;
        for (const auto& item : _position_data) {
            current_all_profit += item.profit;
        }
        
        if (std::abs(current_all_profit - _total_profit) > 0.01) {
            _total_profit = current_all_profit;
            emit totalProfitChanged(_total_profit);
        }
    } catch (...) {}
}

void PositionModel::updatePriceBinary(const TickData& data) {
    QString id = QString::fromLatin1(data.instrument_id);
    if (!_instrument_to_indices.contains(id)) return;
    
    for (int row : _instrument_to_indices[id]) {
        if (row < 0 || row >= _position_data.size()) continue;
        
        PositionItem& item = _position_data[row];
        item.lastPrice = data.last_price;
        item.bidPrice1 = data.bid_price1;
        item.askPrice1 = data.ask_price1;
        item.upperLimit = data.upper_limit_price;
        item.lowerLimit = data.lower_limit_price;
        
        if (_instrument_dict.contains(id)) {
            item.priceTick = _instrument_dict[id].price_tick;
        }
        
        calculateProfit(item);
        emit dataChanged(index(row), index(row), {ProfitRole, CostRole, LastPriceRole, BidPrice1Role, AskPrice1Role, PriceTickRole, UpperLimitRole, LowerLimitRole});
    }

    double newTotal = 0.0;
    for (const auto& item : _position_data) {
        newTotal += item.profit;
    }
    
    if (std::abs(newTotal - _total_profit) > 0.01) {
        _total_profit = newTotal;
        emit totalProfitChanged(_total_profit);
    }
}

void PositionModel::updateInstrument(const QJsonObject& j) {
    try {
        QString id;
        if (j.contains("instrument_id")) id = j["instrument_id"].toString();
        else return;
        
        InstrumentData info;
        std::memset(&info, 0, sizeof(info));
        
        // Basic fields
        strncpy(info.instrument_id, id.toStdString().c_str(), sizeof(info.instrument_id)-1);
        if(j.contains("instrument_name")) {
             std::string name = j["instrument_name"].toString().toStdString();
             strncpy(info.instrument_name, name.c_str(), sizeof(info.instrument_name)-1);
        }
        if(j.contains("exchange_id")) {
             std::string ex = j["exchange_id"].toString().toStdString();
             strncpy(info.exchange_id, ex.c_str(), sizeof(info.exchange_id)-1);
        }

        if(j.contains("volume_multiple")) info.volume_multiple = j["volume_multiple"].toInt();
        if(j.contains("price_tick")) info.price_tick = j["price_tick"].toDouble();
        
        // Margins
        if(j.contains("long_margin_ratio_by_money")) info.long_margin_ratio_by_money = j["long_margin_ratio_by_money"].toDouble();
        if(j.contains("short_margin_ratio_by_money")) info.short_margin_ratio_by_money = j["short_margin_ratio_by_money"].toDouble();
        
        // Fees
        if(j.contains("open_ratio_by_money")) info.open_ratio_by_money = j["open_ratio_by_money"].toDouble();
        if(j.contains("close_ratio_by_money")) info.close_ratio_by_money = j["close_ratio_by_money"].toDouble();
        if(j.contains("close_today_ratio_by_money")) info.close_today_ratio_by_money = j["close_today_ratio_by_money"].toDouble();

        _instrument_dict[id] = info;
        
        qDebug() << "[PositionModel] Full Instrument Dict Sync:" << id 
                 << "Mult:" << info.volume_multiple 
                 << "Margin(L):" << info.long_margin_ratio_by_money;

        if (_instrument_to_indices.contains(id)) {
            for (int row : _instrument_to_indices[id]) {
                if (row >= 0 && row < _position_data.size()) {
                    calculateProfit(_position_data[row]);
                    emit dataChanged(index(row), index(row));
                }
            }
        }
    } catch (...) {}
}

void PositionModel::calculateProfit(PositionItem& item) {
    if (item.lastPrice <= 0.001) return;

    double multiplier = 10.0; 
    if (_instrument_dict.contains(item.instrumentId)) {
        multiplier = _instrument_dict[item.instrumentId].volume_multiple;
        if (multiplier < 1) multiplier = 10.0; // protection
    } else {
        // Fallback guess logic
        if (item.instrumentId.startsWith("rb")) multiplier = 10.0;
        // ... add more if needed or just wait for instrument dict
    }

    double current_value = item.lastPrice * item.data.position * multiplier;
    
    // Cost in CTP is usually total cost.
    if (item.data.direction == '2' || item.data.direction == '0') { // Buy
        item.profit = current_value - item.data.position_cost;
    } else { // Sell
        item.profit = item.data.position_cost - current_value;
    }
}

} // namespace QuantLabs
