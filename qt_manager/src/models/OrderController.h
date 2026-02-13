#pragma once

#include <QObject>
#include <QString>
#include <QHash>
#include <QVariant>
#include <QList>
#include <QJsonObject>
#include <nlohmann/json.hpp>
#include "protocol/message_schema.h" // Shared Schema

namespace QuantLabs {

class OrderController : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(QString instrumentId READ instrumentId WRITE setInstrumentId NOTIFY orderParamsChanged)
    Q_PROPERTY(double price READ price WRITE setPrice NOTIFY orderParamsChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY orderParamsChanged)
    
    Q_PROPERTY(double estimatedMargin READ estimatedMargin NOTIFY calculationChanged)
    Q_PROPERTY(double estimatedCommission READ estimatedCommission NOTIFY calculationChanged)
    
    Q_PROPERTY(bool isAutoPrice READ isAutoPrice WRITE setAutoPrice NOTIFY orderParamsChanged)
    Q_PROPERTY(bool isTestMode READ isTestMode WRITE setTestMode NOTIFY orderParamsChanged)
    
    Q_PROPERTY(double priceTick READ priceTick NOTIFY orderParamsChanged)

    Q_PROPERTY(QVariantList bidPrices READ bidPrices NOTIFY marketDataChanged)
    Q_PROPERTY(QVariantList bidVolumes READ bidVolumes NOTIFY marketDataChanged)
    Q_PROPERTY(QVariantList askPrices READ askPrices NOTIFY marketDataChanged)
    Q_PROPERTY(QVariantList askVolumes READ askVolumes NOTIFY marketDataChanged)

    // 连接状态
    Q_PROPERTY(bool coreConnected READ coreConnected NOTIFY connectionChanged)
    Q_PROPERTY(bool ctpConnected READ ctpConnected NOTIFY connectionChanged)

    // Current Instrument Position
    Q_PROPERTY(int longPosition READ longPosition NOTIFY positionChanged)
    Q_PROPERTY(int shortPosition READ shortPosition NOTIFY positionChanged)
    Q_PROPERTY(int longYd READ longYd NOTIFY positionChanged)
    Q_PROPERTY(int shortYd READ shortYd NOTIFY positionChanged)
    Q_PROPERTY(int longTd READ longTd NOTIFY positionChanged)
    Q_PROPERTY(int shortTd READ shortTd NOTIFY positionChanged)
    
    // Condition Orders
    // Condition Orders
    Q_PROPERTY(QVariantList conditionOrderList READ conditionOrderList NOTIFY conditionOrderListChanged)
    Q_PROPERTY(QVariantList strategyList READ strategyList NOTIFY strategyListChanged) // Added

public:
    explicit OrderController(QObject *parent = nullptr);

    QString instrumentId() const { return _instrumentId; }
    void setInstrumentId(const QString& id) { 
        if(_instrumentId != id) { 
            _instrumentId = id; 
            _isManualPrice = false; 
            updateCurrentPos(id);
            emit orderParamsChanged(); 
        } 
    }

    double price() const { return _price; }
    void setPrice(double p) { if(_price != p) { _price = p; emit orderParamsChanged(); } }

    int volume() const { return _volume; }
    void setVolume(int v) { if(_volume != v) { _volume = v; emit orderParamsChanged(); } }

    double priceTick() const { 
        if(_instrumentId.isEmpty() || !_instrument_dict.contains(_instrumentId)) return 1.0;
        double t = _instrument_dict[_instrumentId].price_tick;
        return (t < 1e-6) ? 1.0 : t;
    }

    double estimatedMargin() const { return _estimatedMargin; }
    double estimatedCommission() const { return _estimatedCommission; }

    bool isAutoPrice() const { return !_isManualPrice; }
    void setAutoPrice(bool v) { 
        if(_isManualPrice == v) { _isManualPrice = !v; emit orderParamsChanged(); } 
    }
    
    bool isTestMode() const { return _isTestMode; }
    void setTestMode(bool v) { if(_isTestMode!=v){_isTestMode=v; emit orderParamsChanged();} }

    QVariantList bidPrices() const { return _bidPrices; }
    QVariantList bidVolumes() const { return _bidVolumes; }
    QVariantList askPrices() const { return _askPrices; }
    QVariantList askVolumes() const { return _askVolumes; }
    
    bool coreConnected() const { return _coreConnected; }
    bool ctpConnected() const { return _ctpConnected; }
    
    QVariantList conditionOrderList() const { return _conditionOrderList; }
    QVariantList strategyList() const { return _strategyList; }

public slots:
    void onTick(const QJsonObject& json);
    void onTickBinary(const TickData& data);
    void updateInstrument(const QJsonObject& json);
    Q_INVOKABLE void sendOrder(const QString& direction, const QString& offset, const QString& priceType = "LIMIT");
    void cancelOrder(const QString& instrumentId, const QString& orderSysId, const QString& orderRef, const QString& exchangeId, int frontId, int sessionId); // Added
    void subscribe(const QString& instrumentId);
    void unsubscribe(const QString& instrumentId);
    
    void setPriceOriginal(double p) { _price = p; emit orderParamsChanged(); }
    void setManualPrice(bool manual) { _isManualPrice = manual; emit orderParamsChanged(); }

    // 由 ZmqWorker 调用
    // 由 Workers 调用
    void updateConnectionStatus(bool core, bool ctp); // Keep for compatibility if needed, or remove
    void updateCoreStatus(bool connected);
    void updateCtpStatus(bool connected);
    void onPositionReceived(const QJsonObject& json);

    // QML 调用此方法发送指令 (中转到 Worker)
    Q_INVOKABLE void sendCommand(const QString& cmd);
    
    // 封装条件单发送
    Q_INVOKABLE void sendConditionOrder(const QString& dataJson);
    
    // Condition Order Management
    Q_INVOKABLE void cancelConditionOrder(const QString& requestId);
    void onConditionOrderReturn(const QJsonObject& json);
    void queryConditionOrders(); // Called on startup/reconnect
    void queryStrategies(); // Query available strategies
    void modifyConditionOrder(const QString& requestId, double triggerPrice, double limitPrice, int volume); // 修改条件单
    Q_INVOKABLE double getInstrumentPriceTick(const QString& instrumentId) const; // 获取合约最小变动价位
    
    // 自动补全：模糊搜索合约（ID 或名称），返回最多 maxResults 条匹配结果
    Q_INVOKABLE QVariantList searchInstruments(const QString& keyword, int maxResults = 10) const;
    // 精确验证：合约 ID 是否存在于 instrument_dict
    Q_INVOKABLE bool isValidInstrument(const QString& instrumentId) const;

signals:
    void orderParamsChanged();
    void calculationChanged();
    void marketDataChanged();
    void connectionChanged();
    void orderSent(const QString& json); 
    void positionChanged();
    void conditionOrderListChanged();
    void strategyListChanged();
    void conditionOrderSound(const QString& soundType); // "triggered" or "cancelled"

private:
    void recalculate();
    void updateCurrentPos(const QString& id);
    
    QString _instrumentId;
    double _price = 0.0;
    int _volume = 1;
    bool _isManualPrice = false;
    bool _isTestMode = false;
    
    double _estimatedMargin = 0.0;
    double _estimatedCommission = 0.0;
    
    bool _coreConnected = false;
    bool _ctpConnected = false;
    
    QVariantList _bidPrices, _bidVolumes, _askPrices, _askVolumes;
    
    QHash<QString, InstrumentMeta> _instrument_dict;

    struct PosSummary {
        int longTotal = 0;
        int longYd = 0;
        int longTd = 0;
        int shortTotal = 0;
        int shortYd = 0;
        int shortTd = 0;
    };
    QHash<QString, PosSummary> _pos_cache;
    PosSummary _currentPos;

    int longPosition() const { return _currentPos.longTotal; }
    int shortPosition() const { return _currentPos.shortTotal; }
    int longYd() const { return _currentPos.longYd; }
    int shortYd() const { return _currentPos.shortYd; }
    int longTd() const { return _currentPos.longTd; }
    int shortTd() const { return _currentPos.shortTd; }
    
    // Condition orders data
    QVariantList _conditionOrderList;
    QVariantList _strategyList;
};

} // namespace QuantLabs
