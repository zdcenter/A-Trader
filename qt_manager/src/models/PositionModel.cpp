#include "models/PositionModel.h"

#include <QDebug>
#include <cstring>
#include <cmath>

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
        case DirectionRole: return (item.data.direction == '2' || item.data.direction == '0') ? "BUY" : "SELL";
        case PosRole: return item.data.position;
        case TodayPosRole: return item.data.today_position;
        case YdPosRole: return item.data.yd_position;
        case AvgPriceRole: {
            // 均价 = open_cost / (position × volume_multiple)
            // volume_multiple 优先用 core 推送的值，其次查合约字典
            int mult = item.data.volume_multiple;
            if (mult <= 0 && _instrument_dict.contains(item.instrumentId)) {
                mult = _instrument_dict[item.instrumentId].volume_multiple;
            }
            if (mult <= 0) mult = 1;  // 兜底
            
            if (item.data.position > 0 && item.data.open_cost > 0) {
                return QString::number(item.data.open_cost / (item.data.position * mult), 'f', 2);
            }
            return "0.00";
        }
        case LastPriceRole: return item.lastPrice;
        case PosProfitRole: return QString::number(item.data.pos_profit, 'f', 2);
        case CloseProfitRole: return QString::number(item.data.close_profit, 'f', 2);
        case MarginRole: return QString::number(item.data.margin, 'f', 2);
        case BidPrice1Role: return item.bidPrice1;
        case AskPrice1Role: return item.askPrice1;
        case PriceTickRole: return item.priceTick;
        case UpperLimitRole: return item.upperLimit;
        case LowerLimitRole: return item.lowerLimit;
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
    roles[AvgPriceRole] = "avgPrice";
    roles[LastPriceRole] = "lastPrice";
    roles[PosProfitRole] = "posProfit";
    roles[CloseProfitRole] = "closeProfit";
    roles[MarginRole] = "margin";
    roles[BidPrice1Role] = "bidPrice1";
    roles[AskPrice1Role] = "askPrice1";
    roles[PriceTickRole] = "priceTick";
    roles[UpperLimitRole] = "upperLimit";
    roles[LowerLimitRole] = "lowerLimit";
    roles[ExchangeRole] = "exchangeId";
    return roles;
}

void PositionModel::updatePosition(const QJsonObject& j) {
    try {
        QString id;
        if (j.contains("instrument_id")) id = j["instrument_id"].toString();
        else if (j.contains("id")) id = j["id"].toString();
        else return;

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
            snapshot_seq = (int64_t)j["snapshot_seq"].toDouble();
        } else if (j.contains("is_snapshot")) {
             if (j["is_snapshot"].toBool()) snapshot_seq = 1; 
        }

        // 全量快照时清空旧数据
        static int64_t last_snapshot_seq = 0;
        if (snapshot_seq > 0 && snapshot_seq != last_snapshot_seq) {
            beginResetModel();
            _position_data.clear();
            _instrument_to_indices.clear();
            endResetModel();
            last_snapshot_seq = snapshot_seq;
        }
        
        // 查找已存在的行
        int row = -1;
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
            // 更新已有行
            auto& d = _position_data[row].data;
            if(j.contains("position")) d.position = j["position"].toInt();
            if(j.contains("today_position")) d.today_position = j["today_position"].toInt();
            if(j.contains("yd_position")) d.yd_position = j["yd_position"].toInt();
            if(j.contains("position_cost")) d.position_cost = j["position_cost"].toDouble();
            if(j.contains("open_cost")) d.open_cost = j["open_cost"].toDouble();
            if(j.contains("pos_profit")) d.pos_profit = j["pos_profit"].toDouble();
            if(j.contains("close_profit")) d.close_profit = j["close_profit"].toDouble();
            if(j.contains("margin")) d.margin = j["margin"].toDouble();
            if(j.contains("volume_multiple")) d.volume_multiple = j["volume_multiple"].toInt();
            if(j.contains("exchange_id")) {
                std::string s = j["exchange_id"].toString().toStdString();
                strncpy(d.exchange_id, s.c_str(), sizeof(d.exchange_id)-1);
            }
            
            // 持仓量 <= 0 时删除行
            if (d.position <= 0) {
                 beginRemoveRows(QModelIndex(), row, row);
                 _position_data.removeAt(row);
                 endRemoveRows();
                 
                 // 重建索引
                 _instrument_to_indices.clear();
                 for(int i=0; i<_position_data.size(); ++i) {
                     _instrument_to_indices[QString::fromUtf8(_position_data[i].data.instrument_id)].append(i);
                 }
            } else {
                 emit dataChanged(index(row), index(row));
            }
        } else {
            // 新增行（仅 position > 0 时）
            int pos = 0;
            if(j.contains("position")) pos = j["position"].toInt();
            
            if (pos > 0) {
                beginInsertRows(QModelIndex(), _position_data.count(), _position_data.count());
                
                PositionItem item;
                std::memset(&item.data, 0, sizeof(item.data));
                item.instrumentId = id;
                strncpy(item.data.instrument_id, id.toStdString().c_str(), sizeof(item.data.instrument_id)-1);
                item.data.direction = dir;
                item.data.position = pos;
                
                if(j.contains("today_position")) item.data.today_position = j["today_position"].toInt();
                if(j.contains("yd_position")) item.data.yd_position = j["yd_position"].toInt();
                if(j.contains("position_cost")) item.data.position_cost = j["position_cost"].toDouble();
                if(j.contains("open_cost")) item.data.open_cost = j["open_cost"].toDouble();
                if(j.contains("pos_profit")) item.data.pos_profit = j["pos_profit"].toDouble();
                if(j.contains("close_profit")) item.data.close_profit = j["close_profit"].toDouble();
                if(j.contains("margin")) item.data.margin = j["margin"].toDouble();
                if(j.contains("volume_multiple")) item.data.volume_multiple = j["volume_multiple"].toInt();
                if(j.contains("exchange_id")) {
                    std::string s = j["exchange_id"].toString().toStdString();
                    strncpy(item.data.exchange_id, s.c_str(), sizeof(item.data.exchange_id)-1);
                }
                
                _position_data.append(item);
                _instrument_to_indices[id].append(_position_data.count() - 1);
                endInsertRows();
            }
        }
        
        recalcTotalProfit();
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
            
            if (_instrument_dict.contains(id)) {
                _position_data[row].priceTick = _instrument_dict[id].price_tick;
            }
            
            emit dataChanged(index(row), index(row), 
                {LastPriceRole, BidPrice1Role, AskPrice1Role, PriceTickRole, UpperLimitRole, LowerLimitRole});
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
        
        emit dataChanged(index(row), index(row), 
            {LastPriceRole, BidPrice1Role, AskPrice1Role, PriceTickRole, UpperLimitRole, LowerLimitRole});
    }
}

void PositionModel::updateInstrument(const QJsonObject& j) {
    try {
        QString id;
        if (j.contains("instrument_id")) id = j["instrument_id"].toString();
        else return;
        
        InstrumentMeta info;  // 默认构造已全零初始化
        
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
        
        // 保证金率
        if(j.contains("long_margin_ratio_by_money")) info.long_margin_ratio_by_money = j["long_margin_ratio_by_money"].toDouble();
        if(j.contains("short_margin_ratio_by_money")) info.short_margin_ratio_by_money = j["short_margin_ratio_by_money"].toDouble();
        
        // 手续费率
        if(j.contains("open_ratio_by_money")) info.open_ratio_by_money = j["open_ratio_by_money"].toDouble();
        if(j.contains("close_ratio_by_money")) info.close_ratio_by_money = j["close_ratio_by_money"].toDouble();
        if(j.contains("close_today_ratio_by_money")) info.close_today_ratio_by_money = j["close_today_ratio_by_money"].toDouble();

        _instrument_dict[id] = info;

        // 合约信息更新后触发相关持仓行刷新（均价可能需要 volume_multiple）
        if (_instrument_to_indices.contains(id)) {
            for (int row : _instrument_to_indices[id]) {
                if (row >= 0 && row < _position_data.size()) {
                    emit dataChanged(index(row), index(row));
                }
            }
        }
    } catch (...) {}
}

void PositionModel::recalcTotalProfit() {
    double newTotal = 0.0;
    for (const auto& item : _position_data) {
        newTotal += item.data.pos_profit;
    }
    
    if (std::abs(newTotal - _total_profit) > 0.01) {
        _total_profit = newTotal;
        emit totalProfitChanged(_total_profit);
    }
}

} // namespace QuantLabs
