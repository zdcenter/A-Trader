#pragma once

#include <QObject>
#include <QString>
#include <QHash>
#include <QVariant>
#include <QList>
#include <nlohmann/json.hpp>
#include "protocol/message_schema.h" // Shared Schema

namespace atrad {

class OrderController : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(QString instrumentId READ instrumentId WRITE setInstrumentId NOTIFY orderParamsChanged)
    Q_PROPERTY(double price READ price WRITE setPrice NOTIFY orderParamsChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY orderParamsChanged)
    
    Q_PROPERTY(double estimatedMargin READ estimatedMargin NOTIFY calculationChanged)
    Q_PROPERTY(double estimatedCommission READ estimatedCommission NOTIFY calculationChanged)
    
    Q_PROPERTY(bool isAutoPrice READ isAutoPrice WRITE setAutoPrice NOTIFY orderParamsChanged)
    Q_PROPERTY(bool isTestMode READ isTestMode WRITE setTestMode NOTIFY orderParamsChanged)

    Q_PROPERTY(QVariantList bidPrices READ bidPrices NOTIFY marketDataChanged)
    Q_PROPERTY(QVariantList bidVolumes READ bidVolumes NOTIFY marketDataChanged)
    Q_PROPERTY(QVariantList askPrices READ askPrices NOTIFY marketDataChanged)
    Q_PROPERTY(QVariantList askVolumes READ askVolumes NOTIFY marketDataChanged)

    // 连接状态
    Q_PROPERTY(bool coreConnected READ coreConnected NOTIFY connectionChanged)
    Q_PROPERTY(bool ctpConnected READ ctpConnected NOTIFY connectionChanged)

public:
    explicit OrderController(QObject *parent = nullptr);

    QString instrumentId() const { return _instrumentId; }
    void setInstrumentId(const QString& id) { 
        if(_instrumentId != id) { 
            _instrumentId = id; 
            _isManualPrice = false; 
            emit orderParamsChanged(); 
        } 
    }

    double price() const { return _price; }
    void setPrice(double p) { if(_price != p) { _price = p; emit orderParamsChanged(); } }

    int volume() const { return _volume; }
    void setVolume(int v) { if(_volume != v) { _volume = v; emit orderParamsChanged(); } }

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

public slots:
    void onTick(const QString& json);
    void updateInstrument(const QString& json);
    void sendOrder(const QString& direction, const QString& offset);
    void subscribe(const QString& instrumentId);
    void unsubscribe(const QString& instrumentId);
    
    void setPriceOriginal(double p) { _price = p; emit orderParamsChanged(); }
    void setManualPrice(bool manual) { _isManualPrice = manual; emit orderParamsChanged(); }

    // 由 ZmqWorker 调用
    void updateConnectionStatus(bool core, bool ctp);

    // QML 调用此方法发送指令 (中转到 Worker)
    Q_INVOKABLE void sendCommand(const QString& cmd);

signals:
    void orderParamsChanged();
    void calculationChanged();
    void marketDataChanged();
    void connectionChanged();
    void orderSent(const QString& json); 

private:
    void recalculate();
    
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
    
    QHash<QString, InstrumentData> _instrument_dict;
};

} // namespace atrad
