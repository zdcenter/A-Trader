#include "models/PositionModel.h"
#include <nlohmann/json.hpp>
#include <QDebug>

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
        case DirectionRole: return (item.direction == '2' || item.direction == '0') ? "BUY" : "SELL";
        case PosRole: return item.position;
        case TodayPosRole: return item.todayPosition;
        case YdPosRole: return item.ydPosition;
        case CostRole: return item.cost;
        case ProfitRole: return QString::number(item.profit, 'f', 2);
        case LastPriceRole: return item.lastPrice;
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
    return roles;
}

void PositionModel::updatePosition(const QString& json) {
    try {
        auto j = nlohmann::json::parse(json.toStdString());
        QString id = QString::fromStdString(j["id"]);
        std::string dir_str = j["dir"].is_string() ? j["dir"].get<std::string>() : std::string(1, (char)j["dir"].get<int>());
        char dir = dir_str[0];
        
        int pos = j["pos"];
        int td = j["td"];
        int yd = j["yd"];
        double cost = j["cost"];
        
        // CTP 返回的 cost 通常是总成本，我们需要计算平均成本价以便后续调价计算
        // 假设乘数为 10 (后面可以根据品种表细化)
        double multiplier = 10.0; 
        if (id.startsWith("p") || id.startsWith("y")) multiplier = 10.0;
        else if (id.startsWith("rb")) multiplier = 10.0;

        int row = -1;
        for(int i=0; i<_position_data.size(); ++i) {
            if(_position_data[i].instrumentId == id && _position_data[i].direction == dir) {
                row = i;
                break;
            }
        }

        if (row != -1) {
            _position_data[row].position = pos;
            _position_data[row].todayPosition = td;
            _position_data[row].ydPosition = yd;
            _position_data[row].cost = cost;
            calculateProfit(_position_data[row]);
            emit dataChanged(index(row), index(row));
        } else {
            beginInsertRows(QModelIndex(), _position_data.count(), _position_data.count());
            PositionItem item{id, dir, pos, td, yd, cost, 0.0, 0.0};
            _position_data.append(item);
            _instrument_to_indices[id].append(_position_data.count() - 1);
            endInsertRows();
        }
    } catch (...) {}
}

void PositionModel::updatePrice(const QString& json) {
    try {
        auto j = nlohmann::json::parse(json.toStdString());
        QString id = QString::fromStdString(j["id"]);
        // 修正 key: Core 发送的是 "last_price" (或者 "lp" 根据 publisher.cpp 确认，不过看 MarketModel 也是 lp? 
        // 让我确认一下 Publisher.cpp，哦，MarketModel 用的是 publisher 里的 tick 结构体，
        // 让我们保守一点，两个都试一下，或者直接看 Publisher.cpp。
        // 根据之前的 list_dir 结果不好看，但 MarketModel 解析的是 "price"，
        // 让我再看一眼 MarketModel.cpp，它是 j["price"]。
        // 所以这里应该是 j["price"]。
        double lastPrice = 0.0;
        if (j.contains("price")) lastPrice = j["price"];
        else if (j.contains("last_price")) lastPrice = j["last_price"];
        else return;

        if (!_instrument_to_indices.contains(id)) return;

        // 仅处理该合约相关的持仓行
        for (int row : _instrument_to_indices[id]) {
            _position_data[row].lastPrice = lastPrice;
            calculateProfit(_position_data[row]);
            emit dataChanged(index(row), index(row), {ProfitRole, CostRole, LastPriceRole});
        }

        // 重新汇总所有持仓的总盈亏
        double current_all_profit = 0.0;
        for (const auto& item : _position_data) {
            current_all_profit += item.profit;
        }
        
        // 总是发射信号，即使没变，因为可能是其他地方变了需要刷新（或者加个防抖）
        // 这里还是防抖一下吧
        if (std::abs(current_all_profit - _total_profit) > 0.01) {
            _total_profit = current_all_profit;
            emit totalProfitChanged(_total_profit);
        }
    } catch (...) {}
}

void PositionModel::updateInstrument(const QString& json) {
    try {
        auto j = nlohmann::json::parse(json.toStdString());
        QString id = QString::fromStdString(j["id"]);
        
        InstrumentInfo info;
        info.multiple = j["mult"];
        info.tick = j["tick"];
        
        // 保证金率 (优先取按金额的，通常国内期货主要用这个)
        info.longMarginRatio = j["l_m_money"];
        info.shortMarginRatio = j["s_m_money"];
        
        // 手续费率
        info.openRatio = j["o_r_money"];
        info.closeRatio = j["c_r_money"];
        info.closeTodayRatio = j["ct_r_money"];

        _instrument_dict[id] = info;
        qDebug() << "[PositionModel] Full Instrument Dict Sync:" << id 
                 << "Mult:" << info.multiple 
                 << "Margin(L):" << info.longMarginRatio;

        // 更新字典后，如果已有该品种持仓，重新计算一次盈亏
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

    // 优先使用字典中的真实乘数，否则使用默认值 10
    double multiplier = 10.0; 
    if (_instrument_dict.contains(item.instrumentId)) {
        multiplier = _instrument_dict[item.instrumentId].multiple;
    }

    double current_value = item.lastPrice * item.position * multiplier;
    
    if (item.direction == '2' || item.direction == '0') { // Buy
        item.profit = current_value - item.cost;
    } else { // Sell
        item.profit = item.cost - current_value;
    }
}

} // namespace atrad
