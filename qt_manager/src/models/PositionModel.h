#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QVector>
#include <QString>
#include "../../../shared/protocol/message_schema.h"

namespace atrad {

struct PositionItem {
    PositionData data;
    // Helper fields for UI
    QString instrumentId; 
    double lastPrice; 
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
        AvgPriceRole,
        ExchangeRole
    };

    explicit PositionModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    double totalProfit() const { return _total_profit; }

public slots:
    void updatePosition(const QString& json);
    void updatePrice(const QString& json); 
    void updateInstrument(const QString& json); 

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


} // namespace atrad
