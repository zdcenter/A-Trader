import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// 条件单管理面板 - 全新设计
Rectangle {
    id: root
    color: "#1e1e1e"
    
    property var marketModel
    property var orderController
    
    // 选中的合约信息
    property string selectedInstrument: ""
    property real selectedLastPrice: 0.0
    
    // 监听选中合约变化
    onSelectedInstrumentChanged: {
        if (selectedInstrument) {
            // 自动设置合约选择框
            var index = -1
            if (marketModel && marketModel.subscribedInstruments) {
                for (var i = 0; i < marketModel.subscribedInstruments.length; i++) {
                    if (marketModel.subscribedInstruments[i] === selectedInstrument) {
                        index = i
                        break
                    }
                }
            }
            if (index >= 0) {
                instrumentCombo.currentIndex = index
            }
            
            // 自动填充当前价格到触发价格
            if (selectedLastPrice > 0) {
                triggerPriceInput.text = selectedLastPrice.toFixed(2)
            }
        }
    }
    
    // 监听价格变化，实时更新触发价格输入框
    onSelectedLastPriceChanged: {
        if (selectedInstrument && selectedInstrument === instrumentCombo.displayText) {
            // 只有当前选中的合约才更新价格
            if (selectedLastPrice > 0 && triggerPriceInput.text === "") {
                triggerPriceInput.text = selectedLastPrice.toFixed(2)
            }
        }
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20
        
        // 顶部标题栏
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 35
            color: "transparent"
            
            RowLayout {
                anchors.fill: parent
                spacing: 20
                
                Text {
                    text: "条件单管理"
                    font.pixelSize: 16
                    font.bold: true
                    color: "#ffffff"
                }
                
                Item { Layout.fillWidth: true }
                
                // 统计信息
                Row {
                    spacing: 30
                    
                    Column {
                        spacing: 4
                        Text {
                            text: "待触发"
                            font.pixelSize: 11
                            color: "#888888"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: orderController ? orderController.conditionOrderList.filter(function(o) { 
                                return o.status === 0 
                            }).length : "0"
                            font.pixelSize: 16
                            font.bold: true
                            color: "#2196f3"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                    
                    Rectangle {
                        width: 1
                        height: 30
                        color: "#333333"
                    }
                    
                    Column {
                        spacing: 4
                        Text {
                            text: "已触发"
                            font.pixelSize: 11
                            color: "#888888"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Text {
                            text: orderController ? orderController.conditionOrderList.filter(function(o) { 
                                return o.status === 1 
                            }).length : "0"
                            font.pixelSize: 16
                            font.bold: true
                            color: "#4caf50"
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }
        }
        
        // 主体内容区域
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20
            
            // 左侧：新建条件单表单
            Rectangle {
                Layout.preferredWidth: 380
                Layout.fillHeight: true
                color: "#2a2a2a"
                radius: 8
                border.color: "#444444"
                border.width: 1
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 12
                    
                    Text {
                        text: "新建条件单"
                        font.pixelSize: 16
                        font.bold: true
                        color: "#ffffff"
                    }
                    
                    // 合约选择
                    Column {
                        Layout.fillWidth: true
                        spacing: 6
                        
                        Text {
                            text: "合约"
                            font.pixelSize: 12
                            font.bold: true
                            color: "#ffffff"
                        }
                        
                        ComboBox {
                            id: instrumentCombo
                            width: parent.width
                            height: 32
                            model: marketModel ? marketModel.subscribedInstruments : []
                            
                            contentItem: Text {
                                leftPadding: 15
                                text: instrumentCombo.displayText
                                font.pixelSize: 15
                                color: "#ffffff"
                                verticalAlignment: Text.AlignVCenter
                            }
                            
                            background: Rectangle {
                                color: "#1e1e1e"
                                border.color: instrumentCombo.activeFocus ? "#2196f3" : "#555555"
                                border.width: 2
                                radius: 6
                            }
                        }
                    }
                    
                    // 触发条件
                    Column {
                        Layout.fillWidth: true
                        spacing: 6
                        
                        Text {
                            text: "触发条件"
                            font.pixelSize: 12
                            font.bold: true
                            color: "#ffffff"
                        }
                        
                        Row {
                            width: parent.width
                            spacing: 12
                            
                            ComboBox {
                                id: conditionTypeCombo
                                width: 70
                                height: 32
                                model: [">", "≥", "<", "≤"]
                                currentIndex: 0
                                
                                contentItem: Text {
                                    text: conditionTypeCombo.displayText
                                    font.pixelSize: 18
                                    font.bold: true
                                    color: "#ffffff"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                
                                background: Rectangle {
                                    color: "#1e1e1e"
                                    border.color: "#555555"
                                    border.width: 2
                                    radius: 6
                                }
                            }
                            
                            TextField {
                                id: triggerPriceInput
                                width: parent.width - 82
                                height: 32
                                placeholderText: "触发价格"
                                placeholderTextColor: "#666666"
                                font.pixelSize: 15
                                color: "#ffffff"
                                leftPadding: 15
                                
                                background: Rectangle {
                                    color: "#1e1e1e"
                                    border.color: triggerPriceInput.activeFocus ? "#2196f3" : "#555555"
                                    border.width: 2
                                    radius: 6
                                }
                            }
                        }
                    }
                    
                    // 买卖方向
                    Row {
                        Layout.fillWidth: true
                        spacing: 15
                        
                        Text {
                            text: "买卖："
                            font.pixelSize: 12
                            font.bold: true
                            color: "#ffffff"
                            height: 30
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        RadioButton {
                            id: buyRadio
                            checked: true
                            height: 30
                            
                            indicator: Rectangle {
                                width: 14
                                height: 14
                                radius: 7
                                border.color: "#4caf50"
                                border.width: 2
                                color: "transparent"
                                anchors.verticalCenter: parent.verticalCenter
                                
                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    anchors.centerIn: parent
                                    color: "#4caf50"
                                    visible: buyRadio.checked
                                }
                            }
                            
                            contentItem: Text {
                                text: "买"
                                font.pixelSize: 13
                                color: "#ffffff"
                                leftPadding: 20
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        
                        RadioButton {
                            id: sellRadio
                            height: 30
                            
                            indicator: Rectangle {
                                width: 14
                                height: 14
                                radius: 7
                                border.color: "#f44336"
                                border.width: 2
                                color: "transparent"
                                anchors.verticalCenter: parent.verticalCenter
                                
                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    anchors.centerIn: parent
                                    color: "#f44336"
                                    visible: sellRadio.checked
                                }
                            }
                            
                            contentItem: Text {
                                text: "卖"
                                font.pixelSize: 13
                                color: "#ffffff"
                                leftPadding: 20
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                    
                    // 开平仓
                    Row {
                        Layout.fillWidth: true
                        spacing: 8
                        
                        Text {
                            text: "开平："
                            font.pixelSize: 12
                            font.bold: true
                            color: "#ffffff"
                            height: 30
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        RadioButton {
                            id: openRadio
                            checked: true
                            height: 30
                            
                            indicator: Rectangle {
                                width: 14
                                height: 14
                                radius: 7
                                border.color: "#ffa726"
                                border.width: 2
                                color: "transparent"
                                anchors.verticalCenter: parent.verticalCenter
                                
                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    anchors.centerIn: parent
                                    color: "#ffa726"
                                    visible: openRadio.checked
                                }
                            }
                            
                            contentItem: Text {
                                text: "开仓"
                                font.pixelSize: 13
                                color: "#ffffff"
                                leftPadding: 20
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        
                        RadioButton {
                            id: closeRadio
                            height: 30
                            
                            indicator: Rectangle {
                                width: 14
                                height: 14
                                radius: 7
                                border.color: "#ffa726"
                                border.width: 2
                                color: "transparent"
                                anchors.verticalCenter: parent.verticalCenter
                                
                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    anchors.centerIn: parent
                                    color: "#ffa726"
                                    visible: closeRadio.checked
                                }
                            }
                            
                            contentItem: Text {
                                text: "平仓"
                                font.pixelSize: 13
                                color: "#ffffff"
                                leftPadding: 20
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        
                        RadioButton {
                            id: closeTodayRadio
                            height: 30
                            
                            indicator: Rectangle {
                                width: 14
                                height: 14
                                radius: 7
                                border.color: "#ffa726"
                                border.width: 2
                                color: "transparent"
                                anchors.verticalCenter: parent.verticalCenter
                                
                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    anchors.centerIn: parent
                                    color: "#ffa726"
                                    visible: closeTodayRadio.checked
                                }
                            }
                            
                            contentItem: Text {
                                text: "平今"
                                font.pixelSize: 13
                                color: "#ffffff"
                                leftPadding: 20
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }


                    
                    // 手数
                    Column {
                        Layout.fillWidth: true
                        spacing: 6
                        
                        Text {
                            text: "手数"
                            font.pixelSize: 12
                            font.bold: true
                            color: "#ffffff"
                        }
                        
                        TextField {
                            id: volumeInput
                            width: parent.width
                            height: 32
                            placeholderText: "请输入手数"
                            placeholderTextColor: "#666666"
                            font.pixelSize: 15
                            color: "#ffffff"
                            leftPadding: 15
                            
                            background: Rectangle {
                                color: "#1e1e1e"
                                border.color: volumeInput.activeFocus ? "#2196f3" : "#555555"
                                border.width: 2
                                radius: 6
                            }
                        }
                    }
                    
                    // 成交价
                    Column {
                        Layout.fillWidth: true
                        spacing: 6
                        
                        Text {
                            text: "成交价 (0=市价)"
                            font.pixelSize: 12
                            font.bold: true
                            color: "#ffffff"
                        }
                        
                        TextField {
                            id: limitPriceInput
                            width: parent.width
                            height: 32
                            placeholderText: "限价 (0表示市价)"
                            placeholderTextColor: "#666666"
                            text: "0"
                            font.pixelSize: 15
                            color: "#ffffff"
                            leftPadding: 15
                            
                            background: Rectangle {
                                color: "#1e1e1e"
                                border.color: limitPriceInput.activeFocus ? "#2196f3" : "#555555"
                                border.width: 2
                                radius: 6
                            }
                        }
                    }
                    
                    Item { Layout.fillHeight: true }
                    
                    // 提交按钮
                    Button {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        text: "提交条件单"
                        
                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: 14
                            font.bold: true
                            color: "#ffffff"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        background: Rectangle {
                            color: parent.down ? "#1976d2" : (parent.hovered ? "#2196f3" : "#1e88e5")
                            radius: 6
                        }
                        
                        onClicked: submitConditionOrder()
                    }
                }
            }
            
            // 右侧：条件单列表
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#252526"
                radius: 8
                border.color: "#3e3e42"
                border.width: 1
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 1
                    spacing: 0
                    
                    // 列表标题
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 45
                        color: "#2d2d30"
                        radius: 8
                        
                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 20
                            anchors.rightMargin: 20
                            spacing: 15
                            
                            Text { text: "合约"; color: "#aaaaaa"; width: 100; font.pixelSize: 13; font.bold: true; verticalAlignment: Text.AlignVCenter; height: parent.height }
                            Text { text: "触发条件"; color: "#aaaaaa"; width: 150; font.pixelSize: 13; font.bold: true; verticalAlignment: Text.AlignVCenter; height: parent.height }
                            Text { text: "执行动作"; color: "#aaaaaa"; width: 200; font.pixelSize: 13; font.bold: true; verticalAlignment: Text.AlignVCenter; height: parent.height }
                            Text { text: "状态"; color: "#aaaaaa"; width: 80; font.pixelSize: 13; font.bold: true; verticalAlignment: Text.AlignVCenter; height: parent.height }
                            Item { width: 1; Layout.fillWidth: true }
                            Text { text: "操作"; color: "#aaaaaa"; width: 60; font.pixelSize: 13; font.bold: true; verticalAlignment: Text.AlignVCenter; height: parent.height }
                        }
                    }
                    
                    // 条件单列表
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: orderController ? orderController.conditionOrderList : null
                        spacing: 2
                        
                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 60
                            color: index % 2 === 0 ? "#1f1f1f" : "#252526"
                            
                            property var m: modelData
                            
                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: 20
                                anchors.rightMargin: 20
                                spacing: 15
                                
                                // 合约
                                Text {
                                    text: m.instrument_id
                                    color: "#ffffff"
                                    font.pixelSize: 14
                                    width: 100
                                    verticalAlignment: Text.AlignVCenter
                                    height: parent.height
                                }
                                
                                // 触发条件
                                Text {
                                    text: {
                                        var symbols = [">", "≥", "<", "≤"]
                                        return symbols[m.compare_type] + " " + m.trigger_price.toFixed(2)
                                    }
                                    color: "#ffa726"
                                    font.pixelSize: 15
                                    font.bold: true
                                    width: 150
                                    verticalAlignment: Text.AlignVCenter
                                    height: parent.height
                                }
                                
                                // 执行动作
                                Row {
                                    spacing: 8
                                    width: 200
                                    height: parent.height
                                    
                                    Rectangle {
                                        width: 40
                                        height: 24
                                        color: m.direction === "buy" ? "#4caf50" : "#f44336"
                                        radius: 3
                                        anchors.verticalCenter: parent.verticalCenter
                                        
                                        Text {
                                            text: m.direction === "buy" ? "买" : "卖"
                                            color: "#ffffff"
                                            font.pixelSize: 12
                                            font.bold: true
                                            anchors.centerIn: parent
                                        }
                                    }
                                    
                                    Text {
                                        text: m.offset_flag
                                        color: "#cccccc"
                                        font.pixelSize: 13
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    
                                    Text {
                                        text: m.volume + "手"
                                        color: "#ffffff"
                                        font.pixelSize: 14
                                        font.bold: true
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    
                                    Text {
                                        text: m.limit_price > 0 ? "@" + m.limit_price.toFixed(2) : "@市价"
                                        color: "#888888"
                                        font.pixelSize: 12
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                                
                                // 状态
                                Rectangle {
                                    width: 80
                                    height: 28
                                    color: {
                                        if (m.status === 0) return "#1976d2"
                                        if (m.status === 1) return "#388e3c"
                                        return "#616161"
                                    }
                                    radius: 4
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    Text {
                                        text: {
                                            if (m.status === 0) return "待触发"
                                            if (m.status === 1) return "已触发"
                                            return "已取消"
                                        }
                                        color: "#ffffff"
                                        font.pixelSize: 12
                                        font.bold: true
                                        anchors.centerIn: parent
                                    }
                                }
                                
                                Item { Layout.fillWidth: true }
                                
                                // 操作按钮
                                Button {
                                    width: 60
                                    height: 32
                                    text: "撤销"
                                    visible: m.status === 0
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    contentItem: Text {
                                        text: parent.text
                                        font.pixelSize: 12
                                        color: "#ffffff"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    
                                    background: Rectangle {
                                        color: parent.down ? "#c62828" : (parent.hovered ? "#d32f2f" : "#e53935")
                                        radius: 4
                                    }
                                    
                                    onClicked: {
                                        if (orderController) {
                                            orderController.cancelConditionOrder(m.request_id)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 提交条件单函数
    function submitConditionOrder() {
        if (!orderController) {
            console.error("OrderController 未就绪")
            return
        }
        
        var instrument = instrumentCombo.currentText
        var triggerPrice = parseFloat(triggerPriceInput.text)
        var limitPrice = parseFloat(limitPriceInput.text)
        var volume = parseInt(volumeInput.text)
        
        if (!instrument || isNaN(triggerPrice) || isNaN(volume) || volume <= 0) {
            console.error("请填写完整的条件单信息")
            return
        }
        
        // 从单选按钮获取方向
        var direction = buyRadio.checked ? "buy" : "sell"
        
        // 从单选按钮获取开平仓类型
        var offsetFlag = "open"
        if (closeRadio.checked) offsetFlag = "close"
        else if (closeTodayRadio.checked) offsetFlag = "close_today"
        
        var compareType = conditionTypeCombo.currentIndex
        
        orderController.submitConditionOrder(
            instrument,
            triggerPrice,
            compareType,
            direction,
            offsetFlag,
            volume,
            limitPrice
        )
        
        // 清空输入
        triggerPriceInput.text = ""
        limitPriceInput.text = "0"
        volumeInput.text = ""
        // 重置单选按钮
        buyRadio.checked = true
        openRadio.checked = true
    }
}
