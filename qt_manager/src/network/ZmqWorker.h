#pragma once

#include <QJsonObject>
#include <QJsonDocument>
#include <QObject>
#include <QThread>
#include <QString>
#include <zmq.hpp>


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

    /**
     * @brief 向 Core 发送同步指令 (下单/订阅等)
     */
    void sendCommand(const QString& json);

signals:
    /**
     * @brief 收到新的行情 JSON 时触发
     */
    /**
     * @brief 收到新的行情 JSON 时触发
     */
    void tickReceived(const QJsonObject& json);

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
     * @brief 状态更新信号
     * @param core Core是否连接
     * @param ctp CTP是否连接（有行情活动）
     */
    void statusUpdated(bool core, bool ctp);

private:
    bool _running = false;
    zmq::context_t _context;
    zmq::socket_t _subscriber;
};

} // namespace QuantLabs
