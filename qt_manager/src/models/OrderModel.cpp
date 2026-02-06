#include "models/OrderModel.h"

#include <QDebug>
#include <QThread>
#include <QMetaObject>

namespace QuantLabs {

OrderModel::OrderModel(QObject *parent) : QAbstractListModel(parent) {}

int OrderModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    // Mutex not needed if always on UI thread, but for safety against bugs
    // std::lock_guard<std::mutex> lock(_mutex); 
    return static_cast<int>(_orders.size());
}

QVariant OrderModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();
    
    // Safety check for bounds
    if (index.row() < 0 || index.row() >= static_cast<int>(_orders.size())) return QVariant();

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
    roles[OrderRefRole] = "orderRef"; 
    roles[ExchangeIdRole] = "exchangeId"; 
    roles[FrontIdRole] = "frontId"; 
    roles[SessionIdRole] = "sessionId"; 
    return roles;
}

void OrderModel::onOrderReceived(const QJsonObject& j) {
    // 1. Thread Safety: Ensure we run on Main Thread (UI Thread)
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, "onOrderReceived", Qt::QueuedConnection, Q_ARG(QJsonObject, j));
        return;
    }

    std::lock_guard<std::mutex> lock(_mutex);

    try {
        OrderItem item;
        item.instrument_id = j["instrument_id"].toString();
        item.order_sys_id = j["order_sys_id"].toString().trimmed(); 
        item.order_ref = j["order_ref"].toString();
        item.direction = j["direction"].toString();
        item.offset_flag = j["comb_offset_flag"].toString(); 
        
        item.limit_price = j["limit_price"].toDouble();
        item.volume_total_original = j["volume_total_original"].toInt();
        item.volume_traded = j["volume_traded"].toInt();
        item.volume_total = j["volume_total"].toInt();
        
        // Trim Status to avoid "0 " mismatch
        item.order_status = j["order_status"].toString().trimmed();
        
        item.status_msg = j["status_msg"].toString();
        item.insert_time = j["insert_time"].toString();
        
        item.exchange_id = j["exchange_id"].toString();
        item.front_id = j["front_id"].toInt();
        item.session_id = j["session_id"].toInt();
        
        // 调试日志
        qDebug() << "[OrderModel] Received order:"
                 << "instrument_id=" << item.instrument_id
                 << "order_sys_id=" << item.order_sys_id
                 << "order_ref=" << item.order_ref
                 << "status=" << item.order_status;
        
        // 查找是否存在: Pass InstrumentID for accurate matching
        int idx = findOrderIndex(item.order_sys_id, item.order_ref, item.instrument_id);
        
        if (idx >= 0) {
            // Sound Logic: Check for Cancellation
            if (_orders[idx].order_status != "5" && item.order_status == "5") {
                emit orderSoundTriggered("cancel");
            }

            // 更新
            _orders[idx] = item;
            // 通知 View 更新
            emit dataChanged(index(idx), index(idx));
        } else {
            // Sound Logic: New Order
            if (item.order_status == "3" || item.order_status == "1") {
                 emit orderSoundTriggered("success"); 
            } else if (item.order_status == "5") {
                 emit orderSoundTriggered("fail");
            }
            
            // 新增
            beginInsertRows(QModelIndex(), 0, 0);
            _orders.insert(_orders.begin(), item);
            endInsertRows();
        }

    } catch (const std::exception& e) {
        qDebug() << "[OrderModel] Error:" << e.what();
    }
}

int OrderModel::findOrderIndex(const QString& sysId, const QString& ref, const QString& instrumentId) {
    // 优先匹配 SysID，其次 Ref (因为报单初期没有 SysID)
    for (int i = 0; i < static_cast<int>(_orders.size()); ++i) {
        const auto& o = _orders[i];
        
        // Strict Match: SysID must match AND Instrument must match (avoid cross-instrument collision)
        if (!sysId.isEmpty() && !o.order_sys_id.isEmpty()) {
            if (o.order_sys_id == sysId && o.instrument_id == instrumentId) return i;
            // Also relaxed match if instrument missing? No, InstrumentID is critical.
        }
        
        // Fallback: Match by Ref if SysID unavailable (AND Instrument same)
        // Ref is unique per Session+Front. Instrument check adds safety.
        if (o.order_ref == ref && !ref.isEmpty() && o.instrument_id == instrumentId) return i;
    }
    return -1;
}

} // namespace QuantLabs
