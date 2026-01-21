#pragma once

#include <QAbstractListModel>
#include <vector>
#include <mutex>

namespace atrad {

struct OrderItem {
    QString instrument_id;
    QString order_sys_id;
    QString order_ref;
    QString direction;      // "0":Buy, "1":Sell
    QString offset_flag;    // "0":Open, "1":Close, "3":CloseToday, "4":CloseYd
    double limit_price;
    int volume_total_original;
    int volume_traded;
    int volume_total;       // 剩余
    QString order_status;   // "0":AllTraded, "1":PartTraded, "3":NoTrade, "5":Canceled, "a":Unknown
    QString status_msg;
    QString insert_time;
};

class OrderModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum OrderRoles {
        InstrumentIdRole = Qt::UserRole + 1,
        OrderSysIdRole,
        DirectionRole,
        OffsetFlagRole,
        PriceRole,
        VolumeOriginalRole,
        VolumeTradedRole,
        VolumeTotalRole,
        StatusRole,
        StatusMsgRole,
        TimeRole
    };

    explicit OrderModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void onOrderReceived(const QString& json);

private:
    std::vector<OrderItem> _orders;
    mutable std::mutex _mutex;
    
    // 简单的 key-index 映射，用于快速更新 (Key: OrderSysID or OrderRef if sysid empty)
    // 简化起见，我们遍历查找或者用 OrderSysID
    int findOrderIndex(const QString& sysId, const QString& ref);
};

} // namespace atrad
