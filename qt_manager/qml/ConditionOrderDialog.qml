import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12


Popup {
    id: root
    
    // 外部传入控制器
    property var orderController
    
    // 弹窗属性
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: Overlay.overlay // 居中显示
    
    // 内部状态
    property string direction: "BUY"  // BUY, SELL
    property string offset: "OPEN"    // OPEN, CLOSE, CLOSETODAY
    property string conditionType: ">= (" // 自动判断
    property bool isSmartCondition: true
    property bool isTriggerManual: false // 是否手动修改过触发价

    // 自动跟随最新价逻辑
    Connections {
        target: orderController
        function onLastPriceChanged() {
            if (!isTriggerManual && root.visible && orderController && orderController.lastPrice > 0) {
                // 如果用户没手动修改过，输入框跟随最新价跳动
                triggerPriceInput.text = orderController.lastPrice.toFixed(orderController.priceTick < 1 ? 2 : 0)
            }
        }
    }
    
    // 智能推断条件方向
    function updateConditionType() {
        if (!isSmartCondition || !orderController) return
        
        var triggerP = parseFloat(triggerPriceInput.text)
        var currentP = orderController.lastPrice
        
        if (isNaN(triggerP) || currentP === 0) return
        
        if (triggerP > currentP) {
            conditionType = "≥" // 大于等于 (突破)
        } else {
            conditionType = "≤" // 小于等于 (跌破)
        }
    }
    
    // 重置状态
    function reset() {
        if (orderController) {
            isTriggerManual = false
            triggerPriceInput.text = orderController.lastPrice.toFixed(orderController.priceTick < 1 ? 2 : 0)
            
            // Reset Order Price Inputs
            priceTypeCombo.currentIndex = 0
            priceInput.text = orderController.price.toFixed(orderController.priceTick < 1 ? 2 : 0)
            priceInput.enabled = true
            
            volInput.text = orderController.volume.toString()
            direction = "BUY"
            offset = "OPEN"
            updateConditionType()
        }
        triggerPriceInput.forceActiveFocus()
        triggerPriceInput.selectAll()
    }
    
    onOpened: reset()
    
    // 背景样式
    background: Rectangle {
        color: "#333333"
        border.color: "#007acc"
        border.width: 2
        radius: 8
        // 添加阴影效果 (简单模拟)
        Rectangle {
            z: -1
            anchors.fill: parent
            anchors.margins: -4
            color: "#000000"
            opacity: 0.3
            radius: 12
        }
    }
    
    contentItem: ColumnLayout {
        spacing: 20
        width: 720 // Increased width to fit the new controls
        
        // 标题栏
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "条件单设置"
                color: "#cccccc"
                font.bold: true
                font.pixelSize: 14
            }
            Item { Layout.fillWidth: true }
            Text {
                text: "当前最新价: " + (orderController ? orderController.lastPrice.toFixed(2) : "--")
                color: "#ffca28"
                font.family: "Consolas"
                font.pixelSize: 14
            }
        }
        
        Rectangle { Layout.fillWidth: true; height: 1; color: "#444444" }
        
        // --- 第一行：触发条件 (句子式) ---
        // "当 [合约] [最新价] [>=] [触发价] 时"
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            
            Text { text: "当"; color: "white"; font.pixelSize: 16 }
            
            // 合约代码
            Rectangle {
                width: 80; height: 32; color: "#333333"; radius: 4
                Text { 
                    anchors.centerIn: parent
                    text: orderController ? orderController.instrumentId : "--"
                    color: "white"; font.bold: true; font.family: "Consolas"; font.pixelSize: 15
                }
            }
            
            // 价格类型 (默认最新价)
            ComboBox {
                model: ["最新价", "买一价", "卖一价"]
                currentIndex: 0
                Layout.preferredWidth: 90
                Layout.preferredHeight: 32
                font.pixelSize: 14
                background: Rectangle { color: "#333333"; radius: 4; border.width: 1; border.color: "#555" }
                contentItem: Text { text: parent.displayText; color: "white"; verticalAlignment: Text.AlignVCenter; leftPadding: 10 }
            }
            
            // 比较符
            ComboBox {
                id: operatorCombo
                model: ["≥", "≤"]
                currentIndex: root.conditionType === "≥" ? 0 : 1
                Layout.preferredWidth: 60
                Layout.preferredHeight: 32
                font.pixelSize: 18
                font.bold: true
                background: Rectangle { color: "#333333"; radius: 4; border.width: 1; border.color: "#555" }
                contentItem: Text { text: parent.displayText; color: "#007acc"; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                onActivated: root.conditionType = currentText
            }
            
            // 触发价格输入 (带微调按钮)
            RowLayout {
                spacing: 0
                Layout.preferredHeight: 32
                
                Button {
                    text: "－"
                    Layout.fillHeight: true
                    Layout.preferredWidth: 32
                    background: Rectangle { color: "#444444"; radius: 2 }
                    contentItem: Text { text: parent.text; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: {
                        root.isTriggerManual = true
                        triggerPriceInput.modPrice(-1)
                    }
                }
                
                TextField {
                    id: triggerPriceInput
                    Layout.preferredWidth: 100
                    Layout.fillHeight: true
                    font.pixelSize: 18
                    font.bold: true
                    font.family: "Consolas"
                    color: "#ffca28" // 醒目黄
                    horizontalAlignment: Text.AlignRight
                    selectByMouse: true
                    background: Rectangle {
                        color: "#1e1e1e"
                        border.color: triggerPriceInput.activeFocus ? "#007acc" : "#555"
                        border.width: 1
                    }
                    validator: DoubleValidator { bottom: 0; top: 1000000; decimals: 2 }
                    
                    onTextEdited: root.isTriggerManual = true
                    onTextChanged: root.updateConditionType()
                    
                    // 快捷键微调
                    Keys.onUpPressed: { root.isTriggerManual = true; modPrice(1) }
                    Keys.onDownPressed: { root.isTriggerManual = true; modPrice(-1) }
                    
                    function modPrice(dir) {
                         var val = parseFloat(text)
                         if (!isNaN(val) && orderController) text = (val + orderController.priceTick * dir).toFixed(orderController.priceTick < 1 ? 2 : 0)
                    }
                }
                
                Button {
                    text: "＋"
                    Layout.fillHeight: true
                    Layout.preferredWidth: 32
                    background: Rectangle { color: "#444444"; radius: 2 }
                    contentItem: Text { text: parent.text; color: "white"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: {
                         root.isTriggerManual = true
                         triggerPriceInput.modPrice(1)
                    }
                }
            }
            
            Text { text: "时"; color: "white"; font.pixelSize: 16 }
        }
        
        // --- 第二行：执行动作 ---
        // "执行 [买/卖] [开/平] [数量] 手，报单 [价格类型] [价格]"
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            
            Text { text: "执行"; color: "white"; font.pixelSize: 16 }
            
            // 买卖方向
            RowLayout {
                spacing: 0
                Rectangle {
                    width: 50; height: 32
                    color: root.direction === "BUY" ? "#d32f2f" : "#333333"
                    radius: 4
                    Text { text: "买入"; anchors.centerIn: parent; color: "white"; font.bold: true }
                    MouseArea { anchors.fill: parent; onClicked: root.direction = "BUY" }
                }
                Rectangle {
                    width: 50; height: 32
                    color: root.direction === "SELL" ? "#1976d2" : "#333333"
                    radius: 4
                    Text { text: "卖出"; anchors.centerIn: parent; color: "white"; font.bold: true }
                    MouseArea { anchors.fill: parent; onClicked: root.direction = "SELL" }
                }
            }
            
            // 开平
            RowLayout {
                spacing: 1
                Repeater {
                    model: [ {l:"开", v:"OPEN"}, {l:"平", v:"CLOSE"}, {l:"今", v:"CLOSETODAY"} ]
                    delegate: Rectangle {
                        width: 40; height: 32
                        color: root.offset === modelData.v ? "#fbc02d" : "#333333"
                        Text { 
                            text: modelData.l
                            anchors.centerIn: parent
                            color: root.offset === modelData.v ? "black" : "#aaaaaa"
                        }
                        MouseArea { anchors.fill: parent; onClicked: root.offset = modelData.v }
                    }
                }
            }
            
            // 数量
            TextField {
                id: volInput
                Layout.preferredWidth: 50
                Layout.preferredHeight: 32
                font.pixelSize: 15
                font.family: "Consolas"
                horizontalAlignment: Text.AlignRight
                text: "1"
                selectByMouse: true
                color: "white"
                background: Rectangle { color: "#333333"; radius: 4; border.width: 1; border.color: "#555" }
            }
            
            Text { text: "手 , 报单"; color: "white"; font.pixelSize: 16 }
            
            // 报单价格类型
            ComboBox {
                id: priceTypeCombo
                model: ["限价", "对手价", "最新价", "市价"]
                currentIndex: 0 // 默认限价
                Layout.preferredWidth: 90
                Layout.preferredHeight: 32
                font.pixelSize: 14
                background: Rectangle { color: "#333333"; radius: 4; border.width: 1; border.color: "#555" }
                contentItem: Text { text: parent.displayText; color: "#cccccc"; verticalAlignment: Text.AlignVCenter; leftPadding: 5 }
                
                onCurrentIndexChanged: {
                    if (currentIndex === 0) {
                        // 切换回限价时，恢复当前价格
                        if (orderController) {
                             priceInput.text = orderController.price.toFixed(orderController.priceTick < 1 ? 2 : 0)
                        }
                    } else {
                        // 其他类型显示对应文本
                        var texts = ["", "对手价", "最新价", "市价"]
                        priceInput.text = texts[currentIndex]
                    }
                }
            }

            // 报单价格 (带微调按钮)
            RowLayout {
                spacing: 0
                Layout.preferredHeight: 32
                // 始终显示，保持布局稳定
                
                Button {
                    text: "－"
                    Layout.fillHeight: true
                    Layout.preferredWidth: 28
                    enabled: priceTypeCombo.currentIndex === 0
                    background: Rectangle { color: enabled ? "#444444" : "#2a2a2a"; radius: 2 }
                    contentItem: Text { text: parent.text; color: parent.enabled ? "white" : "#555"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: priceInput.modPrice(-1)
                }
                
                TextField {
                    id: priceInput
                    Layout.preferredWidth: 90
                    Layout.fillHeight: true
                    font.pixelSize: 16
                    font.bold: true
                    font.family: "Consolas"
                    horizontalAlignment: Text.AlignRight
                    selectByMouse: true
                    enabled: priceTypeCombo.currentIndex === 0
                    color: enabled ? "white" : "#aaaaaa"
                    background: Rectangle { color: parent.enabled ? "#333333" : "#222222"; radius: 4; border.width: 1; border.color: "#555" }
                    
                    function modPrice(dir) {
                         if (!enabled) return
                         var val = parseFloat(text)
                         if (!isNaN(val) && orderController) text = (val + orderController.priceTick * dir).toFixed(orderController.priceTick < 1 ? 2 : 0)
                    }
                    Keys.onUpPressed: modPrice(1)
                    Keys.onDownPressed: modPrice(-1)
                }
                
                Button {
                    text: "＋"
                    Layout.fillHeight: true
                    Layout.preferredWidth: 28
                    enabled: priceTypeCombo.currentIndex === 0
                    background: Rectangle { color: enabled ? "#444444" : "#2a2a2a"; radius: 2 }
                    contentItem: Text { text: parent.text; color: parent.enabled ? "white" : "#555"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    onClicked: priceInput.modPrice(1)
                }
            }
        }
        
        Rectangle { Layout.fillWidth: true; height: 1; color: "#444444" }
        
        // --- 第三行：底部按钮 ---
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight
            spacing: 20
            
            Button {
                text: "取消 (Esc)"
                Layout.preferredWidth: 100
                Layout.preferredHeight: 36
                background: Rectangle { color: "#444444"; radius: 4 }
                contentItem: Text { text: parent.text; color: "#cccccc"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: root.close()
            }
            
            Button {
                text: "确定 (Enter)"
                Layout.preferredWidth: 120
                Layout.preferredHeight: 36
                background: Rectangle { color: "#007acc"; radius: 4 }
                contentItem: Text { text: parent.text; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                
                onClicked: confirmOrder()
                Keys.onEnterPressed: confirmOrder()
                Keys.onReturnPressed: confirmOrder()
                
                function confirmOrder() {
                    // 这里填写确认逻辑
                    console.log("条件单确认")
                    root.close()
                }
            }
        }
    }
}
