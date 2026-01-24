import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// 条件单管理面板 - 全新设计
Rectangle {
    id: root
    color: "#1e1e1e"
    
    // 焦点捕捉器：用于承载“无处安放”的焦点
    Item {
        id: focusTrap
        focus: true
    }

    // 全局点击处理：点击空白区域清除焦点，触发提交
    MouseArea {
        anchors.fill: parent
        z: -1 // 确保在最底层
        onClicked: {
            focusTrap.forceActiveFocus() // 将焦点强行转移到 trap
        }
    }
    
    property var marketModel
    property var orderController
    
    // 选中的合约信息
    property string selectedInstrument: ""
    property real selectedLastPrice: 0.0
    
    // 辅助函数：尝试选中合约
    function selectCurrentInstrument() {
        if (!selectedInstrument || !root.marketModel) return
        
        var target = selectedInstrument.trim()
        
        // MarketModel 是 C++ QAbstractListModel，没有 subscribedInstruments 属性
        // 但提供了 getAllInstruments() 方法返回 QStringList
        var instruments = root.marketModel.getAllInstruments()
        
        if (instruments) {
            console.log("Debug: 目标合约=" + target + ", 列表长度=" + instruments.length)
            for (var i = 0; i < instruments.length; i++) {
                if (instruments[i] == target) {
                    if (instrumentCombo.currentIndex !== i) {
                        instrumentCombo.currentIndex = i
                        console.log("Debug: 成功选中索引:", i, target)
                    } else {
                        console.log("Debug: 索引已是正确:", i)
                    }
                    return
                }
            }
            console.log("Debug: 列表遍历结束，未找到目标:", target)
        } else {
            console.log("Debug: 订阅列表为空")
        }
    }

    // 监听选中合约变化
    onSelectedInstrumentChanged: {
        selectCurrentInstrument()
        
        // 重置触发价格绑定
        if (selectedLastPrice > 0) {
             triggerPriceInput.text = Qt.binding(function() { return selectedLastPrice > 0 ? selectedLastPrice.toFixed(2) : "" })
        }
    }
    
    // 监听模型变化
    // MarketModel 是 QAbstractListModel，会自动通知 ComboBox 更新
    // 我们只需要监听 ComboBox 的 count 变化来触发选中逻辑
    /*
    Connections {
        target: root.marketModel
        // QAbstractListModel 没有 onSubscribedInstrumentsChanged 信号
    }
    */
    
    // 监听ComboBox自身变化
    Connections {
        target: instrumentCombo
        function onCountChanged() {
            selectCurrentInstrument()
        }
        function onModelChanged() {
            selectCurrentInstrument()
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
                            text: root.orderController ? root.orderController.conditionOrderList.filter(function(o) { 
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
                            text: root.orderController ? root.orderController.conditionOrderList.filter(function(o) { 
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
                    
                    // 合约与策略选择
                    Column {
                        Layout.fillWidth: true
                        spacing: 6
                        
                        Row {
                            width: parent.width
                            spacing: 12
                            
                            // 合约部分
                            Column {
                                width: (parent.width - 12) * 0.4
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
                                    model: root.marketModel
                                    textRole: "instrumentId"
                                    
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
                            
                            // 策略部分
                            Column {
                                width: (parent.width - 12) * 0.6
                                spacing: 6
                                Text {
                                    text: "策略类型"
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: "#ffffff"
                                }
                                ComboBox {
                                    id: strategyCombo
                                    width: parent.width
                                    height: 32
                                    model: root.orderController ? root.orderController.strategyList : []
                                    textRole: "name"
                                    
                                    contentItem: Text {
                                        leftPadding: 15
                                        text: strategyCombo.displayText
                                        font.pixelSize: 15
                                        color: "#ffffff"
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    
                                    background: Rectangle {
                                        color: "#1e1e1e"
                                        border.color: strategyCombo.activeFocus ? "#2196f3" : "#555555"
                                        border.width: 2
                                        radius: 6
                                    }
                                }
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
                                text: selectedLastPrice > 0 ? selectedLastPrice.toFixed(2) : ""
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
                                
                                // 微调按钮 (仿 OrderPanel 样式)
                                Column {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 1
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 24
                                    height: parent.height - 2
                                    spacing: 0
                                    
                                    // 上调按钮
                                    Rectangle {
                                        width: 24
                                        height: parent.height / 2
                                        color: priceUpArea.containsMouse ? "#444444" : "#333333"
                                        radius: 2
                                        
                                        Text { 
                                            text: "▴"
                                            color: "#cccccc"
                                            font.pixelSize: 14
                                            anchors.centerIn: parent 
                                        }
                                        
                                        MouseArea {
                                            id: priceUpArea
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            onClicked: {
                                                var val = parseFloat(triggerPriceInput.text) || 0
                                                var tick = (root.orderController && root.orderController.priceTick) ? root.orderController.priceTick : 1.0
                                                // 精度处理，防止浮点数误差
                                                var newVal = (Math.round((val + tick) / tick) * tick)
                                                triggerPriceInput.text = newVal.toFixed(2) // 这会断开绑定
                                            }
                                        }
                                    }
                                    
                                    // 下调按钮
                                    Rectangle {
                                        width: 24
                                        height: parent.height / 2
                                        color: priceDownArea.containsMouse ? "#444444" : "#333333"
                                        radius: 2
                                        
                                        Text { 
                                            text: "▾"
                                            color: "#cccccc"
                                            font.pixelSize: 14
                                            anchors.centerIn: parent 
                                        }
                                        
                                        MouseArea {
                                            id: priceDownArea
                                            anchors.fill: parent
                                            hoverEnabled: true
                                            onClicked: {
                                                var val = parseFloat(triggerPriceInput.text) || 0
                                                var tick = (root.orderController && root.orderController.priceTick) ? root.orderController.priceTick : 1.0
                                                var newVal = (Math.round((val - tick) / tick) * tick)
                                                if(newVal < 0) newVal = 0
                                                triggerPriceInput.text = newVal.toFixed(2) // 这会断开绑定
                                            }
                                        }
                                    }
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
                                width: 24
                                height: 24
                                radius: 12
                                border.color: "#4caf50"
                                border.width: 2
                                color: "transparent"
                                anchors.verticalCenter: parent.verticalCenter
                                
                                Rectangle {
                                    width: 14
                                    height: 14
                                    radius: 7
                                    anchors.centerIn: parent
                                    color: "#4caf50"
                                    visible: buyRadio.checked
                                }
                            }
                            
                            contentItem: Text {
                                text: "买"
                                font.pixelSize: 13
                                color: "#ffffff"
                                leftPadding: 28
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        
                        RadioButton {
                            id: sellRadio
                            height: 30
                            
                            indicator: Rectangle {
                                width: 24
                                height: 24
                                radius: 12
                                border.color: "#f44336"
                                border.width: 2
                                color: "transparent"
                                anchors.verticalCenter: parent.verticalCenter
                                
                                Rectangle {
                                    width: 14
                                    height: 14
                                    radius: 7
                                    anchors.centerIn: parent
                                    color: "#f44336"
                                    visible: sellRadio.checked
                                }
                            }
                            
                            contentItem: Text {
                                text: "卖"
                                font.pixelSize: 13
                                color: "#ffffff"
                                leftPadding: 28
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
                                width: 24
                                height: 24
                                radius: 12
                                border.color: "#ffa726"
                                border.width: 2
                                color: "transparent"
                                anchors.verticalCenter: parent.verticalCenter
                                
                                Rectangle {
                                    width: 14
                                    height: 14
                                    radius: 7
                                    anchors.centerIn: parent
                                    color: "#ffa726"
                                    visible: openRadio.checked
                                }
                            }
                            
                            contentItem: Text {
                                text: "开仓"
                                font.pixelSize: 13
                                color: "#ffffff"
                                leftPadding: 28
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        
                        RadioButton {
                            id: closeRadio
                            height: 30
                            
                            indicator: Rectangle {
                                width: 24
                                height: 24
                                radius: 12
                                border.color: "#ffa726"
                                border.width: 2
                                color: "transparent"
                                anchors.verticalCenter: parent.verticalCenter
                                
                                Rectangle {
                                    width: 14
                                    height: 14
                                    radius: 7
                                    anchors.centerIn: parent
                                    color: "#ffa726"
                                    visible: closeRadio.checked
                                }
                            }
                            
                            contentItem: Text {
                                text: "平仓"
                                font.pixelSize: 13
                                color: "#ffffff"
                                leftPadding: 28
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        
                        RadioButton {
                            id: closeTodayRadio
                            height: 30
                            
                            indicator: Rectangle {
                                width: 24
                                height: 24
                                radius: 12
                                border.color: "#ffa726"
                                border.width: 2
                                color: "transparent"
                                anchors.verticalCenter: parent.verticalCenter
                                
                                Rectangle {
                                    width: 14
                                    height: 14
                                    radius: 7
                                    anchors.centerIn: parent
                                    color: "#ffa726"
                                    visible: closeTodayRadio.checked
                                }
                            }
                            
                            contentItem: Text {
                                text: "平今"
                                font.pixelSize: 13
                                color: "#ffffff"
                                leftPadding: 28
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
                            text: "1" // 默认1手
                            
                            background: Rectangle {
                                color: "#1e1e1e"
                                border.color: volumeInput.activeFocus ? "#2196f3" : "#555555"
                                border.width: 2
                                radius: 6
                            }
                            
                            // 微调按钮 (仿 OrderPanel 样式)
                            Column {
                                anchors.right: parent.right
                                anchors.rightMargin: 1
                                anchors.verticalCenter: parent.verticalCenter
                                width: 24
                                height: parent.height - 2
                                spacing: 0
                                
                                // 上调按钮
                                Rectangle {
                                    width: 24
                                    height: parent.height / 2
                                    color: volUpArea.containsMouse ? "#444444" : "#333333"
                                    radius: 2
                                    
                                    Text { 
                                        text: "▴"
                                        color: "#cccccc"
                                        font.pixelSize: 14
                                        anchors.centerIn: parent 
                                    }
                                    
                                    MouseArea {
                                        id: volUpArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: {
                                            var val = parseInt(volumeInput.text) || 0
                                            volumeInput.text = String(val + 1)
                                        }
                                    }
                                }
                                
                                // 下调按钮
                                Rectangle {
                                    width: 24
                                    height: parent.height / 2
                                    color: volDownArea.containsMouse ? "#444444" : "#333333"
                                    radius: 2
                                    
                                    Text { 
                                        text: "▾"
                                        color: "#cccccc"
                                        font.pixelSize: 14
                                        anchors.centerIn: parent 
                                    }
                                    
                                    MouseArea {
                                        id: volDownArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: {
                                            var val = parseInt(volumeInput.text) || 0
                                            if (val > 1) {
                                                volumeInput.text = String(val - 1)
                                            }
                                        }
                                    }
                                }
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

                // 点击列表背景/空白处清除焦点
                MouseArea {
                    anchors.fill: parent
                    onClicked: focusTrap.forceActiveFocus()
                }
                
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
                        model: root.orderController ? root.orderController.conditionOrderList : null
                        spacing: 2
                        
                        interactive: contentHeight > height
                        
                        // 点击列表空白处清除焦点
                        MouseArea {
                            anchors.fill: parent
                            z: -1
                            onClicked: focusTrap.forceActiveFocus()
                        }
                        
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
                                Row {
                                    width: 150
                                    height: parent.height
                                    spacing: 4
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    Text {
                                        text: {
                                            var symbols = [">", "≥", "<", "≤"]
                                            return symbols[m.compare_type]
                                        }
                                        color: "#ffa726"
                                        font.pixelSize: 15
                                        font.bold: true
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    
                                    TextField {
                                        id: editTriggerPrice
                                        text: m.trigger_price.toFixed(2)
                                        width: 100
                                        height: 32
                                        font.pixelSize: 15
                                        // 选中时白色，否则标准色
                                        color: activeFocus ? "#ffffff" : "#ffa726"
                                        anchors.verticalCenter: parent.verticalCenter
                                        enabled: m.status === 0
                                        selectByMouse: true
                                        rightPadding: activeFocus ? 26 : 6
                                        background: Rectangle {
                                            // 选中时背景深色(凹陷感)+边框，未选中则透明无边框
                                            color: parent.activeFocus ? "#111111" : "transparent"
                                            border.color: parent.activeFocus ? "#2196f3" : "transparent"
                                            border.width: parent.activeFocus ? 1 : 0
                                            radius: 2
                                        }
                                        
                                        // 自动提交 (失去焦点时)
                                        onEditingFinished: {
                                            commitChange()
                                        }
                                        
                                        function commitChange() {
                                            if (m.status !== 0 || !orderController) return
                                            
                                            var val = parseFloat(text)
                                            if (isNaN(val) || Math.abs(val - m.trigger_price) < 0.0001) return
                                            
                                            // 同时获取另一字段当前值
                                            var vol = parseInt(editVolume.text)
                                            if (isNaN(vol) || vol <= 0) vol = m.volume
                                            
                                            console.log("Auto Modify Price:", val)
                                            orderController.modifyConditionOrder(m.request_id, val, m.limit_price, vol)
                                            focus = false // Ensure focus cleared
                                        }

                                        // 微调按钮 (仅更新UI，不提交，保持焦点)
                                        Column {
                                            anchors.right: parent.right
                                            anchors.rightMargin: 1
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: 24
                                            height: parent.height - 2
                                            spacing: 0
                                            visible: parent.activeFocus
                                            
                                            Rectangle {
                                                width: 24; height: parent.height/2
                                                color: upAreaP.containsMouse ? "#333333" : "transparent"
                                                Text { text: "▴"; color: "#cccccc"; anchors.centerIn: parent; font.pixelSize: 14 }
                                                MouseArea {
                                                    id: upAreaP; anchors.fill: parent; hoverEnabled: true
                                                    onClicked: {
                                                        editTriggerPrice.forceActiveFocus()
                                                        var v = parseFloat(editTriggerPrice.text) || 0
                                                        var tick = orderController ? orderController.getInstrumentPriceTick(m.instrument_id) : 1.0
                                                        v = (Math.round((v + tick)/tick) * tick)
                                                        editTriggerPrice.text = v.toFixed(2)
                                                    }
                                                }
                                            }
                                            Rectangle {
                                                width: 24; height: parent.height/2
                                                color: downAreaP.containsMouse ? "#333333" : "transparent"
                                                Text { text: "▾"; color: "#cccccc"; anchors.centerIn: parent; font.pixelSize: 14 }
                                                MouseArea {
                                                    id: downAreaP; anchors.fill: parent; hoverEnabled: true
                                                    onClicked: {
                                                        editTriggerPrice.forceActiveFocus()
                                                        var v = parseFloat(editTriggerPrice.text) || 0
                                                        var tick = orderController ? orderController.getInstrumentPriceTick(m.instrument_id) : 1.0
                                                        v = (Math.round((v - tick)/tick) * tick)
                                                        if (v < 0) v = 0
                                                        editTriggerPrice.text = v.toFixed(2)
                                                    }
                                                }
                                            }
                                        }
                                    }
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
                                    
                                    TextField {
                                        id: editVolume
                                        text: m.volume
                                        width: 80
                                        height: 32
                                        font.pixelSize: 15
                                        // 选中时白色，否则标准色
                                        color: activeFocus ? "#ffffff" : "#ffffff"
                                        anchors.verticalCenter: parent.verticalCenter
                                        enabled: m.status === 0
                                        selectByMouse: true
                                        rightPadding: activeFocus ? 26 : 6
                                        background: Rectangle {
                                            color: parent.activeFocus ? "#111111" : "transparent"
                                            border.color: parent.activeFocus ? "#2196f3" : "transparent"
                                            border.width: parent.activeFocus ? 1 : 0
                                            radius: 2
                                        }
                                        
                                        onEditingFinished: {
                                            commitChange()
                                        }
                                        
                                        function commitChange() {
                                            if (m.status !== 0 || !orderController) return
                                            
                                            var vol = parseInt(text)
                                            if (isNaN(vol) || vol <= 0 || vol === m.volume) return
                                            
                                            var price = parseFloat(editTriggerPrice.text)
                                            if (isNaN(price)) price = m.trigger_price
                                            
                                            console.log("Auto Modify Volume:", vol)
                                            orderController.modifyConditionOrder(m.request_id, price, m.limit_price, vol)
                                            focus = false
                                        }

                                        // 微调按钮 (仅更新UI，不提交，保持焦点)
                                        Column {
                                            anchors.right: parent.right
                                            anchors.rightMargin: 1
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: 24
                                            height: parent.height - 2
                                            spacing: 0
                                            visible: parent.activeFocus
                                            
                                            Rectangle {
                                                width: 24; height: parent.height/2
                                                color: upAreaV.containsMouse ? "#333333" : "transparent"
                                                Text { text: "▴"; color: "#cccccc"; anchors.centerIn: parent; font.pixelSize: 14 }
                                                MouseArea {
                                                    id: upAreaV; anchors.fill: parent; hoverEnabled: true
                                                    onClicked: {
                                                        editVolume.forceActiveFocus()
                                                        var v = parseInt(editVolume.text) || 0
                                                        editVolume.text = v + 1
                                                    }
                                                }
                                            }
                                            Rectangle {
                                                width: 24; height: parent.height/2
                                                color: downAreaV.containsMouse ? "#333333" : "transparent"
                                                Text { text: "▾"; color: "#cccccc"; anchors.centerIn: parent; font.pixelSize: 14 }
                                                MouseArea {
                                                    id: downAreaV; anchors.fill: parent; hoverEnabled: true
                                                    onClicked: {
                                                        editVolume.forceActiveFocus()
                                                        var v = parseInt(editVolume.text) || 0
                                                        if (v > 1) {
                                                            editVolume.text = v - 1
                                                        }
                                                    }
                                                }
                                            }
                                        }
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
                                Row {
                                    spacing: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    
                                    /* 修改按钮已移除，功能合并至输入框
                                    Button { ... }
                                    */
                                    
                                    Button {
                                        width: 60
                                        height: 32
                                        text: "撤销"
                                        visible: m.status === 0
                                        
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
        if (isNaN(limitPrice)) limitPrice = 0.0 // 默认为0
        
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
        
        var compareType = conditionTypeCombo.currentIndex // 0:>, 1:>=, 2:<, 3:<=
        
        // 获取策略ID
        // 如果 model 是 ListModel/Array, 取对应 index 的 id
        // 我们绑定的是 root.orderController.strategyList (QVariantList)
        var strategyId = ""
        var strategyIdx = strategyCombo.currentIndex
        var sList = root.orderController.strategyList
        if (sList && strategyIdx >= 0 && strategyIdx < sList.length) {
            strategyId = sList[strategyIdx].id
        }
        
        var data = {
            "instrument_id": instrument,
            "trigger_price": triggerPrice,
            "compare_type": compareType,
            "direction": direction,
            "offset_flag": offsetFlag,
            "volume": volume,
            "limit_price": limitPrice,
            "strategy_id": strategyId
        }
        
        console.log("Submit Condition Order:", JSON.stringify(data))
        
        orderController.sendConditionOrder(JSON.stringify(data))
        
        // 清空输入
        // triggerPriceInput.text = "" 
        // volumeInput.text = "1"
        // 不清空可能方便连续下单？或者只清空价格？
        // 这里选择不清空，保持用户输入状态
    }
}
