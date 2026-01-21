#include "models/TradeModel.h"
#include <nlohmann/json.hpp>
#include <QDebug>

namespace atrad {

TradeModel::TradeModel(QObject *parent) : QAbstractListModel(parent) {}

int TradeModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(_trades.size());
}

QVariant TradeModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= _trades.size()) return QVariant();

    const auto& item = _trades[index.row()];

    switch (role) {
        case InstrumentIdRole: return item.instrument_id;
        case TradeIdRole: return item.trade_id;
        case OrderSysIdRole: return item.order_sys_id;
        case DirectionRole: return item.direction;
        case OffsetFlagRole: return item.offset_flag;
        case PriceRole: return item.price;
        case VolumeRole: return item.volume;
        case TimeRole: return item.trade_time;
        case DateRole: return item.trade_date;
        default: return QVariant();
    }
}

QHash<int, QByteArray> TradeModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[InstrumentIdRole] = "instrumentId";
    roles[TradeIdRole] = "tradeId";
    roles[OrderSysIdRole] = "orderSysId";
    roles[DirectionRole] = "direction";
    roles[OffsetFlagRole] = "offsetFlag";
    roles[PriceRole] = "price";
    roles[VolumeRole] = "volume";
    roles[TimeRole] = "time";
    roles[DateRole] = "date";
    return roles;
}

void TradeModel::onTradeReceived(const QString& json) {
    try {
        auto j = nlohmann::json::parse(json.toStdString());
        
        TradeItem item;
        item.instrument_id = QString::fromStdString(j.value("instrument_id", ""));
        item.trade_id = QString::fromStdString(j.value("trade_id", ""));
        item.order_sys_id = QString::fromStdString(j.value("order_sys_id", ""));
        item.direction = QString::fromStdString(j.value("direction", ""));
        item.offset_flag = QString::fromStdString(j.value("offset_flag", ""));
        item.price = j.value("price", 0.0);
        item.volume = j.value("volume", 0);
        item.trade_time = QString::fromStdString(j.value("trade_time", ""));
        item.trade_date = QString::fromStdString(j.value("trade_date", ""));
        
        // 查重 (TradeID 唯一)
        bool exists = false;
        for(const auto& t : _trades) {
            if (t.trade_id == item.trade_id && t.instrument_id == item.instrument_id) {
                exists = true; break;
            }
        }
        
        if (!exists) {
            std::lock_guard<std::mutex> lock(_mutex);
            beginInsertRows(QModelIndex(), 0, 0);
            _trades.insert(_trades.begin(), item);
            endInsertRows();
        }

    } catch (const std::exception& e) {
        qDebug() << "[TradeModel] JSON Parse Error:" << e.what();
    }
}

} // namespace atrad
