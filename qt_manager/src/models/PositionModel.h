#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QVector>
#include <QString>

namespace atrad {

struct PositionItem {
    QString instrumentId;
    char direction; // '0'多, '1'空
    int position;
    int todayPosition;
    int ydPosition;
    double cost;
    double profit;
    double lastPrice; // 新增：最后行情价
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
        LastPriceRole
    };

    explicit PositionModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    double totalProfit() const { return _total_profit; }

public slots:
    void updatePosition(const QString& json);
    void updatePrice(const QString& json); // 新增：接收行情 Tick 更新盈亏
    void updateInstrument(const QString& json); // 新增：接收合约字典

signals:
    void totalProfitChanged(double totalProfit);

private:
    void calculateProfit(PositionItem& item);
    QVector<PositionItem> _position_data;
    QHash<QString, QVector<int>> _instrument_to_indices; // 一个品种可能有多空两个持仓行
    
    struct InstrumentInfo {
        int multiple;
        double tick;
        // 保证金率
        double longMarginRatio;
        double shortMarginRatio;
        // 手续费率 (简化，主要存按金额的)
        double openRatio;
        double closeRatio;
        double closeTodayRatio;
    };
    QHash<QString, InstrumentInfo> _instrument_dict;
    
    double _total_profit = 0.0;
};

} // namespace atrad
