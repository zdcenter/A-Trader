import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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
    
    color: "#252526"
    
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
                    Layout.fillWidth: true
                    spacing: 4
                    
                    Text {
                        text: "价格"
                        color: "#888888"
                        font.pixelSize: 11
                    }
                    
                    TextField {
                        id: priceInput
                        width: parent.width
                        height: 36
                        text: {
                            if (!orderController) return ""
                            return orderController.price > 0 ? orderController.price.toString() : ""
                        }
                        placeholderText: "跟价中..."
                        font.pixelSize: 16
                        font.family: "Consolas"
                        horizontalAlignment: Text.AlignLeft
                        
                        onTextEdited: {
                            if (!orderController) return
                            var val = parseFloat(text)
                            if (!isNaN(val)) {
                                orderController.setPriceOriginal(val)
                                orderController.setManualPrice(true)
                            }
                        }
                        
                        background: Rectangle {
                            color: "#2a2a2a"
                            radius: 4
                            border.color: {
                                if (!orderController) return "#444444"
                                return orderController.isAutoPrice ? "#4caf50" : "#444444"
                            }
                            border.width: 1
                        }
                        
                        color: {
                            if (!orderController) return "white"
                            return orderController.isAutoPrice ? "#4caf50" : "white"
                        }
                    }
                }
                
                // 数量输入
                Column {
                    Layout.preferredWidth: 110
                    spacing: 4
                    
                    Text {
                        text: "数量"
                        color: "#888888"
                        font.pixelSize: 11
                    }
                    
                    RowLayout {
                        width: parent.width
                        height: 36
                        spacing: 0
                        
                        // 减按钮
                        Rectangle {
                            width: 28
                            height: 36
                            color: volumeMinusArea.containsMouse ? "#444444" : "#333333"
                            radius: 4
                            
                            Text {
                                anchors.centerIn: parent
                                text: "−"
                                color: "#cccccc"
                                font.pixelSize: 18
                            }
                            
                            MouseArea {
                                id: volumeMinusArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    if (orderController && orderController.volume > 1) {
                                        orderController.volume--
                                    }
                                }
                            }
                        }
                        
                        // 数量显示/输入
                        TextField {
                            Layout.fillWidth: true
                            height: 36
                            text: orderController ? orderController.volume.toString() : "1"
                            font.pixelSize: 16
                            font.family: "Consolas"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            validator: IntValidator { bottom: 1; top: 9999 }
                            
                            onTextEdited: {
                                if (!orderController) return
                                var val = parseInt(text)
                                if (!isNaN(val) && val > 0) {
                                    orderController.volume = val
                                }
                            }
                            
                            background: Rectangle {
                                color: "#2a2a2a"
                                border.color: "#444444"
                                border.width: 1
                            }
                            color: "white"
                        }
                        
                        // 加按钮
                        Rectangle {
                            width: 28
                            height: 36
                            color: volumePlusArea.containsMouse ? "#444444" : "#333333"
                            radius: 4
                            
                            Text {
                                anchors.centerIn: parent
                                text: "+"
                                color: "#cccccc"
                                font.pixelSize: 18
                            }
                            
                            MouseArea {
                                id: volumePlusArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    if (orderController) {
                                        orderController.volume++
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // 预估信息 (保证金 + 手续费)
        Rectangle {
            Layout.fillWidth: true
            height: 50
            color: "#1e1e1e"
            radius: 4
            border.color: "#333333"
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                
                // 保证金
                Column {
                    spacing: 2
                    Layout.preferredWidth: parent.width / 2 - 10
                    
                    Text {
                        text: "预估保证金"
                        color: "#aaaaaa"
                        font.pixelSize: 11
                        width: parent.width
                        horizontalAlignment: Text.AlignLeft
                    }
                    
                    Text {
                        text: {
                            if (!orderController) return "¥ 0"
                            return "¥ " + orderController.estimatedMargin.toFixed(0)
                        }
                        color: "#ce9178"
                        font.bold: true
                        font.family: "Consolas"
                        font.pixelSize: 15
                        width: parent.width
                        horizontalAlignment: Text.AlignLeft
                    }
                }
                
                // 手续费
                Column {
                    spacing: 2
                    Layout.preferredWidth: parent.width / 2 - 10
                    
                    Text {
                        text: "预估手续费"
                        color: "#aaaaaa"
                        font.pixelSize: 11
                        width: parent.width
                        horizontalAlignment: Text.AlignRight
                    }
                    
                    Text {
                        text: {
                            if (!orderController) return "¥ 0.00"
                            return "¥ " + orderController.estimatedCommission.toFixed(2)
                        }
                        color: "#dcdcaa"
                        font.bold: true
                        font.family: "Consolas"
                        font.pixelSize: 15
                        width: parent.width
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }
        }
        
        // 买开 + 卖开按钮
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            
            Button {
                Layout.fillWidth: true
                height: 45
                text: "买开 (BUY)"
                
                background: Rectangle {
                    color: "#f44336"
                    radius: 4
                }
                
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.bold: true
                }
                
                onClicked: {
                    if (orderController) {
                        orderController.sendOrder("BUY", "OPEN")
                    }
                }
            }
            
            Button {
                Layout.fillWidth: true
                height: 45
                text: "卖开 (SELL)"
                
                background: Rectangle {
                    color: "#4caf50"
                    radius: 4
                }
                
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.bold: true
                }
                
                onClicked: {
                    if (orderController) {
                        orderController.sendOrder("SELL", "OPEN")
                    }
                }
            }
        }
        
        // 平仓按钮
        Button {
            Layout.fillWidth: true
            height: 40
            text: "平仓 (CLOSE)"
            
            onClicked: {
                if (orderController) {
                    orderController.sendOrder("SELL", "CLOSE")
                }
            }
        }
        
        Item { Layout.fillHeight: true }
    }
}
