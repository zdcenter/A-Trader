#include "network/ZmqWorker.h"
#include "protocol/zmq_topics.h"
#include <QDebug>
#include <QCoreApplication>

namespace QuantLabs {

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
#include <string_view>

void ZmqWorker::process() {
    try {
        // [1] 连接与订阅配置
        // 从配置单例获取 SUB 地址 (用于接收行情)
        std::string sub_addr = zmq_topics::Config::instance().getSubMarketAddr();
        _subscriber.connect(sub_addr);
        qDebug() << "[ZmqWorker] Connecting to:" << QString::fromStdString(sub_addr);
        
        // 订阅各类主题 (Topic)
        // CTP 行情、持仓、账户、合约、报单、成交、策略状态
        _subscriber.set(zmq::sockopt::subscribe, zmq_topics::MARKET_DATA);
        _subscriber.set(zmq::sockopt::subscribe, zmq_topics::MARKET_DATA_BIN); // 二进制急速行情
        _subscriber.set(zmq::sockopt::subscribe, zmq_topics::POSITION_DATA);
        _subscriber.set(zmq::sockopt::subscribe, zmq_topics::ACCOUNT_DATA);
        _subscriber.set(zmq::sockopt::subscribe, zmq_topics::INSTRUMENT_DATA);
        _subscriber.set(zmq::sockopt::subscribe, zmq_topics::ORDER_DATA);
        _subscriber.set(zmq::sockopt::subscribe, zmq_topics::TRADE_DATA);
        _subscriber.set(zmq::sockopt::subscribe, QuantLabs::TOPIC_STRATEGY);
        
        _running = true;
        qDebug() << "[ZmqWorker] Connected and Subscribed to all topics";

        // [2] 初始同步请求
        // 延时 500ms 确保连接就绪，然后触发 CommandWorker 发送 SYNC_STATE 指令
        QThread::msleep(500);
        emit commandRequired("{\"type\":\"SYNC_STATE\"}");
        
        // 定义 Poll 监听项：监听 _subscriber 的 ZMQ_POLLIN (可读) 事件
        zmq::pollitem_t items[] = {
            { _subscriber, 0, ZMQ_POLLIN, 0 }
        };
        
        // 时间戳记录 (用于超时检查)
        auto lastCheckTime = std::chrono::steady_clock::now();
        auto lastCtpActivity = std::chrono::steady_clock::now();
        bool ctpStatus = false;

        int msgCount = 0;
        
        // [3] 接收主循环
        while (_running) {
            // zmq::poll 用于多路复用监听
            // 参数 100ms 是超时时间：
            // - 如果有数据：立即返回
            // - 如果无数据：阻塞等待最多 100ms
            zmq::poll(items, 1, std::chrono::milliseconds(100));

            // [4] 自适应事件处理 (优化 CPU 占用)
            if (items[0].revents & ZMQ_POLLIN) {
                // 有数据（繁忙模式）：每 100 条消息才让出 CPU 处理一次 Qt 事件
                // 这样可以最大化吞吐量
                if (++msgCount >= 100) {
                    QCoreApplication::processEvents();
                    msgCount = 0;
                }
            } else {
                // 无数据（空闲模式）：每次循环都处理 Qt 事件
                // 确保对 stop() 等控制信号的实时响应
                QCoreApplication::processEvents();
            }          

            // [5] 数据接收与处理
            if (items[0].revents & ZMQ_POLLIN) {
                zmq::message_t topic_msg;
                zmq::message_t payload_msg;
                
                // 接收两帧：Topic 帧和 Payload 帧
                // 使用非阻塞标志 zmq::recv_flags::none 是因为 poll 已经确认有数据了
                (void)_subscriber.recv(topic_msg, zmq::recv_flags::none);
                (void)_subscriber.recv(payload_msg, zmq::recv_flags::none);
                
                // 使用 string_view 进行零拷贝的 Topic 匹配
                std::string_view topic(static_cast<char*>(topic_msg.data()), topic_msg.size());
                
                if (topic == zmq_topics::MARKET_DATA_BIN) {
                     // 二进制行情处理 (高性能)
                     if (payload_msg.size() == sizeof(TickData)) {
                        const TickData* pData = static_cast<const TickData*>(payload_msg.data());
                        emit tickReceivedBinary(*pData); // 触发信号
                        lastCtpActivity = std::chrono::steady_clock::now(); // 更新最后活动时间
                     }
                } else {
                    // JSON 数据处理
                    // 使用 fromRawData 避免深拷贝，直接引用 ZMQ 缓冲区进行 JSON 解析
                    QByteArray rawData = QByteArray::fromRawData(static_cast<char*>(payload_msg.data()), payload_msg.size());
                    QJsonDocument doc = QJsonDocument::fromJson(rawData);
                    
                    if (doc.isObject()) {
                        QJsonObject jsonObj = doc.object();

                        // 根据 Topic 分发到不同的信号
                        if (topic == zmq_topics::MARKET_DATA) {
                            emit tickReceived(jsonObj);
                            lastCtpActivity = std::chrono::steady_clock::now();
                        } else if (topic == zmq_topics::POSITION_DATA) {
                            emit positionReceived(jsonObj);
                        } else if (topic == zmq_topics::ACCOUNT_DATA) {
                            emit accountReceived(jsonObj);
                            lastCtpActivity = std::chrono::steady_clock::now(); // 资金变动也算 CTP 活动
                        } else if (topic == zmq_topics::INSTRUMENT_DATA) {
                            emit instrumentReceived(jsonObj);
                        } else if (topic == zmq_topics::ORDER_DATA) {
                            emit orderReceived(jsonObj);
                        } else if (topic == zmq_topics::TRADE_DATA) {
                            emit tradeReceived(jsonObj);
                        } else if (topic == QuantLabs::TOPIC_STRATEGY) {
                            emit conditionOrderReceived(jsonObj);
                        }
                    } 
                }
            }
            
            // [6] 状态超时检查 (每 1 秒检查一次)
            auto now = std::chrono::steady_clock::now();
            if (now - lastCheckTime > std::chrono::seconds(1)) {
                 // 判断逻辑：如果 5 秒内有收到 CTP 相关数据，认为 CTP 连接正常
                 bool newCtpStatus = (now - lastCtpActivity < std::chrono::seconds(5));
                 
                 // 状态变更时发出通知
                 if (newCtpStatus != ctpStatus) {
                     ctpStatus = newCtpStatus;
                     emit ctpStatusUpdated(ctpStatus);
                 }
                 lastCheckTime = now;
            }
        }
    } catch (const zmq::error_t& e) {
        qWarning() << "[ZmqWorker] ZMQ Error:" << e.what();
    }
}

void ZmqWorker::stop() {
    _running = false;
}



} // namespace QuantLabs
