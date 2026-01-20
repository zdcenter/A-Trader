#include "models/MarketModel.h"
#include <nlohmann/json.hpp>
#include <QDebug>
#include <cstring> // for strncpy, memset

namespace atrad {

MarketModel::MarketModel(QObject *parent)
    : QAbstractListModel(parent) {
}

int MarketModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return _market_data.count();
}

QVariant MarketModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= _market_data.count())
        return QVariant();

    const auto &item = _market_data.at(index.row());
    switch (role) {
        case IdRole: return item.instrumentId;
        case PriceRole: return item.data.last_price;
        case PreCloseRole: return item.preClose;
        case ChangeRole: return item.change;
        case ChangePercentRole: return item.changePercent;
        case VolumeRole: return item.data.volume;
        case OpenInterestRole: return item.data.open_interest;
        case BidPrice1Role: return item.data.bid_price1;
        case BidVolume1Role: return item.data.bid_volume1;
        case AskPrice1Role: return item.data.ask_price1;
        case AskVolume1Role: return item.data.ask_volume1;
        case TimeRole: return item.updateTime;
        case TurnoverRole: return item.data.turnover;
        case UpperLimitRole: return item.data.upper_limit_price;
        case LowerLimitRole: return item.data.lower_limit_price;
        case OpenPriceRole: return item.data.open_price;
        case HighestPriceRole: return item.data.highest_price;
        case LowestPriceRole: return item.data.lowest_price;
        case AveragePriceRole: return item.data.average_price;
        default: return QVariant();
    }
}

QHash<int, QByteArray> MarketModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "instrumentId";
    roles[PriceRole] = "lastPrice";
    roles[PreCloseRole] = "preClose";
    roles[ChangeRole] = "change";
    roles[ChangePercentRole] = "changePercent";
    roles[VolumeRole] = "volume";
    roles[OpenInterestRole] = "openInterest";
    roles[BidPrice1Role] = "bidPrice1";
    roles[BidVolume1Role] = "bidVolume1";
    roles[AskPrice1Role] = "askPrice1";
    roles[AskVolume1Role] = "askVolume1";
    roles[TimeRole] = "updateTime";
    roles[TurnoverRole] = "turnover";
    roles[UpperLimitRole] = "upperLimit";
    roles[LowerLimitRole] = "lowerLimit";
    roles[OpenPriceRole] = "openPrice";
    roles[HighestPriceRole] = "highestPrice";
    roles[LowestPriceRole] = "lowestPrice";
    roles[AveragePriceRole] = "averagePrice";
    return roles;
}

void MarketModel::updateTick(const QString& json) {
    try {
        auto j = nlohmann::json::parse(json.toStdString());
        QString id;
        if(j.contains("instrument_id")) id = QString::fromStdString(j["instrument_id"]);
        else if(j.contains("id")) id = QString::fromStdString(j["id"]);
        
        if (id.isEmpty()) return;

        int row = -1;
        if (_instrument_to_index.contains(id)) {
            row = _instrument_to_index[id];
        } else {
             beginInsertRows(QModelIndex(), _market_data.count(), _market_data.count());
             _instrument_to_index[id] = _market_data.count();
             MarketItem newItem;
             std::memset(&newItem.data, 0, sizeof(TickData));
             newItem.instrumentId = id;
             // also copy to data.instrument_id
             strncpy(newItem.data.instrument_id, id.toStdString().c_str(), sizeof(newItem.data.instrument_id)-1);
             // initialized other UI fields
             newItem.preClose = 0.0; newItem.change = 0.0; newItem.changePercent = 0.0; newItem.updateTime = "--";

             _market_data.append(newItem);
             row = _market_data.count() - 1;
             endInsertRows();
        }
        
        // Ensure row is valid
        if(row < 0 || row >= _market_data.size()) return;
        
        auto& item = _market_data[row];
        auto& d = item.data;

        // Parse JSON to TickData
        if(j.contains("last_price")) d.last_price = j["last_price"]; else d.last_price = j.value("price", 0.0);
        if(j.contains("volume")) d.volume = j["volume"];
        if(j.contains("open_interest")) d.open_interest = j["open_interest"]; else d.open_interest = j.value("oi", 0.0);

        if(j.contains("bid_price1")) d.bid_price1 = j["bid_price1"]; else d.bid_price1 = j.value("b1", 0.0);
        if(j.contains("bid_volume1")) d.bid_volume1 = j["bid_volume1"]; else d.bid_volume1 = j.value("bv1", 0);
        if(j.contains("ask_price1")) d.ask_price1 = j["ask_price1"]; else d.ask_price1 = j.value("a1", 0.0);
        if(j.contains("ask_volume1")) d.ask_volume1 = j["ask_volume1"]; else d.ask_volume1 = j.value("av1", 0);

        QString timeStr;
        if(j.contains("update_time")) timeStr = QString::fromStdString(j["update_time"]); 
        else timeStr = QString::fromStdString(j.value("time", ""));
        item.updateTime = timeStr;
        strncpy(d.update_time, timeStr.toStdString().c_str(), sizeof(d.update_time)-1);
        
        if(j.contains("pre_settlement_price")) d.pre_settlement_price = j["pre_settlement_price"]; else d.pre_settlement_price = j.value("pre_settlement", 0.0);
        if(j.contains("pre_close_price")) d.pre_close_price = j["pre_close_price"]; else d.pre_close_price = j.value("pre_close", 0.0);
        
        if(j.contains("turnover")) d.turnover = j["turnover"];
        double limitUp = 0.0; if(j.contains("upper_limit_price")) limitUp = j["upper_limit_price"]; else limitUp = j.value("limit_up", 0.0);
        d.upper_limit_price = limitUp;
        double limitDown = 0.0; if(j.contains("lower_limit_price")) limitDown = j["lower_limit_price"]; else limitDown = j.value("limit_down", 0.0);
        d.lower_limit_price = limitDown;
        
        if(j.contains("open_price")) d.open_price = j["open_price"]; else d.open_price = j.value("open", 0.0);
        if(j.contains("highest_price")) d.highest_price = j["highest_price"]; else d.highest_price = j.value("high", 0.0);
        if(j.contains("lowest_price")) d.lowest_price = j["lowest_price"]; else d.lowest_price = j.value("low", 0.0);
        if(j.contains("average_price")) d.average_price = j["average_price"]; else d.average_price = j.value("avg_price", 0.0);

        // Calculated fields for UI
        double basePrice = d.pre_settlement_price;
        if (basePrice < 0.0001) basePrice = d.pre_close_price;
        item.preClose = basePrice;
        
        item.change = d.last_price - basePrice;
        item.changePercent = (basePrice > 0) ? (item.change / basePrice * 100.0) : 0.0;
        
        emit dataChanged(index(row), index(row));
    } catch (const std::exception& e) {
        qDebug() << "[MarketModel] updateTick Error:" << e.what();
    } catch (...) {
        qDebug() << "[MarketModel] updateTick Unknown Error";
    }
}

void MarketModel::handleInstrument(const QString& json) {
    qDebug() << "[MarketModel] handleInstrument called with:" << json;
    try {
        auto j = nlohmann::json::parse(json.toStdString());
        QString id;
        if(j.contains("instrument_id")) id = QString::fromStdString(j["instrument_id"]);
        else if(j.contains("id")) id = QString::fromStdString(j["id"]);
        
        if (id.isEmpty()) return;
        
        qDebug() << "[MarketModel] Parsed instrument ID:" << id;

        if (!_instrument_to_index.contains(id)) {
            qDebug() << "[MarketModel] Adding new instrument:" << id;
            beginInsertRows(QModelIndex(), _market_data.count(), _market_data.count());
            _instrument_to_index[id] = _market_data.count();
            
            MarketItem item;
            std::memset(&item.data, 0, sizeof(TickData));
            item.instrumentId = id;
            strncpy(item.data.instrument_id, id.toStdString().c_str(), sizeof(item.data.instrument_id)-1);
            // Default init
            item.preClose = 0; item.change = 0; item.changePercent = 0; item.updateTime = "--";
            
            _market_data.append(item);
            endInsertRows();
            qDebug() << "[MarketModel] Instrument added successfully";
        } else {
            qDebug() << "[MarketModel] Instrument already exists:" << id;
        }
    } catch (const std::exception& e) {
        qDebug() << "[MarketModel] Exception in handleInstrument:" << e.what();
    } catch (...) {
        qDebug() << "[MarketModel] Unknown exception in handleInstrument";
    }
}

void MarketModel::addInstrument(const QString& instrumentId) {
    if (instrumentId.isEmpty()) return;
    if (_instrument_to_index.contains(instrumentId)) return;
    
    beginInsertRows(QModelIndex(), _market_data.count(), _market_data.count());
    _instrument_to_index[instrumentId] = _market_data.count();
    
    MarketItem item;
    std::memset(&item.data, 0, sizeof(TickData));
    item.instrumentId = instrumentId;
    strncpy(item.data.instrument_id, instrumentId.toStdString().c_str(), sizeof(item.data.instrument_id)-1);
    
    // UI defaults
    item.updateTime = "等待数据...";
    item.preClose = 0; item.change = 0; item.changePercent = 0;
    
    _market_data.append(item);
    
    endInsertRows();
    qDebug() << "[MarketModel] Manually added instrument:" << instrumentId;
}

void MarketModel::removeInstrument(const QString& instrumentId) {
    if (!_instrument_to_index.contains(instrumentId)) return;
    
    int row = _instrument_to_index[instrumentId];
    beginRemoveRows(QModelIndex(), row, row);
    _market_data.removeAt(row);
    _instrument_to_index.remove(instrumentId);
    
    // 更新受影响的索引
    for (int i = row; i < _market_data.size(); ++i) {
        _instrument_to_index[_market_data[i].instrumentId] = i;
    }
    
    endRemoveRows();
}

void MarketModel::move(int from, int to) {
    if (from < 0 || from >= _market_data.size() || to < 0 || to >= _market_data.size() || from == to) return;

    // 如果是向下移 (to > from)，beginMoveRows 的 dest 其实是 to+1
    int destIndex = (to > from) ? to + 1 : to;

    if (beginMoveRows(QModelIndex(), from, from, QModelIndex(), destIndex)) {
        _market_data.move(from, to); 
        
        // 重建索引
        _instrument_to_index.clear();
        for (int i = 0; i < _market_data.size(); ++i) {
            _instrument_to_index[_market_data[i].instrumentId] = i;
        }
        
        endMoveRows();
    }
}

QStringList MarketModel::getAllInstruments() const {
    QStringList list;
    for (const auto& item : _market_data) {
        list.append(item.instrumentId);
    }
    return list;
}

void MarketModel::moveToTop(int index) {
    if (index <= 0 || index >= _market_data.size()) return;
    
    beginMoveRows(QModelIndex(), index, index, QModelIndex(), 0);
    MarketItem item = _market_data.takeAt(index);
    _market_data.prepend(item);
    
    // 重建索引
    for (int i = 0; i < _market_data.size(); ++i) {
        _instrument_to_index[_market_data[i].instrumentId] = i;
    }
    endMoveRows();
}

void MarketModel::moveToBottom(int index) {
    if (index < 0 || index >= _market_data.size() - 1) return;
    
    beginMoveRows(QModelIndex(), index, index, QModelIndex(), _market_data.size());
    MarketItem item = _market_data.takeAt(index);
    _market_data.append(item);
    
    // 重建索引
    for (int i = 0; i < _market_data.size(); ++i) {
        _instrument_to_index[_market_data[i].instrumentId] = i;
    }
    endMoveRows();
}

bool MarketModel::hasInstrument(const QString& instrumentId) const {
    return _instrument_to_index.contains(instrumentId);
}

void MarketModel::setInstrumentOrder(const QStringList& ids) {
    if (ids.isEmpty()) return;
    
    qDebug() << "[MarketModel] Reordering instruments...";
    
    // 使用 resetModel 一次性刷新整个视图，避免大量信号
    beginResetModel();
    
    QVector<MarketItem> newData;
    QSet<QString> processedIds;
    
    // 1. 优先按照传入列表的顺序添加已存在的合约
    for (const auto& id : ids) {
        if (id.isEmpty()) continue;
        if (_instrument_to_index.contains(id)) {
            newData.append(_market_data[_instrument_to_index[id]]);
            processedIds.insert(id);
        } else {
            // 如果列表中有但内存里没有，应该新建吗？
            // 是的，恢复订阅时这就是“添加”逻辑
            MarketItem item;
            std::memset(&item.data, 0, sizeof(TickData));
            item.instrumentId = id;
            strncpy(item.data.instrument_id, id.toStdString().c_str(), sizeof(item.data.instrument_id)-1);
            item.preClose = 0; item.change = 0; item.changePercent = 0; item.updateTime = "等待数据...";
            newData.append(item);
            processedIds.insert(id);
        }
    }
    
    // 2. 把剩余的（可能是刚才 ZMQ 推送的新合约，但不在 saved list 里）放到最后
    for (const auto& item : _market_data) {
        if (!processedIds.contains(item.instrumentId)) {
            newData.append(item);
        }
    }
    
    _market_data = newData;
    
    // 重建索引
    _instrument_to_index.clear();
    for (int i = 0; i < _market_data.size(); ++i) {
        _instrument_to_index[_market_data[i].instrumentId] = i;
    }
    
    endResetModel();
    qDebug() << "[MarketModel] Reorder completed. Total:" << _market_data.size();
}

} // namespace atrad
