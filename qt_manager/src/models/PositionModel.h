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
    // Helper fields for UI
    QString instrumentId; 
    double lastPrice;
    double bidPrice1;  // 买一价
    double askPrice1;  // 卖一价
    double priceTick;  // 最小变动价位
    double upperLimit; // 涨停板
    double lowerLimit; // 跌停板
    double profit;
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
        CostRole,
        ProfitRole,
        LastPriceRole,
        BidPrice1Role,
        AskPrice1Role,
        PriceTickRole,
        UpperLimitRole,
        LowerLimitRole,
        AvgPriceRole,
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
    void updateInstrument(const QJsonObject& json); 

signals:
    void totalProfitChanged(double totalProfit);

private:
    void calculateProfit(PositionItem& item);
    QVector<PositionItem> _position_data;
    QHash<QString, QVector<int>> _instrument_to_indices; 
    
    // Use shared InstrumentData
    QHash<QString, InstrumentData> _instrument_dict;
    
    double _total_profit = 0.0;
};


} // namespace QuantLabs
