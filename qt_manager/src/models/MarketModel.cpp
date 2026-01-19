#include "models/MarketModel.h"
#include <nlohmann/json.hpp>
#include <QDebug>

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
        case PriceRole: return item.lastPrice;
        case PreCloseRole: return item.preClose;
        case ChangeRole: return item.change;
        case ChangePercentRole: return item.changePercent;
        case VolumeRole: return item.volume;
        case OpenInterestRole: return item.openInterest;
        case BidPrice1Role: return item.bidPrice1;
        case BidVolume1Role: return item.bidVolume1;
        case AskPrice1Role: return item.askPrice1;
        case AskVolume1Role: return item.askVolume1;
        case TimeRole: return item.updateTime;
        case TurnoverRole: return item.turnover;
        case UpperLimitRole: return item.upperLimit;
        case LowerLimitRole: return item.lowerLimit;
        case OpenPriceRole: return item.openPrice;
        case HighestPriceRole: return item.highestPrice;
        case LowestPriceRole: return item.lowestPrice;
        case AveragePriceRole: return item.averagePrice;
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
        QString id = QString::fromStdString(j["id"]);
        double price = j["price"];
        int volume = j["volume"];
        int oi = j.value("oi", 0);
        double b1 = j.value("b1", 0.0);
        int bv1 = j.value("bv1", 0);
        double a1 = j.value("a1", 0.0);
        int av1 = j.value("av1", 0);
        QString time = QString::fromStdString(j["time"]);

        // 解析昨结和昨收
        double preSettlement = j.value("pre_settlement", 0.0);
        double preCloseRaw = j.value("pre_close", 0.0);
        
        // 用于计算涨跌的基准价：期货通常优先使用昨结算价
        double basePrice = preSettlement;
        if (basePrice < 0.0001) basePrice = preCloseRaw;
        
        double turnover = j.value("turnover", 0.0);
        double limitUp = j.value("limit_up", 0.0);
        double limitDown = j.value("limit_down", 0.0);
        double open = j.value("open", 0.0);
        double high = j.value("high", 0.0);
        double low = j.value("low", 0.0);
        double avg = j.value("avg_price", 0.0);

        if (_instrument_to_index.contains(id)) {
            int row = _instrument_to_index[id];
            auto& item = _market_data[row];
            
            // 更新基础数据
            item.lastPrice = price;
            item.volume = volume;
            item.openInterest = oi;
            item.bidPrice1 = b1;
            item.bidVolume1 = bv1;
            item.askPrice1 = a1;
            item.askVolume1 = av1;
            item.updateTime = time;
            
            // 更新扩展数据
            if (basePrice > 0.0001) item.preClose = basePrice;
            item.turnover = turnover;
            item.upperLimit = limitUp;
            item.lowerLimit = limitDown;
            item.openPrice = open;
            item.highestPrice = high;
            item.lowestPrice = low;
            item.averagePrice = avg;
            
            // 计算涨跌（使用最新的 preClose）
            if (item.preClose > 0.0001) {
                item.change = price - item.preClose;
                item.changePercent = (item.change / item.preClose) * 100.0;
            }
            
            emit dataChanged(index(row), index(row));
        } else {
            // 新合约，初始化所有字段
            beginInsertRows(QModelIndex(), _market_data.count(), _market_data.count());
            _instrument_to_index[id] = _market_data.count();
            
            double change = 0.0;
            double changePercent = 0.0;
            if (basePrice > 0.0001) {
                change = price - basePrice;
                changePercent = (change / basePrice) * 100.0;
            }
            
            _market_data.append({
                id, price, basePrice, change, changePercent, 
                volume, oi, b1, bv1, a1, av1, time,
                turnover, limitUp, limitDown,
                open, high, low, avg // O/H/L/Avg
            });
            endInsertRows();
        }
    } catch (...) {
        // 解析失败，忽略
    }
}

void MarketModel::handleInstrument(const QString& json) {
    qDebug() << "[MarketModel] handleInstrument called with:" << json;
    try {
        auto j = nlohmann::json::parse(json.toStdString());
        QString id = QString::fromStdString(j["id"]);
        
        qDebug() << "[MarketModel] Parsed instrument ID:" << id;
        
        // 如果列表中还没有这个合约，先添加一个占位行
        if (!_instrument_to_index.contains(id)) {
            qDebug() << "[MarketModel] Adding new instrument:" << id;
            beginInsertRows(QModelIndex(), _market_data.count(), _market_data.count());
            _instrument_to_index[id] = _market_data.count();
            // 初始化所有字段为0或空
            _market_data.append({
                id, 0.0, 0.0, 0.0, 0.0, 
                0, 0, 0.0, 0, 0.0, 0, "--:--:--",
                0.0, 0.0, 0.0,
                0.0, 0.0, 0.0, 0.0
            });
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
    if (_instrument_to_index.contains(instrumentId)) return;
    
    beginInsertRows(QModelIndex(), _market_data.count(), _market_data.count());
    _instrument_to_index[instrumentId] = _market_data.count();
    
    // 初始化占位数据
    _market_data.append({
        instrumentId, 0.0, 0.0, 0.0, 0.0, 
        0, 0, 0.0, 0, 0.0, 0, "等待数据...",
        0.0, 0.0, 0.0, 
        0.0, 0.0, 0.0, 0.0
    });
    
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
        // 优化：其实只用重建 min(from,to) 到 max(from,to) 范围的，但这里简单处理
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

} // namespace atrad
