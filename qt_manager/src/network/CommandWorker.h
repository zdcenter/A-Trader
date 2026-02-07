#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <zmq.hpp>

namespace QuantLabs {

/**
 * @brief 专门负责处理 REQ-REP 同步指令的 Worker
 * 运行在独立线程中，避免阻塞行情/UI线程
 */
class CommandWorker : public QObject {
    Q_OBJECT
public:
    explicit CommandWorker(QObject *parent = nullptr);
    ~CommandWorker();

public slots:
    /**
     * @brief 初始化并连接REQ套接字
     */
    void connectToCore();

    /**
     * @brief 向 Core 发送同步指令 (下单/订阅等)
     * 该函数会阻塞等待响应，但因为在独立线程，不会卡死UI或行情
     */
    void sendCommand(const QString& json);

    /**
     * @brief 启动心跳检测 (默认5秒)
     */
    void startHeartbeat(int intervalMs = 5000);

signals:
    /**
     * @brief 收到指令回复（可选使用）
     */
    void commandReplyReceived(const QString& reply);

    /**
     * @brief Core 连接状态更新
     */
    void coreStatusUpdated(bool connected);

private slots:
    void sendPing(); 

private:
    zmq::context_t _context;
    zmq::socket_t _requester;
    bool _connected = false;
    QTimer* _heartbeatTimer = nullptr;
    bool _lastCoreStatus = false;
};

} // namespace QuantLabs
