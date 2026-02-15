#include "models/TradeModel.h"

#include <QDebug>
#include <QThread>
#include <QVariantList>
#include <QVariantMap>
#include <map>

namespace QuantLabs {

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
        case CommissionRole: return item.commission;
        case CloseProfitRole: return item.close_profit;
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
    roles[CommissionRole] = "commission";
    roles[CloseProfitRole] = "closeProfit";
    roles[TimeRole] = "time";
    roles[DateRole] = "date";
    return roles;
}

void TradeModel::onTradeReceived(const QJsonObject& j) {
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, "onTradeReceived", Qt::QueuedConnection, Q_ARG(QJsonObject, j));
        return;
    }
    
    try {
        TradeItem item;
        item.instrument_id = j["instrument_id"].toString();
        item.trade_id = j["trade_id"].toString();
        item.order_sys_id = j["order_sys_id"].toString();
        item.direction = j["direction"].toString();
        item.offset_flag = j["offset_flag"].toString();
        item.price = j["price"].toDouble();
        item.volume = j["volume"].toInt();
        item.commission = j["commission"].toDouble();
        item.close_profit = j["close_profit"].toDouble();
        item.trade_time = j["trade_time"].toString();
        item.trade_date = j["trade_date"].toString();
        
        // 查重 (TradeID 唯一)
        bool exists = false;
        for(const auto& t : _trades) {
            if (t.trade_id == item.trade_id && t.instrument_id == item.instrument_id) {
                exists = true; break;
            }
        }
        
        if (!exists) {
            std::lock_guard<std::mutex> lock(_mutex);
            
            // 保持按时间倒序排列 (最新的在前面)
            // Comparator: 返回 true 如果 a 应该排在 b 前面 (即 a 比 b 新)
            auto it = std::lower_bound(_trades.begin(), _trades.end(), item, [](const TradeItem& a, const TradeItem& b) {
                int dateCmp = QString::compare(a.trade_date, b.trade_date);
                if (dateCmp > 0) return true;
                if (dateCmp < 0) return false;
                
                int timeCmp = QString::compare(a.trade_time, b.trade_time);
                if (timeCmp > 0) return true;
                if (timeCmp < 0) return false;
                
                // TradeID 降序
                return QString::compare(a.trade_id, b.trade_id) > 0;
            });
            
            int index = std::distance(_trades.begin(), it);
            beginInsertRows(QModelIndex(), index, index);
            _trades.insert(it, item);
            endInsertRows();
            
            emit tradeSoundTriggered();
            emit summaryChanged();
        }

    } catch (const std::exception& e) {
        qDebug() << "[TradeModel] Error:" << e.what();
    }
}

QVariantList TradeModel::getTradeSummary() const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    // 分组键: instrument_id + direction + offset_flag
    struct SummaryGroup {
        double totalAmount = 0.0;  // sum(price * volume)，用于算均价
        int totalVolume = 0;
        double totalCommission = 0.0;
        double totalCloseProfit = 0.0;
    };
    
    // 用 tuple 作为 key：(instrument_id, direction, offset_flag)
    // 但 std::map 需要 string key，所以拼接
    std::map<QString, SummaryGroup> groups;
    // 保持插入顺序，记录首次出现的 key
    std::vector<QString> keyOrder;
    // 额外记录每个 key 对应的 direction 和 offset_flag
    std::map<QString, std::pair<QString, QString>> keyMeta;
    
    for (const auto& t : _trades) {
        // 简化 offset: "1"/"3"/"4" 都归为平仓
        QString offsetGroup = (t.offset_flag == "0") ? "0" : "1";
        QString key = t.instrument_id + "|" + t.direction + "|" + offsetGroup;
        
        auto& g = groups[key];
        if (g.totalVolume == 0) {
            keyOrder.push_back(key);
            keyMeta[key] = {t.direction, offsetGroup};
        }
        g.totalAmount += t.price * t.volume;
        g.totalVolume += t.volume;
        g.totalCommission += t.commission;
        g.totalCloseProfit += t.close_profit;
    }
    
    QVariantList result;
    for (const auto& key : keyOrder) {
        const auto& g = groups[key];
        const auto& meta = keyMeta[key];
        
        // 从 key 中提取 instrument_id
        QString instrumentId = key.section('|', 0, 0);
        
        QVariantMap row;
        row["instrumentId"] = instrumentId;
        row["direction"] = meta.first;
        row["offsetFlag"] = meta.second;
        row["avgPrice"] = (g.totalVolume > 0) ? (g.totalAmount / g.totalVolume) : 0.0;
        row["totalVolume"] = g.totalVolume;
        row["totalCommission"] = g.totalCommission;
        row["totalCloseProfit"] = g.totalCloseProfit;
        result.append(row);
    }
    
    return result;
}

} // namespace QuantLabs
