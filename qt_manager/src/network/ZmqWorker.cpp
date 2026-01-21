#include "network/ZmqWorker.h"
#include "protocol/zmq_topics.h"
#include <QDebug>
#include <QCoreApplication>

namespace atrad {

ZmqWorker::ZmqWorker(QObject *parent)
    : QObject(parent),
      _context(1),
      _subscriber(_context, zmq::socket_type::sub) {
}

ZmqWorker::~ZmqWorker() {
    stop();
}

// 引入 chrono
#include <chrono>

void ZmqWorker::process() {
    try {
        _subscriber.connect(zmq_topics::SUB_MARKET_ADDR);
        _subscriber.set(zmq::sockopt::subscribe, zmq_topics::MARKET_DATA);
        _subscriber.set(zmq::sockopt::subscribe, zmq_topics::POSITION_DATA);
        _subscriber.set(zmq::sockopt::subscribe, zmq_topics::ACCOUNT_DATA);
        _subscriber.set(zmq::sockopt::subscribe, zmq_topics::INSTRUMENT_DATA);
        _subscriber.set(zmq::sockopt::subscribe, zmq_topics::ORDER_DATA);
        _subscriber.set(zmq::sockopt::subscribe, zmq_topics::TRADE_DATA);
        
        _running = true;
        qDebug() << "[ZmqWorker] Connected and Subscribed to all topics";

        QThread::msleep(500);
        sendCommand("{\"type\":\"SYNC_STATE\"}");
        
        zmq::pollitem_t items[] = {
            { _subscriber, 0, ZMQ_POLLIN, 0 }
        };
        
        auto lastPingTime = std::chrono::steady_clock::now();
        auto lastCtpActivity = std::chrono::steady_clock::now();
        bool coreStatus = false;
        bool ctpStatus = false;

        while (_running) {
            // 处理队列中的信号槽事件 (例如 sendCommand)
            QCoreApplication::processEvents();
            
            // Poll 100ms
            zmq::poll(items, 1, std::chrono::milliseconds(100));

            if (items[0].revents & ZMQ_POLLIN) {
                zmq::message_t topic_msg;
                zmq::message_t payload_msg;
                
                // 非阻塞接收 (因为 pollin 已经触发)
                (void)_subscriber.recv(topic_msg, zmq::recv_flags::none);
                (void)_subscriber.recv(payload_msg, zmq::recv_flags::none);
                
                std::string topic = std::string(static_cast<char*>(topic_msg.data()), topic_msg.size());
                QString payload = QString::fromUtf8(static_cast<char*>(payload_msg.data()), payload_msg.size());

                if (topic == zmq_topics::MARKET_DATA) {
                    emit tickReceived(payload);
                    lastCtpActivity = std::chrono::steady_clock::now();
                    ctpStatus = true;
                } else if (topic == zmq_topics::POSITION_DATA) {
                    emit positionReceived(payload);
                } else if (topic == zmq_topics::ACCOUNT_DATA) {
                    emit accountReceived(payload);
                    lastCtpActivity = std::chrono::steady_clock::now(); // Account 也是 CTP 活动
                    ctpStatus = true;
                } else if (topic == zmq_topics::INSTRUMENT_DATA) {
                    emit instrumentReceived(payload);
                } else if (topic == zmq_topics::ORDER_DATA) {
                    emit orderReceived(payload);
                } else if (topic == zmq_topics::TRADE_DATA) {
                    emit tradeReceived(payload);
                }
            }
            
            // CTP 超时检查 (20秒无数据认为 CTP 断开或空闲)
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastCtpActivity).count() > 20) {
                ctpStatus = false;
            }

            // Core 心跳检查 (每10秒)
            // 使用 PING 命令，Core 仅回复 PONG，不触发 CTP 查询，安全且轻量。
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastPingTime).count() >= 10) {
                 bool currentCore = false;
                 try {
                     zmq::socket_t req(_context, zmq::socket_type::req);
                     std::string addr = zmq_topics::REQ_CMD_ADDR;
                     
                     req.connect(addr);
                     // 快速超时检测 (300ms)
                     req.set(zmq::sockopt::sndtimeo, 300);
                     req.set(zmq::sockopt::rcvtimeo, 300);
                     req.set(zmq::sockopt::linger, 0); // 重要: 避免关闭时阻塞
                     
                     std::string ping = "{\"type\":\"PING\"}"; 
                     req.send(zmq::message_t(ping.data(), ping.size()), zmq::send_flags::none);
                     
                     zmq::message_t rep;
                     if (req.recv(rep, zmq::recv_flags::none)) {
                        currentCore = true;
                     }
                 } catch (...) { 
                     currentCore = false; 
                 }
                 
                 // Core 刚连上时，触发同步
                 if (currentCore && !coreStatus) {
                     sendCommand("{\"type\":\"SYNC_STATE\"}");
                 }

                 coreStatus = currentCore;
                 if (!coreStatus) ctpStatus = false; // Core 断了 CTP 也断
                 
                 emit statusUpdated(coreStatus, ctpStatus);
                 lastPingTime = now;
            }
        }
    } catch (const zmq::error_t& e) {
        qWarning() << "[ZmqWorker] ZMQ Error:" << e.what();
    }
}

void ZmqWorker::stop() {
    _running = false;
}

void ZmqWorker::sendCommand(const QString& json) {
    qDebug() << "[ZmqWorker] sendCommand called with:" << json;
    try {
        zmq::socket_t requester(_context, zmq::socket_type::req);
        
        // 修复: connect 不支持通配符 *，需要替换为 localhost
        std::string addr = zmq_topics::REQ_CMD_ADDR;
        
        qDebug() << "[ZmqWorker] Connecting to:" << QString::fromStdString(addr);
        requester.connect(addr);
        
        // 设置超时，避免因为 Core 没开而导致 UI 线程阻塞
        requester.set(zmq::sockopt::sndtimeo, 2000);
        requester.set(zmq::sockopt::rcvtimeo, 2000);
        requester.set(zmq::sockopt::linger, 0); // 立即丢弃未发送数据，防止 close 时阻塞

        std::string payload = json.toStdString();
        requester.send(zmq::message_t(payload.data(), payload.size()), zmq::send_flags::none);
        qDebug() << "[ZmqWorker] Command sent, waiting for reply...";

        zmq::message_t reply;
        auto res = requester.recv(reply, zmq::recv_flags::none);
        if (res) {
            QString replyStr = QString::fromUtf8(static_cast<char*>(reply.data()), reply.size());
            qDebug() << "[ZmqWorker] Command Reply:" << replyStr;
        } else {
            qDebug() << "[ZmqWorker] No reply received";
        }
    } catch (const zmq::error_t& e) {
        qWarning() << "[ZmqWorker] Send Command Error:" << e.what();
    }
}

} // namespace atrad
