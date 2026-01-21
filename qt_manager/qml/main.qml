import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Qt.labs.settings

ApplicationWindow {
    id: appWindow
    // visible: true // 移除，避免与 visibility 冲突
    title: "A-Trader 现代量化终端 (v3.0)"

    // 持久化设置
    Settings {
        id: appSettings
        category: "Layout"
        
        // 窗口状态
        property int width: 1200
        property int height: 850
        property int x: 100
        property int y: 100
        
        // 分割面板状态
        property int orderPanelWidth: 320
        property int quotePanelHeight: 450
        property int visibility: Window.Windowed
    }
    
    Component.onCompleted: {
        // 手动恢复状态，比较安全
        if (appSettings.width > 0) width = appSettings.width
        if (appSettings.height > 0) height = appSettings.height
        // 简单的屏幕边界检查 (可选，暂省略)
        if (appSettings.x >= 0) x = appSettings.x
        if (appSettings.y >= 0) y = appSettings.y
        
        // 恢复显示模式
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
        appSettings.orderPanelWidth = orderPanel.width
        appSettings.quotePanelHeight = quotePanel.height
    }

    background: Rectangle {
        color: "#1e1e1e" // 深色系背景
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 1. 顶部账户资金面板 (使用独立组件)
        AccountInfoPanel {
            Layout.fillWidth: true
            accountInfo: AppAccountInfo
            ctpConnected: true
            accountId: "SIM-207611"
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#333333" }

        // 2. 主内容区域：使用 SplitView 支持大小调整
        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal
            
            // 分割条样式
            handle: Rectangle {
                implicitWidth: 4
                color: SplitHandle.pressed ? "#007acc" : (SplitHandle.hovered ? "#444444" : "#252526")
            }

            // 左侧：行情 + 持仓 (垂直分割)
            SplitView {
                id: leftSideGroup
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                orientation: Qt.Vertical
                
                handle: Rectangle {
                    implicitHeight: 4
                    color: SplitHandle.pressed ? "#007acc" : (SplitHandle.hovered ? "#444444" : "#252526")
                }

                // ... (内部内容不变，不用写出来) ...
                
                // 实时行情面板
                MarketQuotePanel {
                    id: quotePanel
                    SplitView.fillWidth: true
                    SplitView.preferredHeight: appSettings.quotePanelHeight
                    SplitView.minimumHeight: 200
                    // SplitView.fillHeight: true // 移除此行，让 preferredHeight 生效

                    marketModel: AppMarketModel
                    orderController: AppOrderController

                }

                // 底部多标签页区域 (持仓 | 委托 | 成交)
                ColumnLayout {
                    SplitView.fillWidth: true
                    SplitView.fillHeight: true
                    SplitView.minimumHeight: 150
                    spacing: 0

                    // 标签栏
                    TabBar {
                        id: bottomTabBar
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        
                        background: Rectangle { color: "#252526" }

                        TabButton {
                            text: "持仓 (Positions)"
                            width: implicitWidth + 20
                            contentItem: Text {
                                text: parent.text
                                font: parent.font
                                color: parent.checked ? "white" : "#888888"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                color: parent.checked ? "#1e1e1e" : "#2d2d30"
                                Rectangle { width: parent.width; height: 2; color: "#007acc"; anchors.bottom: parent.bottom; visible: parent.parent.checked }
                            }
                        }
                        
                        TabButton {
                            text: "委托 (Orders)"
                            width: implicitWidth + 20
                            contentItem: Text {
                                text: parent.text
                                font: parent.font
                                color: parent.checked ? "white" : "#888888"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                color: parent.checked ? "#1e1e1e" : "#2d2d30"
                                Rectangle { width: parent.width; height: 2; color: "#007acc"; anchors.bottom: parent.bottom; visible: parent.parent.checked }
                            }
                        }
                        
                        TabButton {
                            text: "成交 (Trades)"
                            width: implicitWidth + 20
                            contentItem: Text {
                                text: parent.text
                                font: parent.font
                                color: parent.checked ? "white" : "#888888"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle {
                                color: parent.checked ? "#1e1e1e" : "#2d2d30"
                                Rectangle { width: parent.width; height: 2; color: "#007acc"; anchors.bottom: parent.bottom; visible: parent.parent.checked }
                            }
                        }
                    }

                    // 内容区：StackLayout
                    StackLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: bottomTabBar.currentIndex
                        
                        // 1. 持仓面板
                        PositionPanel {
                            positionModel: AppPositionModel
                            orderController: AppOrderController
                        }
                        
                        // 2. 报单列表面板
                        OrderListPanel {
                            orderModel: AppOrderModel
                            orderController: AppOrderController
                        }

                        // 3. 成交列表面板
                        TradeListPanel {
                            tradeModel: AppTradeModel
                        }
                    }
                }
            }

            // 右侧：下单面板
            OrderPanel {
                id: orderPanel
                SplitView.preferredWidth: appSettings.orderPanelWidth
                
                SplitView.minimumWidth: 250
                SplitView.maximumWidth: 500
                SplitView.fillHeight: true
                
                orderController: AppOrderController
            }
        }
        
        // 状态栏
        // Status bar
        Rectangle {
            id: statusBar
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            color: "#007acc"
            
            // 直接绑定到 Controller 属性 (自动更新)
            property bool isCoreConnected: AppOrderController.coreConnected
            property bool isCtpConnected: AppOrderController.ctpConnected
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 20
                
                // CTP 连接状态
                Row {
                    spacing: 5
                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: statusBar.isCtpConnected ? "#55ff55" : "#ff5555" // 绿/红
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: statusBar.isCtpConnected ? "CTP: 已连接" : "CTP: 未连接"
                        color: "white"
                        font.pixelSize: 11
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                
                // Core 连接状态
                Row {
                    spacing: 5
                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: statusBar.isCoreConnected ? "#55ff55" : "#ff5555"
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: statusBar.isCoreConnected ? "Core: 已连接" : "Core: 未连接"
                        color: "white"
                        font.pixelSize: 11
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                Item { Layout.fillWidth: true }
                
                // 时间
                Text {
                    id: timeText
                    color: "white"
                    font.pixelSize: 11
                    font.family: "Consolas"
                    
                    Timer {
                        interval: 1000; running: true; repeat: true
                        onTriggered: timeText.text = Qt.formatDateTime(new Date(), "yyyy-MM-dd HH:mm:ss")
                        triggeredOnStart: true
                    }
                }
            }
        }
    }
}
