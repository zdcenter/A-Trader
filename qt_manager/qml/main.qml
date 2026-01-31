// qmllint disable import
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Qt.labs.settings

ApplicationWindow {
    id: appWindow
    // visible: true // 移除，避免与 visibility 冲突
    title: "A-Trader 现代量化终端 (v3.0)"

    // 持久化设置 (Layout Persistence)
    // Saves window geometry and panel sizes to restore them on next launch
    Settings {
        id: appSettings
        category: "Layout"
        
        // 窗口状态
        property int width: 1200
        property int height: 850
        property int x: 100
        property int y: 100
        property int visibility: Window.Windowed
        
        // 分割面板状态
        property int rightPanelWidth: 350
        property int quotePanelHeight: 300
        property int middleRowHeight: 300
        property int orderListWidth: 600
        property int positionListWidth: 600
    }
    
    Component.onCompleted: {
        if (appSettings.width > 0) width = appSettings.width
        if (appSettings.height > 0) height = appSettings.height
        if (appSettings.x >= 0) x = appSettings.x
        if (appSettings.y >= 0) y = appSettings.y
        
        if (appSettings.visibility === Window.Maximized || appSettings.visibility === Window.FullScreen) {
            visibility = appSettings.visibility
        } else {
            visibility = Window.Windowed
        }
    }
    
    onClosing: {
        appSettings.width = width
        appSettings.height = height
        appSettings.x = x
        appSettings.y = y
        appSettings.visibility = visibility
        
        // 保存面板尺寸
        appSettings.rightPanelWidth = rightSidePanel.width
        appSettings.quotePanelHeight = quotePanel.height
        appSettings.middleRowHeight = middleSplitView.height
        appSettings.orderListWidth = orderListPanel.width
        appSettings.positionListWidth = positionPanel.width
    }

    background: Rectangle {
        color: "#1e1e1e" // 深色系背景
    }

    // 主布局容器
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ==================== 主内容区域 (左右分栏) ====================
        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            handle: Rectangle {
                implicitWidth: 4
                color: SplitHandle.pressed ? "#007acc" : (SplitHandle.hovered ? "#444444" : "#252526")
            }

            // -------- 左侧区域 (行情 / 记录) (垂直分栏) --------
            SplitView {
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                orientation: Qt.Vertical

                handle: Rectangle {
                    implicitHeight: 4
                    color: SplitHandle.pressed ? "#007acc" : (SplitHandle.hovered ? "#444444" : "#252526")
                }

                // 1. 顶部：行情表格
                MarketQuotePanel {
                    id: quotePanel
                    SplitView.fillWidth: true
                    SplitView.preferredHeight: appSettings.quotePanelHeight > 0 ? appSettings.quotePanelHeight : parent.height * 0.35
                    SplitView.minimumHeight: 200
                    marketModel: AppMarketModel
                    orderController: AppOrderController
                }

                // 2. 中部：委托记录 | 条件单记录 (水平分栏)
                SplitView {
                    id: middleSplitView
                    SplitView.fillWidth: true
                    SplitView.preferredHeight: appSettings.middleRowHeight > 0 ? appSettings.middleRowHeight : parent.height * 0.35
                    SplitView.minimumHeight: 200
                    orientation: Qt.Horizontal

                    handle: Rectangle {
                        implicitWidth: 4
                        color: SplitHandle.pressed ? "#007acc" : (SplitHandle.hovered ? "#444444" : "#252526")
                    }

                    // 委托记录
                    OrderListPanel {
                        id: orderListPanel
                        SplitView.preferredWidth: appSettings.orderListWidth > 0 ? appSettings.orderListWidth : parent.width * 0.5
                        SplitView.fillHeight: true
                        orderModel: AppOrderModel
                        orderController: AppOrderController
                    }

                    // 条件单记录列表
                    ConditionListPanel {
                        id: conditionListPanel
                        SplitView.fillWidth: true
                        SplitView.fillHeight: true
                        orderController: AppOrderController
                    }
                }

                // 3. 底部：持仓记录 | 成交记录 (水平分栏)
                SplitView {
                    id: bottomSplitView
                    SplitView.fillWidth: true
                    SplitView.fillHeight: true // 占据剩余高度
                    orientation: Qt.Horizontal

                    handle: Rectangle {
                        implicitWidth: 4
                        color: SplitHandle.pressed ? "#007acc" : (SplitHandle.hovered ? "#444444" : "#252526")
                    }

                    // 持仓记录
                    PositionPanel {
                        id: positionPanel
                        SplitView.preferredWidth: appSettings.positionListWidth > 0 ? appSettings.positionListWidth : parent.width * 0.5
                        SplitView.fillHeight: true
                        positionModel: AppPositionModel
                        orderController: AppOrderController
                    }

                    // 成交记录
                    TradeListPanel {
                        SplitView.fillWidth: true
                        SplitView.fillHeight: true
                        tradeModel: AppTradeModel
                    }
                }
            }

            // -------- 右侧区域 (下单 / 条件单面板) --------
            Rectangle {
                id: rightSidePanel
                SplitView.preferredWidth: appSettings.rightPanelWidth > 0 ? appSettings.rightPanelWidth : 350
                SplitView.minimumWidth: 300
                SplitView.maximumWidth: 500
                SplitView.fillHeight: true
                color: "#1e1e1e"
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0
                    
                    // 下单功能切换标签 (普通下单/条件单)
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 35
                        color: "#2d2d30"
                        
                        RowLayout {
                            anchors.fill: parent
                            spacing: 0
                            
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: orderPanel.visible ? "#007acc" : "#2d2d30"
                                Text {
                                    anchors.centerIn: parent
                                    text: "普通下单"
                                    color: "white"
                                    font.pixelSize: 13
                                    font.bold: orderPanel.visible
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        orderPanel.visible = true
                                        conditionOrderPanel.visible = false
                                    }
                                }
                            }
                            
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: conditionOrderPanel.visible ? "#007acc" : "#2d2d30"
                                Text {
                                    anchors.centerIn: parent
                                    text: "条件单"
                                    color: "white"
                                    font.pixelSize: 13
                                    font.bold: conditionOrderPanel.visible
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        orderPanel.visible = false
                                        conditionOrderPanel.visible = true
                                    }
                                }
                            }
                        }
                    }
                    
                    // 下单面板内容容器
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        
                        // 普通下单面板
                        OrderPanel {
                            id: orderPanel
                            anchors.fill: parent
                            visible: true
                            orderController: AppOrderController
                        }
                        
                        // 条件下单面板
                        ConditionOrderPanel {
                            id: conditionOrderPanel
                            anchors.fill: parent
                            visible: false
                            marketModel: AppMarketModel
                            orderController: AppOrderController
                            
                            Connections {
                                target: AppOrderController
                                function onInstrumentIdChanged() {
                                    if (AppOrderController.instrumentId) {
                                        conditionOrderPanel.selectedInstrument = AppOrderController.instrumentId
                                    }
                                }
                                function onPriceChanged() {
                                    if (AppOrderController.instrumentId === conditionOrderPanel.selectedInstrument && AppOrderController.price > 0) {
                                        conditionOrderPanel.selectedLastPrice = AppOrderController.price
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // 2. 底部区域 (状态栏 + 资金信息)
        Rectangle {
            id: statusBar
            Layout.fillWidth: true
            Layout.preferredHeight: 35
            color: "#252526"
            
            property bool isCoreConnected: AppOrderController.coreConnected
            property bool isCtpConnected: AppOrderController.ctpConnected
            property var accountInfo: AppAccountInfo

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 20

                // 2.1 左侧：连接状态和时间
                RowLayout {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 15

                    // 连接状态
                    RowLayout {
                        spacing: 5
                        Rectangle {
                            width: 8; height: 8; radius: 4
                            color: statusBar.isCtpConnected ? "#00FF00" : "#FF0000"
                        }
                        Text { text: "CTP"; color: "#cccccc"; font.pixelSize: 12 }
                    }
                    RowLayout {
                        spacing: 5
                        Rectangle {
                            width: 8; height: 8; radius: 4
                            color: statusBar.isCoreConnected ? "#00FF00" : "#FF0000"
                        }
                        Text { text: "Core"; color: "#cccccc"; font.pixelSize: 12 }
                    }

                    // 时间
                    Text {
                        id: timeText
                        color: "#cccccc"
                        font.family: "Monospace"
                        font.pixelSize: 12
                        Layout.leftMargin: 10
                        Timer {
                            interval: 1000; running: true; repeat: true
                            onTriggered: timeText.text = Qt.formatDateTime(new Date(), "HH:mm:ss")
                            triggeredOnStart: true
                        }
                    }
                }

                // 中间弹簧
                Item { Layout.fillWidth: true }

                // 2.2 右侧：资金信息
                RowLayout {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 20
                    
                    Text { 
                        text: AppOrderController.investorId
                        color: "#fcce03"; font.bold: true; font.pixelSize: 12
                    }
                    
                    Text { 
                        text: "权益: " + (statusBar.accountInfo ? statusBar.accountInfo.equity.toFixed(2) : "--")
                        color: "#ffffff"; font.family: "Monospace"; font.pixelSize: 12; font.bold: true
                    }
                    
                    Text { 
                        text: "可用: " + (statusBar.accountInfo ? statusBar.accountInfo.available.toFixed(2) : "--")
                        color: "#00FF00"; font.family: "Monospace"; font.pixelSize: 12
                    }
                    
                    Text { 
                        text: "盈亏: " + (statusBar.accountInfo ? statusBar.accountInfo.floatingProfit.toFixed(2) : "--")
                        color: (statusBar.accountInfo && statusBar.accountInfo.floatingProfit >= 0) ? "#FF3333" : "#00FF00"
                        font.family: "Monospace"; font.pixelSize: 12
                    }
                    
                    Text { 
                        text: "占用: " + (statusBar.accountInfo ? statusBar.accountInfo.margin.toFixed(2) : "--")
                        color: "#ffcc00"; font.family: "Monospace"; font.pixelSize: 12
                    }
                }
            }
        }
    }
}
