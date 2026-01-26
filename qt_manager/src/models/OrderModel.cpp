#include "models/OrderModel.h"
#include <nlohmann/json.hpp>
#include <QDebug>

namespace atrad {

OrderModel::OrderModel(QObject *parent) : QAbstractListModel(parent) {}

int OrderModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(_orders.size());
}

QVariant OrderModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= _orders.size()) return QVariant();

    const auto& item = _orders[index.row()];

    switch (role) {
        case InstrumentIdRole: return item.instrument_id;
        case OrderSysIdRole: return item.order_sys_id;
        case DirectionRole: return item.direction;
        case OffsetFlagRole: return item.offset_flag;
        case PriceRole: return item.limit_price;
        case VolumeOriginalRole: return item.volume_total_original;
        case VolumeTradedRole: return item.volume_traded;
        case VolumeTotalRole: return item.volume_total;
        case StatusRole: return item.order_status;
        case StatusMsgRole: return item.status_msg;
        case TimeRole: return item.insert_time;
        case OrderRefRole: return item.order_ref;
        case ExchangeIdRole: return item.exchange_id;
        case FrontIdRole: return item.front_id;
        case SessionIdRole: return item.session_id;
        default: return QVariant();
    }
}

QHash<int, QByteArray> OrderModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[InstrumentIdRole] = "instrumentId";
    roles[OrderSysIdRole] = "orderSysId";
    roles[DirectionRole] = "direction";
    roles[OffsetFlagRole] = "offsetFlag";
    roles[PriceRole] = "price";
    roles[VolumeOriginalRole] = "volumeOriginal";
    roles[VolumeTradedRole] = "volumeTraded";
    roles[VolumeTotalRole] = "volumeTotal";
    roles[StatusRole] = "status";
    roles[StatusMsgRole] = "statusMsg";
    roles[TimeRole] = "time";
    roles[OrderRefRole] = "orderRef"; // Added
    roles[ExchangeIdRole] = "exchangeId"; // Added
    roles[FrontIdRole] = "frontId"; // Added
    roles[SessionIdRole] = "sessionId"; // Added
    return roles;
}

void OrderModel::onOrderReceived(const QString& json) {
    try {
        auto j = nlohmann::json::parse(json.toStdString());
        
        OrderItem item;
        item.instrument_id = QString::fromStdString(j.value("instrument_id", ""));
        item.order_sys_id = QString::fromStdString(j.value("order_sys_id", ""));
        item.order_ref = QString::fromStdString(j.value("order_ref", ""));
        item.direction = QString::fromStdString(j.value("direction", ""));
        item.offset_flag = QString::fromStdString(j.value("comb_offset_flag", "")); // Core 发送的是 comb_offset_flag
        item.limit_price = j.value("limit_price", 0.0);
        item.volume_total_original = j.value("volume_total_original", 0);
        item.volume_traded = j.value("volume_traded", 0);
        item.volume_total = j.value("volume_total", 0);
        item.order_status = QString::fromStdString(j.value("order_status", ""));
        item.status_msg = QString::fromStdString(j.value("status_msg", ""));
        item.insert_time = QString::fromStdString(j.value("insert_time", ""));
        
        item.exchange_id = QString::fromStdString(j.value("exchange_id", ""));
        item.front_id = j.value("front_id", 0);
        item.session_id = j.value("session_id", 0);
        
        // 查找是否存在
        int idx = findOrderIndex(item.order_sys_id, item.order_ref);
        
        std::lock_guard<std::mutex> lock(_mutex);
        
        if (idx >= 0) {
            // 更新
            _orders[idx] = item;
            // 简单起见，全部刷新改行。其实可以只 emit dataChanged
            emit dataChanged(index(idx), index(idx));
        } else {
            // 新增 (CTP 报单通常不仅是追加，也可能是旧单的状态更新。如果找不到就插入最前)
            beginInsertRows(QModelIndex(), 0, 0);
            _orders.insert(_orders.begin(), item);
            endInsertRows();
        }

    } catch (const std::exception& e) {
        qDebug() << "[OrderModel] JSON Parse Error:" << e.what();
    }
}

int OrderModel::findOrderIndex(const QString& sysId, const QString& ref) {
    // 优先匹配 SysID，其次 Ref (因为报单初期没有 SysID)
    // 注意：OrderRef 是前端生成的序列号，结合 SessionID/FrontID 唯一。
    // 这里简单实现：倒序查找，匹配 Ref 即可 (单Session情况)
    for (int i = 0; i < _orders.size(); ++i) {
        const auto& o = _orders[i];
        if (!sysId.isEmpty() && !o.order_sys_id.isEmpty()) {
            if (o.order_sys_id == sysId && o.instrument_id == _orders[i].instrument_id) return i;
        }
        // 如果没有 SysID，尝试用 Ref 匹配 (且 Instrument 相同)
         if (o.order_ref == ref && !ref.isEmpty()) return i;
    }
    return -1;
}

} // namespace atrad
