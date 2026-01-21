#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QThread>
#include <QDebug>
#include "network/ZmqWorker.h"
#include "models/MarketModel.h"
#include "models/PositionModel.h"
#include "models/AccountInfo.h"
#include "models/OrderController.h"
#include "models/OrderModel.h"
#include "models/TradeModel.h"

#include <QFont>

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    
    // 设置应用程序信息（用于 Settings 持久化）
    app.setOrganizationName("ATrader");
    app.setOrganizationDomain("atrader.local");
    app.setApplicationName("qt_manager");
    
    // 设置全局字体
    QFont font("WenQuanYi Micro Hei");
    font.setPixelSize(16);
    app.setFont(font);

    QQmlApplicationEngine engine;

    atrad::MarketModel* marketModel = new atrad::MarketModel(&app);
    atrad::PositionModel* positionModel = new atrad::PositionModel(&app);
    atrad::AccountInfo* accountInfo = new atrad::AccountInfo(&app);
    atrad::OrderController* orderController = new atrad::OrderController(&app);
    atrad::OrderModel* orderModel = new atrad::OrderModel(&app);
    atrad::TradeModel* tradeModel = new atrad::TradeModel(&app);

    qDebug() << "[Main] Created marketModel:" << marketModel << "rows:" << marketModel->rowCount();
    
    // 使用 App 前缀避免与 QML 组件内部属性名冲突
    engine.rootContext()->setContextProperty("AppMarketModel", marketModel);
    engine.rootContext()->setContextProperty("AppPositionModel", positionModel);
    engine.rootContext()->setContextProperty("AppAccountInfo", accountInfo);
    engine.rootContext()->setContextProperty("AppOrderController", orderController);
    engine.rootContext()->setContextProperty("AppOrderModel", orderModel);
    engine.rootContext()->setContextProperty("AppTradeModel", tradeModel);
    
    qDebug() << "[Main] Set context properties";

    // 2. 创建 ZMQ 工作线程
    QThread* workerThread = new QThread();
    atrad::ZmqWorker* worker = new atrad::ZmqWorker();
    worker->moveToThread(workerThread);
    
    // 连接信号
    QObject::connect(workerThread, &QThread::started, worker, &atrad::ZmqWorker::process);
    
    // 分发不同主题的消息
    QObject::connect(worker, &atrad::ZmqWorker::tickReceived, marketModel, &atrad::MarketModel::updateTick);
    // 行情同时也发给持仓模型计算盈亏
    QObject::connect(worker, &atrad::ZmqWorker::tickReceived, positionModel, &atrad::PositionModel::updatePrice);
    
    QObject::connect(worker, &atrad::ZmqWorker::positionReceived, positionModel, &atrad::PositionModel::updatePosition);
    QObject::connect(worker, &atrad::ZmqWorker::accountReceived, accountInfo, &atrad::AccountInfo::updateAccount);
    // 连接合约信息到 MarketModel，确保订阅后能立即显示条目
    QObject::connect(worker, &atrad::ZmqWorker::instrumentReceived, marketModel, &atrad::MarketModel::handleInstrument);
    QObject::connect(worker, &atrad::ZmqWorker::instrumentReceived, positionModel, &atrad::PositionModel::updateInstrument);

    QObject::connect(worker, &atrad::ZmqWorker::instrumentReceived, orderController, &atrad::OrderController::updateInstrument);
    
    QObject::connect(worker, &atrad::ZmqWorker::orderReceived, orderModel, &atrad::OrderModel::onOrderReceived);
    QObject::connect(worker, &atrad::ZmqWorker::tradeReceived, tradeModel, &atrad::TradeModel::onTradeReceived);
    QObject::connect(worker, &atrad::ZmqWorker::positionReceived, orderController, &atrad::OrderController::onPositionReceived);
    
    // 连接状态更新 (Worker -> OrderController)
    QObject::connect(worker, &atrad::ZmqWorker::statusUpdated, 
                     orderController, &atrad::OrderController::updateConnectionStatus,
                     Qt::QueuedConnection);

    // 连接行情到 OrderController 以实现自动跟价
    QObject::connect(worker, &atrad::ZmqWorker::tickReceived, orderController, &atrad::OrderController::onTick);
    
    // 发送指令链路 (UI -> Controller -> ZmqWorker -> Core)
    // 显式使用 QueuedConnection 因为 worker 在另一个线程
    bool connected = QObject::connect(orderController, &atrad::OrderController::orderSent, 
                                     worker, &atrad::ZmqWorker::sendCommand, 
                                     Qt::QueuedConnection);
    qDebug() << "[Main] orderController.orderSent -> worker.sendCommand connection:" << (connected ? "SUCCESS" : "FAILED");
    
    // 连接持仓总盈亏到资金面板
    QObject::connect(positionModel, &atrad::PositionModel::totalProfitChanged, accountInfo, &atrad::AccountInfo::setFloatingProfit);
    
    QObject::connect(&app, &QGuiApplication::aboutToQuit, worker, &atrad::ZmqWorker::stop);
    QObject::connect(&app, &QGuiApplication::aboutToQuit, workerThread, &QThread::quit);
    QObject::connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
    QObject::connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    workerThread->start();

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
