#pragma once

#include <QJsonObject>
#include <QAbstractListModel>
#include <vector>
#include <mutex>

namespace QuantLabs {

struct TradeItem {
    QString instrument_id;
    QString trade_id;
    QString order_sys_id;
    QString direction;      // "0":Buy, "1":Sell
    QString offset_flag;    // "0":Open, "1":Close ...
    double price;
    int volume;
    double commission; // Added
    double close_profit; // Added
    QString trade_time;
    QString trade_date;
};

class TradeModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum TradeRoles {
        InstrumentIdRole = Qt::UserRole + 1,
        TradeIdRole,
        OrderSysIdRole,
        DirectionRole,
        OffsetFlagRole,
        PriceRole,
        VolumeRole,
        CommissionRole, // Added
        CloseProfitRole, // Added
        TimeRole,
        DateRole
    };

    explicit TradeModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void tradeSoundTriggered();

public slots:
    void onTradeReceived(const QJsonObject& json);

private:
    std::vector<TradeItem> _trades;
    mutable std::mutex _mutex;
};

} // namespace QuantLabs
