// qmllint disable import
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Qt.labs.settings
import QtMultimedia
import "."

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
        
        // 主题索引
        property int themeIndex: 0
        
        // 分割面板状态
        property int rightPanelWidth: 350
        property int quotePanelHeight: 300
        property int middleRowHeight: 300
        property int orderListWidth: 600
        property int positionListWidth: 600
    }
    
    // 全局交易设置（供所有组件共享）
    Settings {
        id: globalTradeSettings
        category: "Trade"
        property int defaultVolume: 1
        property int defaultPriceType: 0
    }
    
    // 声音设置
    Settings {
        id: soundSettings
        category: "Sound"
        property bool enableOrderSound: true
        property bool enableTradeSound: true
        property int volume: 50
    }

    // 声音效果
    SoundEffect {
        id: soundOrderSuccess
        source: "qrc:/sounds/success1.wav"
        volume: soundSettings.volume / 100.0
    }
    SoundEffect {
        id: soundOrderFail
        source: "qrc:/sounds/fail1.wav"
        volume: soundSettings.volume / 100.0
    }
    SoundEffect {
        id: soundOrderCancel // 使用 success2 作为撤单音
        source: "qrc:/sounds/success2.wav" 
        volume: soundSettings.volume / 100.0
    }
    SoundEffect {
        id: soundTrade
        source: "qrc:/sounds/trade1.wav"
        volume: soundSettings.volume / 100.0
    }

    // 声音信号连接
    Connections {
        target: AppOrderModel
        function onOrderSoundTriggered(type) {
            if (!soundSettings.enableOrderSound) return;
            // console.log("Sound Trigger:" + type);
            if (type === "success") soundOrderSuccess.play();
            else if (type === "fail") soundOrderFail.play();
            else if (type === "cancel") soundOrderCancel.play();
        }
    }

    Connections {
        target: AppTradeModel
        function onTradeSoundTriggered() {
            if (!soundSettings.enableTradeSound) return;
            soundTrade.play();
        }
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

    // ============ 主题系统 ============
    property int themeIndex: appSettings.themeIndex
    
    // 主题配色定义
    readonly property var themes: [
        // 0: 深色经典 - VS Code 风格
        { name: "深色经典", bg: "#1e1e1e", surface: "#252526", surfaceLight: "#2d2d30", border: "#3e3e42", 
          text: "#ffffff", textSec: "#cccccc", accent: "#007acc", success: "#4caf50", danger: "#f44336", warning: "#ff9800" },
        // 1: 文华财经 - 深蓝背景，红涨绿跌，橙黄强调
        { name: "文华财经", bg: "#0c1929", surface: "#0f2137", surfaceLight: "#132a45", border: "#1a3a5c",
          text: "#ffffff", textSec: "#8cb4d8", accent: "#f0a030", success: "#00cc66", danger: "#ff3333", warning: "#ffcc00" },
        // 2: 经典黑金 - 纯黑背景，金色强调，专业交易风格
        { name: "经典黑金", bg: "#0d0d0d", surface: "#141414", surfaceLight: "#1c1c1c", border: "#333333",
          text: "#ffffff", textSec: "#b0b0b0", accent: "#d4a84b", success: "#00b050", danger: "#ff3030", warning: "#ffc000" },
        // 3: 深蓝专业
        { name: "深蓝专业", bg: "#0a1628", surface: "#0d1f3c", surfaceLight: "#132744", border: "#1e3a5f",
          text: "#e8f4ff", textSec: "#a8c5e8", accent: "#3b82f6", success: "#22c55e", danger: "#ef4444", warning: "#f59e0b" },
        // 4: 暖夜护眼
        { name: "暖夜护眼", bg: "#1a1614", surface: "#242019", surfaceLight: "#2e2820", border: "#3d362b",
          text: "#f5e6d3", textSec: "#c9b89a", accent: "#d4a574", success: "#7cb342", danger: "#e57373", warning: "#ffb74d" },
        // 5: 高对比度
        { name: "高对比度", bg: "#000000", surface: "#1a1a1a", surfaceLight: "#2a2a2a", border: "#4a4a4a",
          text: "#ffffff", textSec: "#e0e0e0", accent: "#00bfff", success: "#00ff7f", danger: "#ff4444", warning: "#ffdd00" }
    ]
    
    // 当前主题便捷访问
    readonly property var currentTheme: themes[themeIndex] || themes[0]

    background: Rectangle {
        color: appWindow.currentTheme.bg
    }


    // 菜单栏
    menuBar: MenuBar {
        Menu {
            title: "选项(&O)"
            
            Menu {
                title: "🎨 配色方案"
                
                MenuItem {
                    text: "深色经典" + (appWindow.themeIndex === 0 ? " ✓" : "")
                    onTriggered: { appSettings.themeIndex = 0; appWindow.themeIndex = 0 }
                }
                MenuItem {
                    text: "文华财经" + (appWindow.themeIndex === 1 ? " ✓" : "")
                    onTriggered: { appSettings.themeIndex = 1; appWindow.themeIndex = 1 }
                }
                MenuItem {
                    text: "经典黑金" + (appWindow.themeIndex === 2 ? " ✓" : "")
                    onTriggered: { appSettings.themeIndex = 2; appWindow.themeIndex = 2 }
                }
                
                MenuSeparator {}
                
                MenuItem {
                    text: "深蓝专业" + (appWindow.themeIndex === 3 ? " ✓" : "")
                    onTriggered: { appSettings.themeIndex = 3; appWindow.themeIndex = 3 }
                }
                MenuItem {
                    text: "暖夜护眼" + (appWindow.themeIndex === 4 ? " ✓" : "")
                    onTriggered: { appSettings.themeIndex = 4; appWindow.themeIndex = 4 }
                }
                MenuItem {
                    text: "高对比度" + (appWindow.themeIndex === 5 ? " ✓" : "")
                    onTriggered: { appSettings.themeIndex = 5; appWindow.themeIndex = 5 }
                }
            }
            
            MenuSeparator {}
            
            MenuItem {
                text: "⚙ 设置"
                onTriggered: settingsWin.visible = true
            }
        }
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
                        
                        onRequestConditionOrderPanel: {
                            orderPanel.visible = false
                            conditionOrderPanel.visible = true
                        }
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
                            property var tradeSettings: globalTradeSettings
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
            Layout.preferredHeight: 38
            color: currentTheme.surface
            
            property bool isCoreConnected: AppOrderController.coreConnected
            property bool isCtpConnected: AppOrderController.ctpConnected
            property var accountInfo: AppAccountInfo

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 20

                // 2.1 左侧：资金信息
                RowLayout {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 20
                    
                    Text { 
                        text: AppOrderController.investorId
                        color: currentTheme.warning; font.bold: true; font.pixelSize: 13
                    }
                    
                    Text { 
                        text: "权益: " + (statusBar.accountInfo ? statusBar.accountInfo.equity.toFixed(2) : "--")
                        color: currentTheme.text; font.family: "Monospace"; font.pixelSize: 13; font.bold: true
                    }
                    
                    Text { 
                        text: "可用: " + (statusBar.accountInfo ? statusBar.accountInfo.available.toFixed(2) : "--")
                        color: currentTheme.success; font.family: "Monospace"; font.pixelSize: 13
                    }
                    
                    Text { 
                        text: "盈亏: " + (statusBar.accountInfo ? statusBar.accountInfo.floatingProfit.toFixed(2) : "--")
                        color: (statusBar.accountInfo && statusBar.accountInfo.floatingProfit >= 0) ? currentTheme.danger : currentTheme.success
                        font.family: "Monospace"; font.pixelSize: 13
                    }
                    
                    Text { 
                        text: "平盈: " + (statusBar.accountInfo ? statusBar.accountInfo.closeProfit.toFixed(2) : "--")
                        color: (statusBar.accountInfo && statusBar.accountInfo.closeProfit >= 0) ? currentTheme.danger : currentTheme.success
                        font.family: "Monospace"; font.pixelSize: 13
                    }
                    
                    Text { 
                        text: "占用: " + (statusBar.accountInfo ? statusBar.accountInfo.margin.toFixed(2) : "--")
                        color: currentTheme.warning; font.family: "Monospace"; font.pixelSize: 13
                    }
                }

                // 中间弹簧
                Item { Layout.fillWidth: true }

                // 2.2 右侧：连接状态和时间
                RowLayout {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 15

                    // 连接状态
                    RowLayout {
                        spacing: 5
                        Rectangle {
                            width: 10; height: 10; radius: 5
                            color: statusBar.isCtpConnected ? currentTheme.success : currentTheme.danger
                        }
                        Text { text: "CTP"; color: currentTheme.textSec; font.pixelSize: 13 }
                    }
                    RowLayout {
                        spacing: 5
                        Rectangle {
                            width: 10; height: 10; radius: 5
                            color: statusBar.isCoreConnected ? currentTheme.success : currentTheme.danger
                        }
                        Text { text: "Core"; color: currentTheme.textSec; font.pixelSize: 13 }
                    }

                    // 时间
                    Text {
                        id: timeText
                        color: currentTheme.textSec
                        font.family: "Monospace"
                        font.pixelSize: 13
                        Layout.leftMargin: 10
                        Timer {
                            interval: 1000; running: true; repeat: true
                            onTriggered: timeText.text = Qt.formatDateTime(new Date(), "HH:mm:ss")
                            triggeredOnStart: true
                        }
                    }
                }
            }
        }
    }
    
    // 设置窗口实例
    SettingsWindow {
        id: settingsWin
        mainWindow: appWindow
        property var tradeSettings: globalTradeSettings
        soundSettings: soundSettings
    }
}
