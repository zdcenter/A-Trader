#include "models/PositionModel.h"
#include <nlohmann/json.hpp>
#include <QDebug>
#include <cstring>

namespace atrad {

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
    roles[AvgPriceRole] = "avgPrice";
    return roles;
}

void PositionModel::updatePosition(const QString& json) {
    try {
        auto j = nlohmann::json::parse(json.toStdString());
        QString id;
        if (j.contains("instrument_id")) id = QString::fromStdString(j["instrument_id"]);
        else if (j.contains("id")) id = QString::fromStdString(j["id"]);
        else return;

        // Parse direction
        char dir = '0';
        if (j.contains("direction")) {
            if (j["direction"].is_string()) dir = j["direction"].get<std::string>()[0];
            else dir = (char)j["direction"].get<int>();
        } else if (j.contains("dir")) {
             if (j["dir"].is_string()) dir = j["dir"].get<std::string>()[0];
             else dir = (char)j["dir"].get<int>();
        }
        
        int row = -1;
        // Search by ID and Direction
        for(int i=0; i<_position_data.size(); ++i) {
            if(_position_data[i].instrumentId == id && _position_data[i].data.direction == dir) {
                row = i;
                break;
            }
        }

        if (row != -1) {
            // Update existing
            auto& d = _position_data[row].data;
            if(j.contains("position")) d.position = j["position"]; else d.position = j["pos"];
            if(j.contains("today_position")) d.today_position = j["today_position"]; else d.today_position = j["td"];
            if(j.contains("yd_position")) d.yd_position = j["yd_position"]; else d.yd_position = j["yd"];
            if(j.contains("position_cost")) d.position_cost = j["position_cost"]; else d.position_cost = j["cost"];
            
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
            if(j.contains("position")) pos = j["position"]; else pos = j["pos"];
            
            if (pos > 0) {
                beginInsertRows(QModelIndex(), _position_data.count(), _position_data.count());
                
                PositionItem item;
                // ... (rest of insert logic)
                std::memset(&item.data, 0, sizeof(item.data));
                item.instrumentId = id;
                strncpy(item.data.instrument_id, id.toStdString().c_str(), sizeof(item.data.instrument_id)-1);
                item.data.direction = dir;
                item.data.position = pos;
                
                if(j.contains("today_position")) item.data.today_position = j["today_position"]; else item.data.today_position = j["td"];
                if(j.contains("yd_position")) item.data.yd_position = j["yd_position"]; else item.data.yd_position = j["yd"];
                if(j.contains("position_cost")) item.data.position_cost = j["position_cost"]; else item.data.position_cost = j["cost"];
                
                item.lastPrice = 0.0;
                item.profit = 0.0;
                
                _position_data.append(item);
                // Re-add to index (actually append is safe, but rebuilding is safer if we mix logic)
                 _instrument_to_indices[id].append(_position_data.count() - 1);
                endInsertRows();
            }
        }
    } catch (...) {}
}

void PositionModel::updatePrice(const QString& json) {
    try {
        auto j = nlohmann::json::parse(json.toStdString());
        QString id;
        if (j.contains("instrument_id")) id = QString::fromStdString(j["instrument_id"]);
        else if (j.contains("id")) id = QString::fromStdString(j["id"]);
        else return;

        double lastPrice = 0.0;
        if (j.contains("last_price")) lastPrice = j["last_price"];
        else if (j.contains("price")) lastPrice = j["price"]; 
        else return;

        if (!_instrument_to_indices.contains(id)) return;

        for (int row : _instrument_to_indices[id]) {
            _position_data[row].lastPrice = lastPrice;
            calculateProfit(_position_data[row]);
            emit dataChanged(index(row), index(row), {ProfitRole, CostRole, LastPriceRole});
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

void PositionModel::updateInstrument(const QString& json) {
    try {
        auto j = nlohmann::json::parse(json.toStdString());
        QString id;
        if (j.contains("instrument_id")) id = QString::fromStdString(j["instrument_id"]);
        else return;
        
        InstrumentData info;
        std::memset(&info, 0, sizeof(info));
        
        // Basic fields
        strncpy(info.instrument_id, id.toStdString().c_str(), sizeof(info.instrument_id)-1);
        if(j.contains("instrument_name")) {
             std::string name = j["instrument_name"];
             strncpy(info.instrument_name, name.c_str(), sizeof(info.instrument_name)-1);
        }

        if(j.contains("volume_multiple")) info.volume_multiple = j["volume_multiple"];
        if(j.contains("price_tick")) info.price_tick = j["price_tick"];
        
        // Margins
        if(j.contains("long_margin_ratio_by_money")) info.long_margin_ratio_by_money = j["long_margin_ratio_by_money"];
        if(j.contains("short_margin_ratio_by_money")) info.short_margin_ratio_by_money = j["short_margin_ratio_by_money"];
        
        // Fees
        if(j.contains("open_ratio_by_money")) info.open_ratio_by_money = j["open_ratio_by_money"];
        if(j.contains("close_ratio_by_money")) info.close_ratio_by_money = j["close_ratio_by_money"];
        if(j.contains("close_today_ratio_by_money")) info.close_today_ratio_by_money = j["close_today_ratio_by_money"];

        _instrument_dict[id] = info;
        
        qDebug() << "[PositionModel] Full Instrument Dict Sync:" << id 
                 << "Mult:" << info.volume_multiple 
                 << "Margin(L):" << info.long_margin_ratio_by_money;

        if (_instrument_to_indices.contains(id)) {
            for (int row : _instrument_to_indices[id]) {
                calculateProfit(_position_data[row]);
                emit dataChanged(index(row), index(row));
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

} // namespace atrad
