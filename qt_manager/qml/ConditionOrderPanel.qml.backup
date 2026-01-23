import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtMultimedia

Rectangle {
    id: root
    color: "#1e1e1e"
    focus: true  // 允许根元素获取焦点
    
    // 外部传入的数据模型
    property var marketModel
    property var orderController
    
    // 声音效果
    SoundEffect {
        id: triggeredSound
        source: "qrc:/sounds/condition_triggered.wav"
        volume: 0.8
    }
    
    SoundEffect {
        id: cancelledSound
        source: "qrc:/sounds/condition_cancelled.wav"
        volume: 0.8
    }
    
    // 视觉提示动画
    Rectangle {
        id: flashOverlay
        anchors.fill: parent
        color: "transparent"
        opacity: 0
        z: 1000
        
        SequentialAnimation {
            id: flashAnimation
            PropertyAnimation {
                target: flashOverlay
                property: "opacity"
                to: 0.3
                duration: 100
            }
            PropertyAnimation {
                target: flashOverlay
                property: "opacity"
                to: 0
                duration: 200
            }
        }
    }
    
    // 监听声音信号 - 同时播放音频和视觉提示
    Connections {
        target: orderController
        function onConditionOrderSound(soundType) {
            if (soundType === "triggered") {
                // 播放音频
                triggeredSound.play()
                // 视觉提示
                flashOverlay.color = "#00ff00"  // 绿色闪烁
                flashAnimation.restart()
                console.log("🔔 条件单已触发！")
            } else if (soundType === "cancelled") {
                // 播放音频
                cancelledSound.play()
                // 视觉提示
                flashOverlay.color = "#ff6600"  // 橙色闪烁
                flashAnimation.restart()
                console.log("⚠️  条件单已取消！")
            }
        }
    }
    
    // 提交条件单逻辑
    function submitConditionOrder() {
        if (!orderController) {
            console.error("Error: OrderController is not ready")
            return
        }
        
        var instr = inputInstr.text.trim()
        var trigPrice = parseFloat(triggerPriceInput.text)
        
        if (instr === "" || isNaN(trigPrice)) {
            console.warn("Invalid input: Instrument or Trigger Price missing")
            return
        }
        
        // 映射方向: 0-买, 1-卖
        var direction = 0 
        if (dirGroup.checkedButton && dirGroup.checkedButton.text.indexOf("卖") >= 0) direction = 1
        
        // 映射开平: 0-开, 1-平, 3-平今
        var offset = 0
        var offText = offsetGroup.checkedButton ? offsetGroup.checkedButton.text : ""
        if (offText.indexOf("平今") >= 0) offset = 2 // Note: 2 maps to CloseToday in main.cpp logic
        else if (offText.indexOf("平") >= 0) offset = 1 // 1 maps to Close
        
        // 价格逻辑
        var pTypeIdx = priceTypeCombo.currentIndex
        // 0: Fix, 1: Last, 2: Opp, 3: Mkt
        var limitPrice = 0.0
        var priceTicks = 0
        
        if (pTypeIdx === 0) { // 指定价
            limitPrice = parseFloat(inputFixedPrice.text) || 0.0
        } else if (pTypeIdx === 1 || pTypeIdx === 2) { // 最新/对手
             priceTicks = inputPriceTicks.value // SpinBox value
        }
        
        // 构建指令数据 (Data Only)
        var data = {
            "instrument_id": instr,
            "exchange_id": "", 
            "direction": direction,
            "offset_flag": offset,
            "price_type": pTypeIdx, 
            "limit_price": limitPrice,
            "tick_offset": priceTicks,
            "volume": volumeSpinInput.value,
            "condition_compare": condCombo.currentIndex, // Send Integer Index
            "trigger_price": trigPrice,
            "strategy_id": ""
        }
        
        // Get Strategy ID from model if available
        if (orderController && orderController.strategyList && orderController.strategyList.length > 0 && strategyCombo.currentIndex >= 0) {
            data["strategy_id"] = orderController.strategyList[strategyCombo.currentIndex]["id"]
        }
        
        // 调用封装好的 C++ 接口发送
        orderController.sendConditionOrder(JSON.stringify(data))
        console.log("Condition Order Sent via Controller:", JSON.stringify(data))
    }

    // 模拟数据模型 (后续对接 C++ / DB)
    ListModel {
        id: conditionModel
        ListElement { 
            c_id: 101; c_instr: "rb2605"; c_cond: ">= 3600"; 
            c_action: "Buy Open 1"; c_status: "Pending"; c_price: "Limit 3605"
        }
        ListElement { 
            c_id: 102; c_instr: "ni2601"; c_cond: "<= 120000"; 
            c_action: "Sell Close 2"; c_status: "Triggered"; c_price: "Market" 
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        // 标题与状态栏
        RowLayout {
            Text {
                text: "云端条件单 (Cloud Condition Orders)"
                color: "#ffffff"
                font.pixelSize: 16
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            Label {
                text: "Status: Connected"
                color: "#4caf50"
                font.pixelSize: 12
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#333333" }

        // 主体区域
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            // 左侧：新建条件单表单
            Rectangle {
                Layout.preferredWidth: 280
                Layout.fillHeight: true
                color: "#252526"
                radius: 4
                border.color: "#3e3e42"
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 15
                    
                    Text { text: "新建条件 (New Condition)"; color: "#cccccc"; font.bold: true }
                    
                    // 1. 监控 (Monitor)
                    GroupBox {
                        Layout.fillWidth: true
                        title: "1. 监控 (Monitor)"
                        background: Rectangle { color: "transparent"; border.color: "#444444" }
                        label: Text { text: "Monitor"; color: "#888888" }
                        
                        // Compact Layout: [Instrument] [Operator] [Price]
                        RowLayout {
                            width: parent.width
                            spacing: 8
                            
                            // 合约
                            TextField { 
                                id: inputInstr
                                Layout.fillWidth: true
                                Layout.preferredWidth: 80
                                placeholderText: "合约"
                                text: orderController ? orderController.instrumentId : ""
                                color: "#ffffff"; background: Rectangle { color: "#333333"; radius: 2 }
                            }
                            
                            // 比较符
                            ComboBox { 
                                id: condCombo
                                Layout.preferredWidth: 60
                                model: [">", ">=", "<", "<="] 
                            }
                            
                            // 触发价 (带微调)
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 100
                                spacing: 0
                                
                                TextField { 
                                    id: triggerPriceInput
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 30
                                    placeholderText: "触发价格"
                                    text: (orderController && orderController.price > 0) ? orderController.price.toString() : ""
                                    color: "#ffffff"
                                    background: Rectangle { 
                                        color: "#333333" 
                                        radius: 2
                                        border.color: "#555555"
                                    }
                                    validator: DoubleValidator { bottom: 0.0; decimals: 2 }
                                    horizontalAlignment: Text.AlignRight
                                    rightPadding: 5
                                }
                                
                                Column {
                                    Layout.preferredWidth: 20
                                    Layout.fillHeight: true
                                    spacing: 1
                                    
                                    // Up
                                    Rectangle {
                                        width: 20; height: 15
                                        color: "#444444"
                                        Text { anchors.centerIn: parent; text: "▴"; color: "#ccc"; font.pixelSize: 10 }
                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: {
                                                var tick = orderController ? orderController.priceTick : 1.0
                                                var v = parseFloat(triggerPriceInput.text) || 0
                                                triggerPriceInput.text = (v + tick).toFixed(2)
                                            }
                                        }
                                    }
                                    // Down
                                    Rectangle {
                                        width: 20; height: 15
                                        color: "#444444"
                                        Text { anchors.centerIn: parent; text: "▾"; color: "#ccc"; font.pixelSize: 10 }
                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: {
                                                var tick = orderController ? orderController.priceTick : 1.0
                                                var v = parseFloat(triggerPriceInput.text) || 0
                                                var n = v - tick
                                                if(n<0) n=0
                                                triggerPriceInput.text = n.toFixed(2)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    // 2. 执行 (Action)
                    GroupBox {
                        Layout.fillWidth: true
                        title: "2. 执行 (Action)"
                        background: Rectangle { color: "transparent"; border.color: "#444444" }
                        label: Text { text: "Action"; color: "#888888" }
                        
                        ColumnLayout {
                            width: parent.width
                            spacing: 5
                            
                            // 方向 (买卖)
                            ButtonGroup { id: dirGroup }
                            RowLayout {
                                spacing: 10
                                Text { text: "买卖"; color: "#cccccc"; Layout.preferredWidth: 30 }
                                RadioButton { 
                                    text: "买入"; checked: true; ButtonGroup.group: dirGroup 
                                    contentItem: Text { text: parent.text; color: "#ff5252"; font.bold: true; leftPadding: parent.indicator.width + 4; verticalAlignment: Text.AlignVCenter }
                                }
                                RadioButton { 
                                    text: "卖出"; ButtonGroup.group: dirGroup 
                                    contentItem: Text { text: parent.text; color: "#00e676"; font.bold: true; leftPadding: parent.indicator.width + 4; verticalAlignment: Text.AlignVCenter }
                                }
                                Item { Layout.fillWidth: true }
                            }
                            
                            // 开平 (开仓/平今/平仓)
                            ButtonGroup { id: offsetGroup }
                            RowLayout {
                                spacing: 2 
                                Text { text: "开平"; color: "#cccccc"; Layout.preferredWidth: 30 }
                                RadioButton { 
                                    text: "开仓"; checked: true; ButtonGroup.group: offsetGroup 
                                    contentItem: Text { text: parent.text; color: "#ffd740"; font.bold: true; leftPadding: parent.indicator.width + 2; verticalAlignment: Text.AlignVCenter }
                                }
                                RadioButton { 
                                    text: "平今"; ButtonGroup.group: offsetGroup 
                                    contentItem: Text { text: parent.text; color: "#ffffff"; leftPadding: parent.indicator.width + 2; verticalAlignment: Text.AlignVCenter }
                                }
                                RadioButton { 
                                    text: "平仓"; ButtonGroup.group: offsetGroup 
                                    contentItem: Text { text: parent.text; color: "#ffffff"; leftPadding: parent.indicator.width + 2; verticalAlignment: Text.AlignVCenter }
                                }
                            }
                            
                            Rectangle { Layout.fillWidth: true; height: 1; color: "#333333"; Layout.topMargin: 5; Layout.bottomMargin: 5 }
                            
                            // 价格逻辑: 类型 + (数值 OR 跳数)
                            RowLayout {
                                Text { text: "基准"; color: "#cccccc"; Layout.preferredWidth: 30 }
                                ComboBox { 
                                    id: priceTypeCombo
                                    Layout.preferredWidth: 90
                                    model: ["指定价 (Fix)", "最新价 (Last)", "对手价 (Opp)", "市价 (Mkt)"] 
                                    currentIndex: 2 // Default to Opponent Price
                                }
                                
                                // 根据选择显示不同的输入控件
                                StackLayout {
                                    Layout.fillWidth: true
                                    currentIndex: priceTypeCombo.currentIndex
                                    
                                    // 0: 指定价 -> 输入绝对价格
                                    TextField { 
                                        id: inputFixedPrice
                                        placeholderText: "Price"
                                        text: (orderController && orderController.price > 0) ? orderController.price.toString() : ""
                                        color: "#ffffff"; background: Rectangle { color: "#333333" }
                                        horizontalAlignment: Text.AlignRight
                                        validator: DoubleValidator {}
                                    }
                                    
                                    // 1: 最新价 -> 输入偏移跳数
                                    RowLayout {
                                        Text { text: "+"; color: "#aaa" }
                                        SpinBox { 
                                            id: inputPriceTicks
                                            Layout.fillWidth: true; editable: true
                                            from: -100; to: 100; value: 0
                                        }
                                        Text { text: "ticks"; color: "#aaa"; font.pixelSize: 10 }
                                    }
                                    
                                    // 2: 对手价 -> 复用上面的 SpinBox 逻辑引用
                                    RowLayout {
                                        Text { text: "+"; color: "#aaa" }
                                        SpinBox { 
                                            id: inputPriceTicksOpp
                                            Layout.fillWidth: true; editable: true
                                            from: -100; to: 100; value: 0
                                        }
                                        Text { text: "ticks"; color: "#aaa"; font.pixelSize: 10 }
                                    }

                                    // 3: 市价 -> 无需输入
                                    Item { Layout.fillWidth: true } 
                                }
                            }
                            
                            RowLayout {
                                Text { text: "数量"; color: "#cccccc"; Layout.preferredWidth: 30 }
                                SpinBox { 
                                    id: volumeSpinInput
                                    Layout.fillWidth: true; value: 1; editable: true
                                    from: 1; to: 10000
                                }
                            }
                        }
                    }
                    
// 3. 策略归属 (可选)
                    RowLayout {
                        Text { text: "归属组:"; color: "#888888" }
                        ComboBox { 
                            id: strategyCombo
                            Layout.fillWidth: true
                            model: orderController ? orderController.strategyList : []
                            textRole: "name"
                            // Custom background for dark theme
                            delegate: ItemDelegate {
                                width: strategyCombo.width
                                contentItem: Text {
                                    text: modelData.name
                                    color: "#ffffff"
                                    font: strategyCombo.font
                                    elide: Text.ElideRight
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle { color: highlighted ? "#444444" : "#333333" }
                            }
                            contentItem: Text {
                                leftPadding: 10
                                rightPadding: strategyCombo.indicator.width + strategyCombo.spacing
                                text: strategyCombo.displayText
                                font: strategyCombo.font
                                color: "#ffffff"
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            background: Rectangle {
                                implicitWidth: 120
                                implicitHeight: 30
                                color: "#333333"
                                border.color: "#555555"
                                radius: 2
                            }
                        }
                    }

                    Connections {
                        target: orderController
                        function onConnectionChanged() {
                            if (orderController && orderController.coreConnected) orderController.queryStrategies()
                        }
                    }
                    Component.onCompleted: {
                        if (orderController && orderController.coreConnected) orderController.queryStrategies()
                    }

                    Item { Layout.fillHeight: true } 
                    
                    Button {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40
                        text: "提交条件单 (Submit)"
                        onClicked: submitConditionOrder()
                        
                        contentItem: Text {
                            text: parent.text
                            font.bold: true
                            color: "#ffffff"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.down ? "#1976d2" : "#2196f3"
                            radius: 4
                        }
                    }
                }
            }

            // 右侧：条件单列表
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#252526" // 与左侧保持一直
                radius: 4
                border.color: "#3e3e42"
                
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 1
                    
                    // Header
                    Rectangle {
                        Layout.fillWidth: true
                        height: 38 // Height increased
                        color: "#2d2d30"
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 15 // Increased margin
                            anchors.rightMargin: 15
                            spacing: 15
                            Text { text: "ID"; color: "#aaaaaa"; width: 50; font.pixelSize: 13; font.bold: true }
                            Text { text: "合约"; color: "#aaaaaa"; width: 80; font.pixelSize: 13; font.bold: true }
                            Text { text: "触发条件"; color: "#aaaaaa"; Layout.fillWidth: true; Layout.preferredWidth: 2; font.pixelSize: 13; font.bold: true }
                            Text { text: "执行动作"; color: "#aaaaaa"; Layout.fillWidth: true; Layout.preferredWidth: 3; font.pixelSize: 13; font.bold: true }
                            Text { text: "状态"; color: "#aaaaaa"; width: 70; font.pixelSize: 13; font.bold: true } // Wider
                            Text { text: "操作"; color: "#aaaaaa"; width: 60; font.pixelSize: 13; font.bold: true }
                        }
                    }
                    
                    ListView {
                        id: conditionListView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: orderController ? orderController.conditionOrderList : null
                        spacing: 2 // Tiny gap between rows
                        
                        // 全局 MouseArea：点击列表任何地方都关闭所有输入框
                        MouseArea {
                            anchors.fill: parent
                            z: -1
                            propagateComposedEvents: true
                            
                            onPressed: function(mouse) {
                                // 遍历所有 delegate，关闭所有输入框
                                for (var i = 0; i < conditionListView.count; i++) {
                                    var item = conditionListView.itemAtIndex(i)
                                    if (item) {
                                        // 查找并关闭该行的所有 SpinBox
                                        var children = item.children
                                        for (var j = 0; j < children.length; j++) {
                                            closeSpinBoxInItem(children[j])
                                        }
                                    }
                                }
                                mouse.accepted = false
                            }
                            
                            // 递归查找并关闭 NumberSpinBox
                            function closeSpinBoxInItem(item) {
                                if (!item) return
                                
                                // 检查是否有 closeIfVisible 函数（NumberSpinBox）
                                if (typeof item.closeIfVisible === "function") {
                                    item.closeIfVisible()
                                }
                                
                                // 递归检查子元素
                                if (item.children) {
                                    for (var i = 0; i < item.children.length; i++) {
                                        closeSpinBoxInItem(item.children[i])
                                    }
                                }
                            }
                        }
                        
                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 50
                            color: index % 2 === 0 ? "#1f1f1f" : "#252526"
                            
                            property var m: modelData
                            property string statusStr: {
                                if (m.status === 0) return "待触发"
                                if (m.status === 1) return "已触发"
                                if (m.status === 2) return "已取消"
                                return "未知"
                            }
                            property color statusColor: {
                                if (m.status === 0) return "#1976d2"
                                if (m.status === 1) return "#388e3c"
                                return "#616161"
                            }

                            // 点击空白区域关闭所有编辑框
                            MouseArea {
                                anchors.fill: parent
                                z: 0
                                propagateComposedEvents: true
                                
                                onPressed: function(mouse) {
                                    // 直接设置 visible = false 关闭所有输入框
                                    if (triggerPriceSpinBox.visible) triggerPriceSpinBox.visible = false
                                    if (limitPriceSpinBox.visible) limitPriceSpinBox.visible = false
                                    if (volumeSpinBox.visible) volumeSpinBox.visible = false
                                    // 让事件继续传播
                                    mouse.accepted = false
                                }
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: 8
                                
                                // ID列
                                Text { 
                                    text: m.request_id % 10000
                                    color: "#666666"
                                    width: 50
                                    font.pixelSize: 13
                                }
                                
                                // 合约列
                                Text { 
                                    text: m.instrument_id
                                    color: "#ffca28"
                                    width: 80
                                    font.bold: true 
                                    font.pixelSize: 14
                                }
                                
                                // 方向列
                                Text {
                                    text: (m.direction == "0" ? "买入" : "卖出") + " " + (m.offset_flag == "0" ? "开仓" : "平仓")
                                    color: m.direction == "0" ? "#ff5252" : "#00e676"
                                    width: 80
                                    font.pixelSize: 13
                                }
                                
                                // 条件列
                                Text {
                                    text: [">", "≥", "<", "≤"][m.compare_type] || "?"
                                    color: "#ffffff"
                                    width: 70
                                    font.pixelSize: 16
                                    font.bold: true
                                }
                                
                                // 触发价列（可编辑）
                                Item {
                                    Layout.preferredWidth: 100
                                    Layout.fillHeight: true
                                    property double priceTick: orderController ? orderController.getInstrumentPriceTick(m.instrument_id) : 1.0
                                    Text {
                                        id: triggerPriceText
                                        anchors.centerIn: parent
                                        text: m.trigger_price.toFixed(triggerPriceSpinBox.decimals)
                                        color: "#ffffff"
                                        font.pixelSize: 14
                                        visible: !triggerPriceSpinBox.visible
                                    }
                                    NumberSpinBox {
                                        id: triggerPriceSpinBox
                                        objectName: "triggerPriceSpinBox_" + index
                                        anchors.centerIn: parent
                                        width: parent.width - 4
                                        height: parent.height - 4
                                        visible: false
                                        realValue: m.trigger_price
                                        priceTick: parent.priceTick
                                        isPrice: true
                                        onValueCommitted: {
                                            if (orderController && Math.abs(value - m.trigger_price) > 0.00001) {
                                                orderController.modifyConditionOrder(m.request_id, value, m.limit_price, m.volume)
                                            }
                                        }
                                        onEditingFinished: {
                                            visible = false
                                        }
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        enabled: m.status === 0 && !triggerPriceSpinBox.visible
                                        onClicked: {
                                            // 先关闭其他输入框
                                            if (limitPriceSpinBox.visible) limitPriceSpinBox.visible = false
                                            if (volumeSpinBox.visible) volumeSpinBox.visible = false
                                            // 显示当前输入框
                                            triggerPriceSpinBox.visible = true
                                            triggerPriceSpinBox.forceActiveFocus()
                                            triggerPriceSpinBox.contentItem.selectAll()
                                        }
                                    }
                                }
// 成交价列（可编辑）
                                Item {
                                    Layout.preferredWidth: 100
                                    Layout.fillHeight: true
                                    property double priceTick: orderController ? orderController.getInstrumentPriceTick(m.instrument_id) : 1.0
                                    Text {
                                        id: limitPriceText
                                        anchors.centerIn: parent
                                        text: m.limit_price.toFixed(limitPriceSpinBox.decimals)
                                        color: "#ffffff"
                                        font.pixelSize: 14
                                        visible: !limitPriceSpinBox.visible
                                    }
                                    NumberSpinBox {
                                        id: limitPriceSpinBox
                                        objectName: "limitPriceSpinBox_" + index
                                        anchors.centerIn: parent
                                        width: parent.width - 4
                                        height: parent.height - 4
                                        visible: false
                                        realValue: m.limit_price
                                        priceTick: parent.priceTick
                                        isPrice: true
                                        onValueCommitted: {
                                            if (orderController && Math.abs(value - m.limit_price) > 0.00001) {
                                                orderController.modifyConditionOrder(m.request_id, m.trigger_price, value, m.volume)
                                            }
                                        }
                                        onEditingFinished: {
                                            visible = false
                                        }
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        enabled: m.status === 0 && !limitPriceSpinBox.visible
                                        onClicked: {
                                            // 先关闭其他输入框
                                            if (triggerPriceSpinBox.visible) triggerPriceSpinBox.visible = false
                                            if (volumeSpinBox.visible) volumeSpinBox.visible = false
                                            // 显示当前输入框
                                            limitPriceSpinBox.visible = true
                                            limitPriceSpinBox.forceActiveFocus()
                                            limitPriceSpinBox.contentItem.selectAll()
                                        }
                                    }
                                }
// 手数列（可编辑）
                                Item {
                                    Layout.preferredWidth: 80
                                    Layout.fillHeight: true
                                    Text {
                                        id: volumeText
                                        anchors.centerIn: parent
                                        text: Math.round(m.volume)
                                        color: "#ffffff"
                                        font.pixelSize: 14
                                        visible: !volumeSpinBox.visible
                                    }
                                    NumberSpinBox {
                                        id: volumeSpinBox
                                        objectName: "volumeSpinBox_" + index
                                        anchors.centerIn: parent
                                        width: parent.width - 4
                                        height: parent.height - 4
                                        visible: false
                                        realValue: m.volume
                                        priceTick: 1.0
                                        isPrice: false
                                        multiplier: 1
                                        onValueCommitted: {
                                            if (orderController && value !== m.volume) {
                                                orderController.modifyConditionOrder(m.request_id, m.trigger_price, m.limit_price, value)
                                            }
                                        }
                                        onEditingFinished: {
                                            visible = false
                                        }
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        enabled: m.status === 0 && !volumeSpinBox.visible
                                        onClicked: {
                                            // 先关闭其他输入框
                                            if (triggerPriceSpinBox.visible) triggerPriceSpinBox.visible = false
                                            if (limitPriceSpinBox.visible) limitPriceSpinBox.visible = false
                                            // 显示当前输入框
                                            volumeSpinBox.visible = true
                                            volumeSpinBox.forceActiveFocus()
                                            volumeSpinBox.contentItem.selectAll()
                                        }
                                    }
                                }
// 策略列
                                Text {
                                    text: m.strategy_id || "-"
                                    color: "#999999"
                                    width: 80
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                }
                                
                                // 状态列
                                Rectangle {
                                    width: 70
                                    height: 24
                                    color: statusColor
                                    radius: 4
                                    Text { 
                                        anchors.centerIn: parent
                                        text: statusStr
                                        color: "white"
                                        font.pixelSize: 12
                                        font.bold: true
                                    }
                                }
                                
                                // 撤单按钮
                                Rectangle {
                                    width: 56
                                    height: 26
                                    color: "#2b2b2b"
                                    border.color: m.status === 0 ? "#ff5252" : "transparent"
                                    radius: 4
                                    visible: m.status === 0
                                    
                                    Text { 
                                        anchors.centerIn: parent
                                        text: "撤单"
                                        color: "#ff5252"
                                        font.pixelSize: 13
                                    }
                                    
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        hoverEnabled: true
                                        onEntered: parent.color = "#3a1c1c"
                                        onExited: parent.color = "#2b2b2b"
                                        onClicked: {
                                            if(orderController) {
                                                console.log("撤销条件单 " + m.request_id)
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
}
}
