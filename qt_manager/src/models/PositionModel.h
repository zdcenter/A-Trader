#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QVector>
#include <QString>
#include <QJsonObject>
#include "../../../shared/protocol/message_schema.h"

namespace QuantLabs {

struct PositionItem {
    PositionData data;
    // UI 辅助字段（行情推送填充）
    QString instrumentId; 
    double lastPrice = 0.0;
    double bidPrice1 = 0.0;   // 买一价
    double askPrice1 = 0.0;   // 卖一价
    double priceTick = 0.0;   // 最小变动价位
    double upperLimit = 0.0;  // 涨停板
    double lowerLimit = 0.0;  // 跌停板
};

class PositionModel : public QAbstractListModel {

    Q_OBJECT
public:
    enum PositionRoles {
        IdRole = Qt::UserRole + 1,
        DirectionRole,
        PosRole,
        TodayPosRole,
        YdPosRole,
        AvgPriceRole,       // 持仓均价 = open_cost / (pos × mult)
        LastPriceRole,
        PosProfitRole,      // 持仓浮动盈亏 (core 推送)
        CloseProfitRole,    // 已实现平仓盈亏 (core 推送)
        MarginRole,         // 占用保证金
        BidPrice1Role,
        AskPrice1Role,
        PriceTickRole,
        UpperLimitRole,
        LowerLimitRole,
        ExchangeRole
    };

    explicit PositionModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    double totalProfit() const { return _total_profit; }

public slots:
    void updatePosition(const QJsonObject& json);
    void updatePrice(const QJsonObject& json); 
    void updatePriceBinary(const TickData& data);
    void updateInstrument(const QJsonObject& json); 

signals:
    void totalProfitChanged(double totalProfit);
    void instrumentNeeded(const QString& instrumentId);  // 持仓合约需要订阅行情

private:
    void recalcTotalProfit();
    QVector<PositionItem> _position_data;
    QHash<QString, QVector<int>> _instrument_to_indices; 
    
    // 合约信息字典（复用 shared 的 InstrumentMeta）
    QHash<QString, InstrumentMeta> _instrument_dict;
    
    double _total_profit = 0.0;
};


} // namespace QuantLabs
