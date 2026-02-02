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
            Text {
                text: "快速下单"
                color: "white"
                font.pixelSize: 18
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
                font.pixelSize: 18
                font.bold: true
                font.family: "Consolas"
            }
        }
        // 5档行情显示区
        Rectangle {
            Layout.fillWidth: true
            height: 220 
            color: "#1e1e1e"
            radius: 6
            clip: true
            
            Column {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 2
                
                // 卖盘 (Ask 5 -> Ask 1)
                Repeater {
                    model: 5
                    delegate: RowLayout {
                        width: parent.width
                        height: 20
                        property int dataIndex: 4 - index 
                        
                        Text { 
                            text: "卖" + (dataIndex + 1)
                            color: "#888888"
                            font.pixelSize: 11
                            Layout.preferredWidth: 30
                        }
                        
                        Text {
                            text: {
                                if (!orderController || !orderController.askPrices || orderController.askPrices.length <= dataIndex) return "-"
                                var p = orderController.askPrices[dataIndex]
                                return p > 0.0001 && p < 100000000 ? p.toFixed(2) : "-"
                            }
                            color: "#f44336" // 卖出红
                            font.family: "Consolas"
                            font.pixelSize: 13
                            Layout.preferredWidth: 70
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
                        height: 20
                        property int dataIndex: index 
                        
                        Text { 
                            text: "买" + (dataIndex + 1)
                            color: "#888888"
                            font.pixelSize: 11
                            Layout.preferredWidth: 30
                        }
                        
                        Text {
                            text: {
                                if (!orderController || !orderController.bidPrices || orderController.bidPrices.length <= dataIndex) return "-"
                                var p = orderController.bidPrices[dataIndex]
                                return p > 0.0001 && p < 100000000 ? p.toFixed(2) : "-" 
                            }
                            color: "#4caf50" // 买入绿
                            font.family: "Consolas"
                             font.pixelSize: 13
                            Layout.preferredWidth: 70
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
        Rectangle {
            Layout.fillWidth: true
            height: 70
            color: "#1e1e1e"
            radius: 6
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 15
                
                // 价格输入
                Column {
                    Layout.preferredWidth: 140 
                    spacing: 4
                    
                    RowLayout {
                        spacing: 10
                        Text {
                            text: "价格"
                            color: "#888888"
                            font.pixelSize: 11
                        }
                        
                        // 价格类型选择
                        ComboBox {
                            id: priceTypeCombo
                            Layout.preferredWidth: 80
                            Layout.preferredHeight: 20
                            font.pixelSize: 12
                            model: ["限价", "市价", "对手价"]
                            currentIndex: 0
                            
                            delegate: ItemDelegate {
                                width: parent.width
                                height: 25
                                contentItem: Text {
                                    text: modelData
                                    color: "white"
                                    font: priceTypeCombo.font
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: 5
                                }
                                background: Rectangle {
                                    color: parent.highlighted ? "#007acc" : "#252526"
                                }
                            }

                            contentItem: Text {
                                leftPadding: 5
                                rightPadding: priceTypeCombo.indicator.width + 5
                                text: priceTypeCombo.displayText
                                font: priceTypeCombo.font
                                color: "#cccccc"
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignLeft
                                elide: Text.ElideRight
                            }

                            background: Rectangle {
                                implicitWidth: 80
                                implicitHeight: 20
                                color: "#333333"
                                border.color: "#555555"
                                radius: 2
                            }
                        }
                    }
                    
                    RowLayout {
                        width: parent.width
                        height: 36
                        spacing: 2
                        
                        TextField {
                            id: priceInput
                            Layout.fillWidth: true
                            height: 36
                            
                            // 如果是市价单，禁用输入框并显示 "市价"
                            // 如果是市价/对手价，禁用输入框
                            enabled: priceTypeCombo.currentIndex === 0
                            text: {
                                if (priceTypeCombo.currentIndex === 1) return "市价下单"
                                if (priceTypeCombo.currentIndex === 2) return "对手价下单"
                                if (!orderController) return ""
                                return orderController.price > 0 ? orderController.price.toString() : ""
                            }
                            placeholderText: "跟价中..."
                            font.pixelSize: 16
                            font.family: "Consolas"
                            horizontalAlignment: Text.AlignRight
                            leftPadding: 10
                            rightPadding: 10
                            
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
                            
                            // 右侧添加 "自动" 按钮 (当处于手动模式或自动模式时都显示，点击重置)
                            // 仅当非市价模式显示

                            
                            Text {
                                text: "A"
                                visible: priceTypeCombo.currentIndex === 0 && orderController && !orderController.isAutoPrice
                                color: "#4caf50"
                                font.bold: true
                                font.pixelSize: 12
                                anchors.right: parent.right
                                anchors.rightMargin: 8
                                anchors.verticalCenter: parent.verticalCenter
                                
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (orderController) {
                                            orderController.setManualPrice(false)
                                            // 强制触发一次重刷
                                            priceInput.focus = false
                                        }
                                    }
                                }
                            }
                        }
                        
                        // 垂直微调按钮 (上下箭头)
                        Column {
                            visible: priceTypeCombo.currentIndex === 0
                            Layout.preferredWidth: 24
                            Layout.fillHeight: true
                            spacing: 0
                            
                            // 上箭头 (加价)
                            Rectangle {
                                width: 24
                                height: 18
                                color: priceUpArea.containsMouse ? "#444444" : "#333333"
                                radius: 2
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: "▴"
                                    color: "#cccccc"
                                    font.pixelSize: 14
                                }
                                
                                MouseArea {
                                    id: priceUpArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        if (orderController) {
                                            var tick = orderController.priceTick
                                            var val = parseFloat(priceInput.text)
                                            if (isNaN(val)) val = orderController.price
                                            
                                            var p = val + tick
                                            p = Math.round(p / tick) * tick
                                            orderController.setPriceOriginal(Number(p.toFixed(4)))
                                            orderController.setManualPrice(true)
                                        }
                                    }
                                }
                            }
                            
                            // 下箭头 (降价)
                            Rectangle {
                                width: 24
                                height: 18
                                color: priceDownArea.containsMouse ? "#444444" : "#333333"
                                radius: 2
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: "▾"
                                    color: "#cccccc"
                                    font.pixelSize: 14
                                }
                                
                                MouseArea {
                                    id: priceDownArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        if (orderController) {
                                            var tick = orderController.priceTick
                                            var val = parseFloat(priceInput.text)
                                            if (isNaN(val)) val = orderController.price
                                            
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
                
                // 数量输入
                Column {
                    Layout.preferredWidth: 100 // 保持合适宽度
                    spacing: 4
                    
                    Text {
                        text: "数量"
                        color: "#888888"
                        font.pixelSize: 11
                    }
                    
                    RowLayout {
                        width: parent.width
                        height: 36
                        spacing: 2
                        
                        // 数量显示/输入
                        TextField {
                            Layout.fillWidth: true
                            height: 36
                            text: orderController ? orderController.volume.toString() : "1"
                            font.pixelSize: 16
                            font.family: "Consolas"
                            font.bold: true
                            horizontalAlignment: Text.AlignRight
                            leftPadding: 10
                            rightPadding: 10
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

                        // 垂直微调按钮 (上下箭头)
                        Column {
                            Layout.preferredWidth: 24
                            Layout.fillHeight: true
                            spacing: 0
                            
                            // 上箭头 (加量)
                            Rectangle {
                                width: 24
                                height: 18
                                color: volUpArea.containsMouse ? "#444444" : "#333333"
                                radius: 2
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: "▴"
                                    color: "#cccccc"
                                    font.pixelSize: 14
                                }
                                
                                MouseArea {
                                    id: volUpArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        if (orderController) orderController.volume++
                                    }
                                }
                            }
                            
                            // 下箭头 (减量)
                            Rectangle {
                                width: 24
                                height: 18
                                color: volDownArea.containsMouse ? "#444444" : "#333333"
                                radius: 2
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: "▾"
                                    color: "#cccccc"
                                    font.pixelSize: 14
                                }
                                
                                MouseArea {
                                    id: volDownArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        if (orderController && orderController.volume > 1) {
                                            orderController.volume--
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // 下单按钮区域 (仿快期布局)
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            rowSpacing: 10
            columnSpacing: 10
            
            // --- 第一行：开仓 ---
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 45
                text: "买开仓 (多)"
                
                scale: down ? 0.96 : 1.0
                
                background: Rectangle {
                    color: {
                        if (!parent.enabled) return "#555555"
                        return parent.down ? Qt.darker("#f44336", 1.2) : (parent.hovered ? Qt.lighter("#f44336", 1.1) : "#f44336")
                    }
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text; color: parent.enabled ? "white" : "#aaaaaa"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: if(orderController) orderController.sendOrder("BUY", "OPEN", getPriceType())
            }
            
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 45
                text: "卖开仓 (空)"
                
                scale: down ? 0.96 : 1.0
                
                background: Rectangle {
                    color: {
                        if (!parent.enabled) return "#555555"
                        return parent.down ? Qt.darker("#4caf50", 1.2) : (parent.hovered ? Qt.lighter("#4caf50", 1.1) : "#4caf50")
                    }
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text; color: parent.enabled ? "white" : "#aaaaaa"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: if(orderController) orderController.sendOrder("SELL", "OPEN", getPriceType())
            }
            
            // --- 第二行：平仓 (平昨/智能平仓) ---
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 45
                text: "买平仓 (空平)"
                enabled: orderController ? orderController.shortPosition > 0 : false
                
                scale: down ? 0.96 : 1.0
                
                background: Rectangle {
                    color: {
                        if (!parent.enabled) return "#555555"
                        return parent.down ? Qt.darker("#d32f2f", 1.2) : (parent.hovered ? Qt.lighter("#d32f2f", 1.1) : "#d32f2f")
                    }
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text; color: parent.enabled ? "white" : "#aaaaaa"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: if(orderController) orderController.sendOrder("BUY", "CLOSE", getPriceType())
            }
            
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 45
                text: "卖平仓 (多平)"
                enabled: orderController ? orderController.longPosition > 0 : false
                
                scale: down ? 0.96 : 1.0
                
                background: Rectangle {
                    color: {
                        if (!parent.enabled) return "#555555"
                        return parent.down ? Qt.darker("#388e3c", 1.2) : (parent.hovered ? Qt.lighter("#388e3c", 1.1) : "#388e3c")
                    }
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text; color: parent.enabled ? "white" : "#aaaaaa"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: if(orderController) orderController.sendOrder("SELL", "CLOSE", getPriceType())
            }
            
            // --- 第三行：平今仓 (上期所专用) ---
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                text: "买平今"
                enabled: orderController ? orderController.shortTd > 0 : false
                
                scale: down ? 0.96 : 1.0
                
                background: Rectangle {
                    color: {
                        if (!parent.enabled) return "#555555"
                        return parent.down ? Qt.darker("#b71c1c", 1.2) : (parent.hovered ? Qt.lighter("#b71c1c", 1.1) : "#b71c1c")
                    }
                    radius: 4
                    border.color: parent.enabled ? "#ff8a80" : "transparent"; border.width: 1
                }
                contentItem: Text {
                    text: parent.text; color: parent.enabled ? "#ffcdd2" : "#aaaaaa"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: if(orderController) orderController.sendOrder("BUY", "CLOSETODAY", getPriceType())
            }
            
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                text: "卖平今"
                enabled: orderController ? orderController.longTd > 0 : false
                
                scale: down ? 0.96 : 1.0
                
                background: Rectangle {
                    color: {
                        if (!parent.enabled) return "#555555"
                        return parent.down ? Qt.darker("#1b5e20", 1.2) : (parent.hovered ? Qt.lighter("#1b5e20", 1.1) : "#1b5e20")
                    }
                    radius: 4
                    border.color: parent.enabled ? "#a5d6a7" : "transparent"; border.width: 1
                }
                contentItem: Text {
                    text: parent.text; color: parent.enabled ? "#c8e6c9" : "#aaaaaa"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                onClicked: if(orderController) orderController.sendOrder("SELL", "CLOSETODAY", getPriceType())
            }
        }
        
        // 资金信息显示区域 (预估保证金/手续费)
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#1e1e1e"
            radius: 4
            border.color: "#333333"
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                
                // 预估保证金 (左侧对齐)
                ColumnLayout {
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 0
                    
                    Text { 
                        text: "预估保证金"
                        color: "#888888"
                        font.pixelSize: 12
                    }
                    
                    Text { 
                        text: "¥ " + (orderController ? orderController.estimatedMargin.toFixed(0) : "0")
                        color: "#eaa46e" // 金色
                        font.pixelSize: 18
                        font.family: "Consolas"
                        font.bold: true 
                    }
                }
                
                Item { Layout.fillWidth: true } // 弹簧占位符
                
                // 预估手续费 (右侧对齐)
                ColumnLayout {
                    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                    spacing: 0
                    
                    Text { 
                        Layout.alignment: Qt.AlignRight
                        text: "预估手续费"
                        color: "#888888" 
                        font.pixelSize: 12 
                    }
                    
                    Text { 
                        Layout.alignment: Qt.AlignRight
                        text: "¥ " + (orderController ? orderController.estimatedCommission.toFixed(2) : "0.00")
                        color: "#f0f0f0" // 亮白
                        font.pixelSize: 18
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
