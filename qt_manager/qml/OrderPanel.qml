// qmllint disable import
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."
import Qt.labs.settings

/**
 * 下单面板组件
 * 功能：
 * - 显示当前选中的合约
 * - 价格和数量输入
 * - 预估保证金和手续费
 * - 买开、卖开、平仓按钮
 */
Rectangle {
    id: root
    
    // 对外暴露的属性
    property var orderController
    
    function getPriceType() {
        if (priceTypeCombo.currentIndex === 1) return "MARKET"
        if (priceTypeCombo.currentIndex === 2) return "OPPONENT"
        return "LIMIT"
    }
    
    color: "#252526"
    
    // 点击背景抢占焦点 (取消其他面板的高亮)
    MouseArea {
        anchors.fill: parent
        z: -1
        onPressed: root.forceActiveFocus()
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15
        
        // 标题栏 + 当前选中合约
        RowLayout {
            spacing: 10
            Text {
                text: "快速下单"
                color: "white"
                font.pixelSize: 22 // 18 -> 22
                font.bold: true
            }
            
            Item { Layout.fillWidth: true }
            
            // 当前选中合约
            Text {
                text: {
                    if (!orderController) return "未选择"
                    return orderController.instrumentId === "" ? "未选择" : orderController.instrumentId
                }
                color: "#569cd6"
                font.pixelSize: 24 // 18 -> 24
                font.bold: true
                font.family: "Consolas"
            }
        }
        
        // 策略选择栏
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            
            Text {
                text: "策略类型"
                font.pixelSize: 14 // 12 -> 14
                font.bold: true
                color: "#dddddd"
            }
            ComboBox {
                id: strategyCombo
                Layout.fillWidth: true
                Layout.preferredHeight: 40 // 32 -> 40
                font.pixelSize: 16         // 15 -> 16
                
                // 使用 OrderController 中的策略列表
                model: orderController ? orderController.strategyList : []
                textRole: "name"
                
                // 辅助函数：根据ID查找索引
                function getStrategyIndex(sId) {
                    if (!sId) return 0
                    for (var i = 0; i < count; ++i) {
                        var item = (orderController && orderController.strategyList) ? orderController.strategyList[i] : null
                        if (item && item.id === sId) return i
                    }
                    return 0
                }

                // 核心修复：使用直接绑定，确保 currentIndex 始终跟随后端状态
                currentIndex: getStrategyIndex(orderController ? orderController.currentStrategy : "")

                // 监听手动切换
                onActivated: {
                    var item = (orderController && orderController.strategyList) ? orderController.strategyList[index] : null
                    if (item) {
                        console.log("[OrderPanel] User Select:", item.id)
                        if (orderController.currentStrategy !== item.id) {
                             orderController.currentStrategy = item.id
                             // 持久化
                             settings.lastStrategyId = item.id
                        }
                    }
                }

                // 调试：监听后端变化并打印
                Connections {
                    target: orderController
                    function onCurrentStrategyChanged() {
                        console.log("[OrderPanel] Backend Strategy Changed to:", orderController.currentStrategy)
                        // Binding automatically updates currentIndex, just logging here
                    }
                }
                
                // 确保初始加载
                Component.onCompleted: {
                    if (settings.lastStrategyId && orderController) {
                        console.log("[OrderPanel] Restore Init:", settings.lastStrategyId)
                         // 如果当前后端没值，我们推上去；如果有值，Binding会拉下来
                        if (!orderController.currentStrategy) {
                            orderController.currentStrategy = settings.lastStrategyId
                        }
                    }
                }
                
                // 样式完全复刻 ConditionOrderPanel
                contentItem: Text {
                    leftPadding: 15
                    rightPadding: strategyCombo.indicator.width + strategyCombo.spacing
                    text: strategyCombo.displayText
                    font: strategyCombo.font
                    color: "#ffffff"
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                background: Rectangle {
                    color: "#1e1e1e" // ConditionPanel Color
                    border.color: strategyCombo.activeFocus ? "#2196f3" : "#555555"
                    border.width: 2      // ConditionPanel Width
                    radius: 6            // ConditionPanel Radius
                }
                
                // 保持下拉列表样式美观
                delegate: ItemDelegate {
                    width: strategyCombo.width
                    contentItem: Text {
                        text: modelData.name 
                        color: parent.highlighted ? "#ffffff" : "#cccccc"
                        font: strategyCombo.font
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 15
                    }
                    background: Rectangle {
                        color: parent.highlighted ? "#2196f3" : "transparent"
                    }
                }
            }
            
            Settings {
                id: settings
                category: "OrderPanel"
                property string lastStrategyId: "" 
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 300 // 220 -> 300 (给五档行情更多空间)
            color: "#1e1e1e"
            radius: 8 // 6 -> 8
            clip: true
            
            Column {
                anchors.fill: parent
                anchors.margins: 10 // 8 -> 10
                spacing: 4 // 2 -> 4
                
                // 卖盘 (Ask 5 -> Ask 1)
                Repeater {
                    model: 5
                    delegate: RowLayout {
                        width: parent.width
                        height: 26 // 20 -> 26
                        property int dataIndex: 4 - index 
                        
                        Text { 
                            text: "卖" + (dataIndex + 1)
                            color: "#999999"
                            font.pixelSize: 13 // 11 -> 13
                            Layout.preferredWidth: 36
                        }
                        
                        Text {
                            text: {
                                if (!orderController || !orderController.askPrices || orderController.askPrices.length <= dataIndex) return "-"
                                var p = orderController.askPrices[dataIndex]
                                return p > 0.0001 && p < 100000000 ? p.toFixed(2) : "-"
                            }
                            color: "#ff5252" // 鲜艳红
                            font.family: "Consolas"
                            font.pixelSize: 16 // 13 -> 16
                            font.bold: true
                            Layout.preferredWidth: 80
                            horizontalAlignment: Text.AlignRight
                        }
                        
                        // 量 + 柱状图
                        Item {
                            Layout.fillWidth: true
                            height: 16
                            
                            property int vol: {
                                if (!orderController || !orderController.askVolumes || orderController.askVolumes.length <= dataIndex) return 0
                                return orderController.askVolumes[dataIndex] || 0
                            }
                            
                            // 柱状图
                            Rectangle {
                                anchors.right: parent.right
                                height: parent.height
                                width: parent.width * Math.min(parent.vol / 500.0, 1.0) 
                                color: "#33F44336" // rgba(244, 67, 54, 0.2)
                                radius: 2
                            }
                            
                            Text {
                                anchors.right: parent.right
                                anchors.rightMargin: 4
                                anchors.verticalCenter: parent.verticalCenter
                                text: parent.vol
                                color: "#cccccc"
                                font.family: "Consolas"
                                font.pixelSize: 11
                            }
                        }
                    }
                }
                
                // 分割线
                Rectangle { 
                    width: parent.width; height: 1; color: "#444444" 
                    Layout.topMargin: 4; Layout.bottomMargin: 4
                }
                
                // 买盘 (Bid 1 -> Bid 5)
                Repeater {
                    model: 5
                    delegate: RowLayout {
                        width: parent.width
                        height: 26 // 20 -> 26
                        property int dataIndex: index 
                        
                        Text { 
                            text: "买" + (dataIndex + 1)
                            color: "#999999"
                            font.pixelSize: 13 // 11 -> 13
                            Layout.preferredWidth: 36
                        }
                        
                        Text {
                            text: {
                                if (!orderController || !orderController.bidPrices || orderController.bidPrices.length <= dataIndex) return "-"
                                var p = orderController.bidPrices[dataIndex]
                                return p > 0.0001 && p < 100000000 ? p.toFixed(2) : "-" 
                            }
                            color: "#69f0ae" // 鲜艳绿
                            font.family: "Consolas"
                            font.pixelSize: 16 // 13 -> 16
                            font.bold: true
                            Layout.preferredWidth: 80
                            horizontalAlignment: Text.AlignRight
                        }
                        
                        // 量 + 柱状图
                        Item {
                            Layout.fillWidth: true
                            height: 16
                            
                            property int vol: {
                                if (!orderController || !orderController.bidVolumes || orderController.bidVolumes.length <= dataIndex) return 0
                                return orderController.bidVolumes[dataIndex] || 0
                            }
                            
                            Rectangle {
                                anchors.right: parent.right
                                height: parent.height
                                width: parent.width * Math.min(parent.vol / 500.0, 1.0) 
                                color: "#334CAF50" // rgba(76, 175, 80, 0.2)
                                radius: 2
                            }
                            
                            Text {
                                anchors.right: parent.right
                                anchors.rightMargin: 4
                                anchors.verticalCenter: parent.verticalCenter
                                text: parent.vol
                                color: "#cccccc"
                                font.family: "Consolas"
                                font.pixelSize: 11
                            }
                        }
                    }
                }
            }
        }
        // 价格和数量输入区域
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 110 // Increased height for generous layout
            color: "#1e1e1e"
            radius: 8
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15 // More breathing room
                spacing: 15
                
                // --- 价格行 ---
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    
                    // 标签 + 类型选择
                    Text {
                        text: "价格"
                        color: "#cccccc"
                        font.pixelSize: 14
                        font.bold: true
                    }
                    
                    ComboBox {
                        id: priceTypeCombo
                        Layout.preferredWidth: 100 // 80 -> 100
                        Layout.preferredHeight: 36 // 20 -> 36
                        font.pixelSize: 14 // 12 -> 14
                        model: ["限价", "市价", "对手价"]
                        currentIndex: 0
                        
                        delegate: ItemDelegate {
                            width: parent.width
                            height: 30
                            contentItem: Text {
                                text: modelData
                                color: "white"
                                font: priceTypeCombo.font
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: 8
                            }
                            background: Rectangle {
                                color: parent.highlighted ? "#007acc" : "#252526"
                            }
                        }
                        contentItem: Text {
                            leftPadding: 8
                            rightPadding: priceTypeCombo.indicator.width + 5
                            text: priceTypeCombo.displayText
                            font: priceTypeCombo.font
                            color: "#cccccc"
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        background: Rectangle {
                            implicitWidth: 100
                            implicitHeight: 36
                            color: "#333333"
                            border.color: "#555555"
                            radius: 4
                        }
                    }
                    
                    // 价格输入框 + 微调
                    RowLayout {
                        Layout.fillWidth: true
                        height: 48 // 36 -> 48
                        spacing: 2
                        
                        TextField {
                            id: priceInput
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            
                            enabled: priceTypeCombo.currentIndex === 0
                            text: {
                                if (priceTypeCombo.currentIndex === 1) return "市价下单"
                                if (priceTypeCombo.currentIndex === 2) return "对手价下单"
                                if (!orderController) return ""
                                return orderController.price > 0 ? orderController.price.toString() : ""
                            }
                            placeholderText: "跟价中..."
                            font.pixelSize: 20 // 16 -> 20
                            font.family: "Consolas"
                            font.bold: true
                            horizontalAlignment: Text.AlignRight
                            leftPadding: 12
                            rightPadding: 12
                            
                            onTextEdited: {
                                if (!orderController) return
                                var val = parseFloat(text)
                                if (!isNaN(val)) {
                                    orderController.setPriceOriginal(val)
                                    orderController.setManualPrice(true)
                                }
                            }
                            
                            background: Rectangle {
                                color: priceTypeCombo.currentIndex !== 0 ? "#333333" : "#2a2a2a"
                                radius: 4
                                border.color: {
                                    if (priceTypeCombo.currentIndex !== 0) return "#333333"
                                    if (!orderController) return "#444444"
                                    return orderController.isAutoPrice ? "#4caf50" : "#444444"
                                }
                                border.width: 1
                            }
                            
                            color: {
                                if (priceTypeCombo.currentIndex !== 0) return "#aaaaaa"
                                if (!orderController) return "white"
                                return orderController.isAutoPrice ? "#4caf50" : "white"
                            }
                            
                            // "A" (自动) 按钮
                            Text {
                                text: "A"
                                visible: priceTypeCombo.currentIndex === 0 && orderController && !orderController.isAutoPrice
                                color: "#4caf50"
                                font.bold: true
                                font.pixelSize: 14
                                anchors.right: parent.right
                                anchors.rightMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (orderController) {
                                            orderController.setManualPrice(false)
                                            priceInput.focus = false
                                        }
                                    }
                                }
                            }
                        }
                        
                        // 微调按钮
                        Column {
                            visible: priceTypeCombo.currentIndex === 0
                            Layout.preferredWidth: 32 // 24 -> 32
                            Layout.fillHeight: true
                            spacing: 1
                            
                            Rectangle {
                                width: 32
                                height: 23
                                color: priceUpArea.containsMouse ? "#444444" : "#333333"
                                radius: 2
                                Text { anchors.centerIn: parent; text: "▴"; color: "#cccccc"; font.pixelSize: 16 }
                                MouseArea {
                                    id: priceUpArea; anchors.fill: parent; hoverEnabled: true
                                    onClicked: {
                                        if (orderController) {
                                            var tick = orderController.priceTick
                                            var val = parseFloat(priceInput.text) || orderController.price
                                            var p = Math.round((val + tick) / tick) * tick
                                            orderController.setPriceOriginal(Number(p.toFixed(4)))
                                            orderController.setManualPrice(true)
                                        }
                                    }
                                }
                            }
                            Rectangle {
                                width: 32
                                height: 23
                                color: priceDownArea.containsMouse ? "#444444" : "#333333"
                                radius: 2
                                Text { anchors.centerIn: parent; text: "▾"; color: "#cccccc"; font.pixelSize: 16 }
                                MouseArea {
                                    id: priceDownArea; anchors.fill: parent; hoverEnabled: true
                                    onClicked: {
                                        if (orderController) {
                                            var tick = orderController.priceTick
                                            var val = parseFloat(priceInput.text) || orderController.price
                                            var p = val - tick
                                            if (p < 0) p = 0
                                            p = Math.round(p / tick) * tick
                                            orderController.setPriceOriginal(Number(p.toFixed(4)))
                                            orderController.setManualPrice(true)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                
                // --- 数量行 ---
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    
                    Text {
                        text: "数量"
                        color: "#cccccc"
                        font.pixelSize: 14
                        font.bold: true
                        Layout.preferredWidth: priceTypeCombo.width // Align with combo above? Actually price label + combo. 
                        // Just let it flow naturally or fix width. The label "数量" corresponds to "价格" + Combo? No.
                        // Let's just keep simple label.
                    }
                    
                    RowLayout {
                        Layout.fillWidth: true
                        height: 48 // 36 -> 48
                        spacing: 2
                        
                        TextField {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            text: orderController ? orderController.volume.toString() : "1"
                            font.pixelSize: 20 // 16 -> 20
                            font.family: "Consolas"
                            font.bold: true
                            horizontalAlignment: Text.AlignRight
                            leftPadding: 12
                            rightPadding: 12
                            validator: IntValidator { bottom: 1; top: 9999 }
                            
                            onTextEdited: {
                                var v = parseInt(text)
                                if (!isNaN(v) && v > 0 && orderController) {
                                    orderController.volume = v
                                }
                            }
                            
                            background: Rectangle {
                                color: "#2a2a2a"
                                radius: 4
                                border.color: "#444444"
                                border.width: 1
                            }
                            color: "white"
                        }
                        
                        Column {
                            Layout.preferredWidth: 32 // 24 -> 32
                            Layout.fillHeight: true
                            spacing: 1
                            
                            Rectangle {
                                width: 32
                                height: 23
                                color: volUpArea.containsMouse ? "#444444" : "#333333"
                                radius: 2
                                Text { anchors.centerIn: parent; text: "▴"; color: "#cccccc"; font.pixelSize: 16 }
                                MouseArea {
                                    id: volUpArea; anchors.fill: parent; hoverEnabled: true
                                    onClicked: if (orderController) orderController.volume++
                                }
                            }
                            Rectangle {
                                width: 32
                                height: 23
                                color: volDownArea.containsMouse ? "#444444" : "#333333"
                                radius: 2
                                Text { anchors.centerIn: parent; text: "▾"; color: "#cccccc"; font.pixelSize: 16 }
                                MouseArea {
                                    id: volDownArea; anchors.fill: parent; hoverEnabled: true
                                    onClicked: if (orderController && orderController.volume > 1) orderController.volume--
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // 下单操作区域 (仿快期布局：两行选择 + 一个主按钮)
        ColumnLayout {
            Layout.fillWidth: true
            Layout.topMargin: 10
            spacing: 12
            
            // 内部状态跟踪
            property string selectedDirection: "BUY" 
            property string selectedOffset: "OPEN"

            // 第一行：买卖方向
            RowLayout {
                spacing: 15
                Text { 
                    text: "买卖" 
                    color: "#cccccc" 
                    font.pixelSize: 14 
                    font.bold: true 
                    Layout.preferredWidth: 32
                }
                
                // 买入按钮
                Rectangle {
                    Layout.fillWidth: true
                    height: 32
                    color: parent.parent.selectedDirection === "BUY" ? "#d32f2f" : "#333333"
                    radius: 4
                    border.color: parent.parent.selectedDirection === "BUY" ? "#b71c1c" : "#555555"
                    border.width: 1
                    
                    Row {
                        anchors.centerIn: parent
                        spacing: 6
                        // 模拟单选框圆圈
                        Rectangle {
                            width: 14; height: 14; radius: 7
                            color: "transparent"
                            border.color: "white"
                            border.width: 1
                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: "white"
                                anchors.centerIn: parent
                                visible: parent.parent.parent.parent.selectedDirection === "BUY"
                            }
                        }
                        Text { text: "买入"; color: "white"; font.bold: true; font.pixelSize: 14 }
                    }
                    MouseArea { 
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: parent.parent.parent.selectedDirection = "BUY"
                    }
                }
                
                // 卖出按钮
                Rectangle {
                    Layout.fillWidth: true
                    height: 32
                    color: parent.parent.selectedDirection === "SELL" ? "#1976d2" : "#333333"
                    radius: 4
                    border.color: parent.parent.selectedDirection === "SELL" ? "#0d47a1" : "#555555"
                    border.width: 1
                    
                     Row {
                        anchors.centerIn: parent
                        spacing: 6
                        Rectangle {
                            width: 14; height: 14; radius: 7
                            color: "transparent"
                            border.color: "white"
                            border.width: 1
                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: "white"
                                anchors.centerIn: parent
                                visible: parent.parent.parent.parent.selectedDirection === "SELL"
                            }
                        }
                        Text { text: "卖出"; color: "white"; font.bold: true; font.pixelSize: 14 }
                    }
                    MouseArea { 
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: parent.parent.parent.selectedDirection = "SELL"
                    }
                }
            }
            
            // 第二行：开平属性
            RowLayout {
                spacing: 15
                Text { 
                    text: "开平" 
                    color: "#cccccc" 
                    font.pixelSize: 14 
                    font.bold: true 
                    Layout.preferredWidth: 32
                }
                
                // 开仓
                Rectangle {
                    Layout.fillWidth: true
                    height: 32
                    color: parent.parent.selectedOffset === "OPEN" ? "#ffd600" : "#333333"
                    radius: 4
                    border.color: parent.parent.selectedOffset === "OPEN" ? "#fbc02d" : "#555555"
                    border.width: 1
                    
                    Row {
                        anchors.centerIn: parent
                        spacing: 6
                        Rectangle {
                            width: 14; height: 14; radius: 7
                            color: "transparent"
                            border.color: parent.parent.parent.parent.selectedOffset === "OPEN" ? "black" : "white"
                            border.width: 1
                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: parent.parent.parent.parent.selectedOffset === "OPEN" ? "black" : "white"
                                anchors.centerIn: parent
                                visible: parent.parent.parent.parent.selectedOffset === "OPEN"
                            }
                        }
                        Text { 
                            text: "开仓"
                            color: parent.parent.parent.parent.selectedOffset === "OPEN" ? "black" : "white" 
                            font.bold: true
                            font.pixelSize: 14 
                        }
                    }
                    MouseArea { 
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor 
                        onClicked: parent.parent.parent.selectedOffset = "OPEN"
                    }
                }
                
                // 平今
                Rectangle {
                    Layout.fillWidth: true
                    height: 32
                    color: parent.parent.selectedOffset === "CLOSETODAY" ? "#388e3c" : "#333333"
                    radius: 4
                    border.color: parent.parent.selectedOffset === "CLOSETODAY" ? "#2e7d32" : "#555555"
                    border.width: 1
                    
                    Row {
                        anchors.centerIn: parent
                        spacing: 6
                        Rectangle {
                            width: 14; height: 14; radius: 7
                            color: "transparent"
                            border.color: "white"
                            border.width: 1
                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: "white"
                                anchors.centerIn: parent
                                visible: parent.parent.parent.parent.selectedOffset === "CLOSETODAY"
                            }
                        }
                        Text { text: "平今"; color: "white"; font.bold: true; font.pixelSize: 14 }
                    }
                    MouseArea { 
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor 
                        onClicked: parent.parent.parent.selectedOffset = "CLOSETODAY"
                    }
                }
                
                // 平仓
                Rectangle {
                    Layout.fillWidth: true
                    height: 32
                    color: parent.parent.selectedOffset === "CLOSE" ? "#388e3c" : "#333333"
                    radius: 4
                    border.color: parent.parent.selectedOffset === "CLOSE" ? "#2e7d32" : "#555555"
                    border.width: 1
                    
                    Row {
                        anchors.centerIn: parent
                        spacing: 6
                        Rectangle {
                            width: 14; height: 14; radius: 7
                            color: "transparent"
                            border.color: "white"
                            border.width: 1
                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: "white"
                                anchors.centerIn: parent
                                visible: parent.parent.parent.parent.selectedOffset === "CLOSE"
                            }
                        }
                        Text { text: "平仓"; color: "white"; font.bold: true; font.pixelSize: 14 }
                    }
                    MouseArea { 
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor 
                        onClicked: parent.parent.parent.selectedOffset = "CLOSE"
                    }
                }
            }
            
            // 主下单按钮
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                Layout.topMargin: 8
                
                text: {
                    var d = parent.selectedDirection === "BUY" ? "买入" : "卖出"
                    var o = ""
                    if (parent.selectedOffset === "OPEN") o = "开仓"
                    else if (parent.selectedOffset === "CLOSETODAY") o = "平今"
                    else o = "平仓"
                    return d + o + " 下单"
                }
                
                background: Rectangle {
                    color: parent.parent.selectedDirection === "BUY" ? "#d32f2f" : "#1976d2" // Red for Buy, Blue for Sell
                    radius: 6
                    border.color: Qt.lighter(color, 1.1)
                    border.width: 1
                }
                
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    font.bold: true
                    font.pixelSize: 20
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    // Add shadow for better visibility
                    style: Text.Outline
                    styleColor: parent.parent.selectedDirection === "BUY" ? "#b71c1c" : "#0d47a1"
                }
                
                onClicked: {
                    if (orderController) {
                        orderController.sendOrder(parent.selectedDirection, parent.selectedOffset, getPriceType())
                    }
                }
            }
        }
        
        // 资金信息显示区域 (预估保证金/手续费)
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80 // 60 -> 80
            color: "#1e1e1e"
            radius: 8 // 4 -> 8
            border.color: "#333333"
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20 // 12 -> 20
                anchors.rightMargin: 20
                
                // 预估保证金 (左侧对齐)
                ColumnLayout {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 4
                    
                    Text { 
                        text: "预估保证金"
                        color: "#999999" // Slightly lighter
                        font.pixelSize: 14 // 12 -> 14
                        font.bold: true
                    }
                    
                    Text { 
                        text: "¥ " + (orderController ? orderController.estimatedMargin.toFixed(0) : "0")
                        color: "#eaa46e" // 金色
                        font.pixelSize: 22 // 18 -> 22
                        font.family: "Consolas"
                        font.bold: true 
                    }
                }
                
                Item { Layout.fillWidth: true } // 弹簧占位符
                
                // 预估手续费 (右侧对齐)
                ColumnLayout {
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                    spacing: 4
                    
                    Text { 
                        Layout.alignment: Qt.AlignRight
                        text: "预估手续费"
                        color: "#999999" 
                        font.pixelSize: 14 // 12 -> 14
                        font.bold: true
                    }
                    
                    Text { 
                        Layout.alignment: Qt.AlignRight
                        text: "¥ " + (orderController ? orderController.estimatedCommission.toFixed(2) : "0.00")
                        color: "#f0f0f0" // 亮白
                        font.pixelSize: 22 // 18 -> 22
                        font.family: "Consolas"
                        font.bold: true 
                    }
                }
            }
        }
        
    // 交易设置（从外部传入，不在这里定义）
    // property var tradeSettings 已在 main.qml 中传入

    // 监听交易设置变化，立即应用
    Connections {
        target: tradeSettings
        function onDefaultVolumeChanged() {
            if (orderController && tradeSettings) {
                orderController.volume = tradeSettings.defaultVolume
            }
        }
        function onDefaultPriceTypeChanged() {
            if (tradeSettings) {
                priceTypeCombo.currentIndex = tradeSettings.defaultPriceType
            }
        }
    }

    // 监听合约切换，重置面板默认值
    Connections {
        target: orderController
        function onInstrumentIdChanged() {
            if (orderController && tradeSettings) {
                // 重置手数
                orderController.volume = tradeSettings.defaultVolume
                
                // 重置价格类型
                priceTypeCombo.currentIndex = tradeSettings.defaultPriceType
            }
        }
    }
    
    // 初始化时也应用一次
    Component.onCompleted: {
        if (orderController && tradeSettings) {
             orderController.volume = tradeSettings.defaultVolume
             priceTypeCombo.currentIndex = tradeSettings.defaultPriceType
        }
    }
        
    Item { Layout.fillHeight: true }
}
}
