#pragma once

#include <QJsonObject>
#include <QJsonDocument>
#include <QObject>
#include <QThread>
#include <QString>
#include <zmq.hpp>
#include "protocol/message_schema.h"


namespace QuantLabs {

class ZmqWorker : public QObject {
    Q_OBJECT
public:
    explicit ZmqWorker(QObject *parent = nullptr);
    ~ZmqWorker();

public slots:
    /**
     * @brief 启动工作循环，订阅行情
     */
    void process();
    void stop();



signals:
    /**
     * @brief 收到新的行情 JSON 时触发
     */
    /**
     * @brief 收到新的行情 JSON 时触发
     */
    void tickReceived(const QJsonObject& json);
    void tickReceivedBinary(const TickData& data);

    /**
     * @brief 收到持仓 JSON 时触发
     */
    void positionReceived(const QJsonObject& json);

    /**
     * @brief 收到账户资金 JSON 时触发
     */
    void accountReceived(const QJsonObject& json);

    /**
     * @brief 收到合约属性 JSON 时触发
     */
    void instrumentReceived(const QJsonObject& json);

    void orderReceived(const QJsonObject& json);
    void tradeReceived(const QJsonObject& json);
    void conditionOrderReceived(const QJsonObject& json); // Added for Strategy Push
    
    /**
     * @brief CTP 行情连接状态更新 (基于是否有行情数据推送)
     */
    void ctpStatusUpdated(bool connected);
    
    /**
     * @brief 请求发送指令（由 CommandWorker 处理）
     */
    void commandRequired(const QString& json);

private:
    bool _running = false;
    zmq::context_t _context;
    zmq::socket_t _subscriber;
};

} // namespace QuantLabs
