#include "network/CommandWorker.h"
#include "protocol/zmq_topics.h"
#include <QDebug>
#include <QThread>
#include <QTimer>

namespace QuantLabs {

CommandWorker::CommandWorker(QObject *parent)
    : QObject(parent),
      _context(1),
      _requester(_context, zmq::socket_type::req) {
}

CommandWorker::~CommandWorker() {
    _connected = false;
    if (_heartbeatTimer) delete _heartbeatTimer;
    _requester.close();
    _context.close();
}

void CommandWorker::connectToCore() {
    try {
        std::string addr = zmq_topics::Config::instance().getReqCmdAddr();
        qDebug() << "[CommandWorker] Connecting to:" << QString::fromStdString(addr);
        
        _requester.connect(addr);
        
        // 设置默认超时
        _requester.set(zmq::sockopt::sndtimeo, 2000);
        _requester.set(zmq::sockopt::rcvtimeo, 2000);
        _requester.set(zmq::sockopt::linger, 0); // 立即丢弃未发送数据
        
        _connected = true;
        qDebug() << "[CommandWorker] REQ Connected";
    } catch (const zmq::error_t& e) {
        qCritical() << "[CommandWorker] Connect Error:" << e.what();
    }
}

void CommandWorker::startHeartbeat(int intervalMs) {
    if (!_heartbeatTimer) {
        _heartbeatTimer = new QTimer(this);
        connect(_heartbeatTimer, &QTimer::timeout, this, &CommandWorker::sendPing);
    }
    _heartbeatTimer->start(intervalMs);
    qDebug() << "[CommandWorker] Heartbeat started, interval:" << intervalMs << "ms";
}

void CommandWorker::sendPing() {
    // Ping 逻辑：发送 PING，设置短超时 (300ms)
    // 如果失败，认为连接断开
    try {
        if (!_connected) {
            connectToCore();
            if (!_connected) {
                if (_lastCoreStatus) {
                    emit coreStatusUpdated(false);
                    _lastCoreStatus = false;
                }
                return;
            }
        }

        // 设置短超时
        _requester.set(zmq::sockopt::rcvtimeo, 500); 
        _requester.set(zmq::sockopt::sndtimeo, 500);

        std::string ping = "{\"type\":\"PING\"}";
        zmq::message_t msg(ping.data(), ping.size());
        
        bool sent = false;
        try {
            auto res = _requester.send(msg, zmq::send_flags::none);
            sent = res.has_value();
        } catch (...) { sent = false; }

        if (!sent) {
            // 发送失败
            if (_lastCoreStatus) {
                qDebug() << "[CommandWorker] Ping Send Failed";
                emit coreStatusUpdated(false);
                _lastCoreStatus = false;
            }
            // 重建Socket清理状态
            _requester.close();
            _requester = zmq::socket_t(_context, zmq::socket_type::req);
            _connected = false;
            return;
        }

        // 接收
        zmq::message_t reply;
        bool received = false;
        try {
            auto res = _requester.recv(reply, zmq::recv_flags::none);
            received = res.has_value();
        } catch (...) { received = false; }

        if (received) {
            if (!_lastCoreStatus) {
                qDebug() << "[CommandWorker] Core Connected (Ping Success)";
                emit coreStatusUpdated(true);
                _lastCoreStatus = true;
                
                // 首次连上，发送 SYNC_STATE (或者通知 UI 端去请求)
                // 这里我们暂不发送 SYNC，因为 UI 可能更知道何时发生
                // 但是 ZMQWorker 以前是发 SYNC 的。
                // 我们可以 emit 一个 connected 信号，main.cpp 里连到 sendCommand SYNC 
            }
        } else {
            if (_lastCoreStatus) {
                qDebug() << "[CommandWorker] Ping Timeout";
                emit coreStatusUpdated(false);
                _lastCoreStatus = false;
            }
            // Timeout -> Socket state bad -> Reconnect
             _requester.close();
             _requester = zmq::socket_t(_context, zmq::socket_type::req);
             _connected = false;
        }

        // 恢复默认超时
        _requester.set(zmq::sockopt::rcvtimeo, 2000);
        _requester.set(zmq::sockopt::sndtimeo, 2000);

    } catch (const std::exception& e) {
        qWarning() << "[CommandWorker] Ping Exception:" << e.what();
         // 重建
         _requester.close();
         _requester = zmq::socket_t(_context, zmq::socket_type::req);
         _connected = false;
         if (_lastCoreStatus) {
             emit coreStatusUpdated(false);
             _lastCoreStatus = false;
         }
    }
}

void CommandWorker::sendCommand(const QString& json) {
    if (!_connected) {
        connectToCore();
        if (!_connected) {
            qWarning() << "[CommandWorker] Not connected, cannot send command:" << json;
            return;
        }
    }

    // 确保超时设置正确 (2s)
    _requester.set(zmq::sockopt::rcvtimeo, 2000);
    _requester.set(zmq::sockopt::sndtimeo, 2000);

    try {
        std::string payload = json.toStdString();
        zmq::message_t msg(payload.data(), payload.size());
        auto res = _requester.send(msg, zmq::send_flags::none);
        
        if (!res) {
            qWarning() << "[CommandWorker] Send failed (EAGAIN/Timeout)";
             _requester.close();
             _requester = zmq::socket_t(_context, zmq::socket_type::req);
             _connected = false;
             connectToCore(); 
             // Retry sending once
             if (_connected) {
                  zmq::message_t msgRetry(payload.data(), payload.size());
                  _requester.send(msgRetry, zmq::send_flags::none);
                  // Wait for reply below
             } else {
                 return;
             }
        }

        zmq::message_t reply;
        auto recvRes = _requester.recv(reply, zmq::recv_flags::none);
        
        if (recvRes) {
            QString replyStr = QString::fromUtf8(static_cast<char*>(reply.data()), reply.size());
            emit commandReplyReceived(replyStr);
        } else {
            qWarning() << "[CommandWorker] No reply received for command (Timeout)";
            qWarning() << "[CommandWorker] Reconnecting due to timeout...";
            _requester.close();
            _requester = zmq::socket_t(_context, zmq::socket_type::req);
            _connected = false;
            connectToCore();
        }

    } catch (const zmq::error_t& e) {
        qCritical() << "[CommandWorker] ZMQ Exception:" << e.what();
        _requester.close();
        _requester = zmq::socket_t(_context, zmq::socket_type::req);
        _connected = false;
        connectToCore();
    }
}

} // namespace QuantLabs
