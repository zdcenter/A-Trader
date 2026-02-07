#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include "network/ZmqWorker.h"
#include "network/CommandWorker.h"
#include "models/MarketModel.h"
#include "models/PositionModel.h"
#include "models/AccountInfo.h"
#include "models/OrderController.h"
#include "models/OrderModel.h"
#include "models/TradeModel.h"
#include "protocol/zmq_topics.h"

#include <QFont>
#include <QIcon>  // Added

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/app_icon.png")); // 设置窗口图标
    
    // 设置应用程序信息（用于 Settings 持久化）
    app.setOrganizationName("QuantLabs");
    app.setOrganizationDomain("quantlabs.local");
    app.setApplicationName("qt_manager");
    
    // 加载配置文件
    QFile configFile("config.json");
    if (configFile.open(QIODevice::ReadOnly)) {
        QByteArray data = configFile.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject config = doc.object();
        
        if (config.contains("connection")) {
            QJsonObject conn = config["connection"].toObject();
            QString serverAddr = conn["server_address"].toString("127.0.0.1");
            int pubPort = conn["pub_port"].toInt(5555);
            int repPort = conn["rep_port"].toInt(5556);
            
            // 配置 ZMQ 地址
            QuantLabs::zmq_topics::Config::instance().setServerAddress(serverAddr.toStdString());
            QuantLabs::zmq_topics::Config::instance().setPubPort(pubPort);
            QuantLabs::zmq_topics::Config::instance().setRepPort(repPort);
            
            qDebug() << "[Main] Loaded config: Server =" << serverAddr 
                     << "PubPort =" << pubPort << "RepPort =" << repPort;
        }
    } else {
        qDebug() << "[Main] No config.json found, using default (127.0.0.1:5555/5556)";
    }
    
    // 从 QSettings 加载字体大小
    QSettings settings;
    settings.beginGroup("UI");  // UI 设置
    int fontSize = settings.value("fontSize", 16).toInt();
    settings.endGroup();
    
    // 设置全局字体（使用跨平台字体）
    QFont font;
    font.setPixelSize(fontSize);
    font.setFamily("sans-serif");  // 跨平台默认字体
    app.setFont(font);
    
    qRegisterMetaType<QuantLabs::TickData>("TickData");
    
    qDebug() << "[Main] Font size:" << fontSize;

    QQmlApplicationEngine engine;

    QuantLabs::MarketModel* marketModel = new QuantLabs::MarketModel(&app);
    QuantLabs::PositionModel* positionModel = new QuantLabs::PositionModel(&app);
    QuantLabs::AccountInfo* accountInfo = new QuantLabs::AccountInfo(&app);
    QuantLabs::OrderController* orderController = new QuantLabs::OrderController(&app);
    QuantLabs::OrderModel* orderModel = new QuantLabs::OrderModel(&app);
    QuantLabs::TradeModel* tradeModel = new QuantLabs::TradeModel(&app);

    qDebug() << "[Main] Created marketModel:" << marketModel << "rows:" << marketModel->rowCount();
    
    // 使用 App 前缀避免与 QML 组件内部属性名冲突
    engine.rootContext()->setContextProperty("AppMarketModel", marketModel);
    engine.rootContext()->setContextProperty("AppPositionModel", positionModel);
    engine.rootContext()->setContextProperty("AppAccountInfo", accountInfo);
    engine.rootContext()->setContextProperty("AppOrderController", orderController);
    engine.rootContext()->setContextProperty("AppOrderModel", orderModel);
    engine.rootContext()->setContextProperty("AppTradeModel", tradeModel);
    
    qDebug() << "[Main] Set context properties";

// 2. 创建 ZMQ 工作线程 (Market Data)
    QThread* workerThread = new QThread();
    QuantLabs::ZmqWorker* worker = new QuantLabs::ZmqWorker();
    worker->moveToThread(workerThread);
    
    // 3. 创建 Command 工作线程 (REQ-REP)
    QThread* commandThread = new QThread();
    QuantLabs::CommandWorker* commandWorker = new QuantLabs::CommandWorker();
    commandWorker->moveToThread(commandThread);

    // 连接信号 - Market Worker
    QObject::connect(workerThread, &QThread::started, worker, &QuantLabs::ZmqWorker::process);
    
    // 连接信号 - Command Worker
    QObject::connect(commandThread, &QThread::started, commandWorker, &QuantLabs::CommandWorker::connectToCore);

    // 分发不同主题的消息
    QObject::connect(worker, &QuantLabs::ZmqWorker::tickReceived, marketModel, &QuantLabs::MarketModel::updateTick);
    QObject::connect(worker, &QuantLabs::ZmqWorker::tickReceivedBinary, marketModel, &QuantLabs::MarketModel::updateTickBinary);

    // 行情同时也发给持仓模型计算盈亏
    QObject::connect(worker, &QuantLabs::ZmqWorker::tickReceived, positionModel, &QuantLabs::PositionModel::updatePrice);
    QObject::connect(worker, &QuantLabs::ZmqWorker::tickReceivedBinary, positionModel, &QuantLabs::PositionModel::updatePriceBinary);

    QObject::connect(worker, &QuantLabs::ZmqWorker::positionReceived, positionModel, &QuantLabs::PositionModel::updatePosition);
    QObject::connect(worker, &QuantLabs::ZmqWorker::accountReceived, accountInfo, &QuantLabs::AccountInfo::updateAccount);
    // 连接合约信息到 MarketModel，确保订阅后能立即显示条目
    QObject::connect(worker, &QuantLabs::ZmqWorker::instrumentReceived, marketModel, &QuantLabs::MarketModel::handleInstrument);
    QObject::connect(worker, &QuantLabs::ZmqWorker::instrumentReceived, positionModel, &QuantLabs::PositionModel::updateInstrument);

    QObject::connect(worker, &QuantLabs::ZmqWorker::instrumentReceived, orderController, &QuantLabs::OrderController::updateInstrument);
    
    QObject::connect(worker, &QuantLabs::ZmqWorker::orderReceived, orderModel, &QuantLabs::OrderModel::onOrderReceived);
    QObject::connect(worker, &QuantLabs::ZmqWorker::tradeReceived, tradeModel, &QuantLabs::TradeModel::onTradeReceived);
    QObject::connect(worker, &QuantLabs::ZmqWorker::positionReceived, orderController, &QuantLabs::OrderController::onPositionReceived);
    
    // 连接状态更新 (Worker -> OrderController)
    // 连接状态更新 (Separated)
    QObject::connect(commandWorker, &QuantLabs::CommandWorker::coreStatusUpdated,
                     orderController, &QuantLabs::OrderController::updateCoreStatus,
                     Qt::QueuedConnection);

    QObject::connect(worker, &QuantLabs::ZmqWorker::ctpStatusUpdated, 
                     orderController, &QuantLabs::OrderController::updateCtpStatus,
                     Qt::QueuedConnection);

    // 启动心跳 (跨线程调用)
    QMetaObject::invokeMethod(commandWorker, "startHeartbeat", Qt::QueuedConnection, Q_ARG(int, 5000));

    // 连接行情到 OrderController 以实现自动跟价
    QObject::connect(worker, &QuantLabs::ZmqWorker::tickReceived, orderController, &QuantLabs::OrderController::onTick);
    QObject::connect(worker, &QuantLabs::ZmqWorker::tickReceivedBinary, orderController, &QuantLabs::OrderController::onTickBinary);
    
    // 发送指令链路 (UI -> Controller -> CommandWorker -> Core)
    // 显式使用 QueuedConnection (跨线程)
    bool connected = QObject::connect(orderController, &QuantLabs::OrderController::orderSent, 
                                     commandWorker, &QuantLabs::CommandWorker::sendCommand, 
                                     Qt::QueuedConnection);
    qDebug() << "[Main] orderController.orderSent -> commandWorker.sendCommand connection:" << (connected ? "SUCCESS" : "FAILED");
    
    // Market Worker 需要发送指令时 (如 SYNC_STATE)，转给 Command Worker
    QObject::connect(worker, &QuantLabs::ZmqWorker::commandRequired,
                     commandWorker, &QuantLabs::CommandWorker::sendCommand,
                     Qt::QueuedConnection);

    // 连接持仓总盈亏到资金面板
    QObject::connect(positionModel, &QuantLabs::PositionModel::totalProfitChanged, accountInfo, &QuantLabs::AccountInfo::setFloatingProfit);
    QObject::connect(worker, &QuantLabs::ZmqWorker::conditionOrderReceived, orderController, &QuantLabs::OrderController::onConditionOrderReturn);
    
    // Cleanup Market Worker
    QObject::connect(&app, &QGuiApplication::aboutToQuit, worker, &QuantLabs::ZmqWorker::stop);
    QObject::connect(&app, &QGuiApplication::aboutToQuit, workerThread, &QThread::quit);
    QObject::connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
    QObject::connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    // Cleanup Command Worker
    // CommandWorker 没有 loop，只需要 quit 线程
    QObject::connect(&app, &QGuiApplication::aboutToQuit, commandThread, &QThread::quit);
    QObject::connect(commandThread, &QThread::finished, commandWorker, &QObject::deleteLater);
    QObject::connect(commandThread, &QThread::finished, commandThread, &QObject::deleteLater);

    workerThread->start();
    commandThread->start();

    // 3. 加载 QML
    const QUrl url(QStringLiteral("qrc:/main.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
