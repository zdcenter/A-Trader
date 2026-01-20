#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QVector>
#include <QString>
#include "../../../shared/protocol/message_schema.h"

namespace atrad {

struct MarketItem {
    TickData data;
    
    // UI 专用的缓存/计算字段
    QString instrumentId; 
    double preClose;         // 显示用的昨收（可能是 PreSettlement）
    double change;           // 涨跌额
    double changePercent;    // 涨跌幅 %
    QString updateTime;      // 字符串格式时间
};

class MarketModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum MarketRoles {
        IdRole = Qt::UserRole + 1,
        PriceRole,
        PreCloseRole,
        ChangeRole,
        ChangePercentRole,
        VolumeRole,
        OpenInterestRole,
        BidPrice1Role,
        BidVolume1Role,
        AskPrice1Role,
        AskVolume1Role,
        TimeRole,
        TurnoverRole,
        UpperLimitRole,
        LowerLimitRole,
        OpenPriceRole,
        HighestPriceRole,
        LowestPriceRole,
        AveragePriceRole
    };

    explicit MarketModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    /**
     * @brief 更新或添加行情的 JSON 数据
     * @param json 来自 ZmqWorker 的消息
     */
    void updateTick(const QString& json);
    // 处理合约信息 (订阅后立即显示)
    void handleInstrument(const QString& json);
    void removeInstrument(const QString& instrumentId);
    
    // 手动添加合约（用于UI订阅时立即显示）
    Q_INVOKABLE void addInstrument(const QString& instrumentId);

    // 拖拽排序
    Q_INVOKABLE void move(int from, int to);
    // 获取当前顺序的所有合约ID
    Q_INVOKABLE QStringList getAllInstruments() const;
    // 排序操作
    Q_INVOKABLE void moveToTop(int index);
    Q_INVOKABLE void moveToBottom(int index);
    
    // 强制重排合约顺序 (用于启动时恢复)
    Q_INVOKABLE void setInstrumentOrder(const QStringList& ids);
    
    // 检查合约是否已存在
    Q_INVOKABLE bool hasInstrument(const QString& instrumentId) const;

private:
    QVector<MarketItem> _market_data;
    QHash<QString, int> _instrument_to_index; // 快速索引
};

} // namespace atrad
