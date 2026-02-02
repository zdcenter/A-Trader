// qmllint disable import
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

/**
 * 条件单列表面板 (从 ConditionOrderPanel 分离)
 */
FocusScope {
    id: root
    
    // 激活状态样式
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: root.activeFocus ? "#2196f3" : "transparent"
        border.width: root.activeFocus ? 2 : 0
        z: 100
    }
    
    property var orderController
    
    // 请求切换到条件单下单面板
    signal requestConditionOrderPanel()
    
    // 焦点捕捉器
    Item {
        id: focusTrap
        focus: true
    }

    // 全局点击处理
    MouseArea {
        anchors.fill: parent
        z: -1 
        onClicked: {
            focusTrap.forceActiveFocus() 
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#252526"
        border.color: "#3e3e42"
        border.width: 1
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0
            
            // 模块标题
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                color: "#2d2d30"
                
                Text {
                    text: "  📋 条件单记录"
                    color: "#cccccc"
                    font.pixelSize: 13
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }

                // 快捷切换到条件单下单面板
                Rectangle {
                    anchors.right: parent.right
                    anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    width: 24
                    height: 24
                    radius: 2
                    color: addBtnHover.containsMouse ? "#444444" : "transparent"
                    
                    Text {
                        text: "+"
                        color: "#cccccc"
                        font.pixelSize: 18
                        font.bold: true
                        anchors.centerIn: parent
                        anchors.verticalCenterOffset: -2
                    }
                    
                    MouseArea {
                        id: addBtnHover
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.requestConditionOrderPanel()
                        ToolTip.visible: containsMouse
                        ToolTip.text: "去添加条件单"
                        ToolTip.delay: 500
                    }
                }
            }

            // 列表表头
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                color: "#2d2d30"
                
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20
                    spacing: 8
                    
                    Text { text: "合约"; color: "#aaaaaa"; Layout.preferredWidth: 80; font.pixelSize: 13; font.bold: true; verticalAlignment: Text.AlignVCenter; Layout.fillHeight: true }
                    Text { text: "触发条件"; color: "#aaaaaa"; Layout.preferredWidth: 160; font.pixelSize: 13; font.bold: true; verticalAlignment: Text.AlignVCenter; Layout.fillHeight: true }
                    Text { text: "执行动作"; color: "#aaaaaa"; Layout.fillWidth: true; font.pixelSize: 13; font.bold: true; verticalAlignment: Text.AlignVCenter; Layout.fillHeight: true }
                    Text { text: "状态"; color: "#aaaaaa"; Layout.preferredWidth: 60; font.pixelSize: 13; font.bold: true; verticalAlignment: Text.AlignVCenter; Layout.fillHeight: true }
                    Text { text: "操作"; color: "#aaaaaa"; Layout.preferredWidth: 50; font.pixelSize: 13; font.bold: true; verticalAlignment: Text.AlignVCenter; Layout.fillHeight: true }
                }
            }
            
            // 条件单列表
            ListView {
                id: conditionListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: root.orderController ? root.orderController.conditionOrderList : null
                spacing: 2
                currentIndex: -1  // 禁用默认选中
                onCountChanged: currentIndex = -1 // 强制重置确保不默认选中
                Component.onCompleted: currentIndex = -1
                ScrollBar.vertical: ScrollBar {}
                
                interactive: contentHeight > height
                
                // 点击列表空白处清除焦点
                MouseArea {
                    anchors.fill: parent
                    z: -1
                    onClicked: focusTrap.forceActiveFocus()
                }
                
                delegate: Rectangle {
                    id: conditionDelegate
                    width: conditionListView.width
                    height: 35
                    
                    property var m: modelData
                    visible: m !== undefined && m !== null
                    
                    // 统一的选中和悬停样式
                    color: {
                        if (conditionMouseArea.containsMouse && conditionListView.currentIndex === index) return "#3a5a7a"
                        if (conditionListView.currentIndex === index) return "#2c5d87"
                        if (conditionMouseArea.containsMouse) return "#2a2a2a"
                        return index % 2 === 0 ? "#1e1e1e" : "#252526"
                    }
                    
                    // 鼠标区域用于悬停效果
                    MouseArea {
                        id: conditionMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        z: -1
                        propagateComposedEvents: true
                        onClicked: (mouse) => {
                            conditionListView.currentIndex = index
                            mouse.accepted = false
                        }
                    }
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 20
                        anchors.rightMargin: 20
                        spacing: 8
                        
                        // 合约
                        Text {
                            text: m.instrument_id
                            color: "#ffffff"
                            font.pixelSize: 13
                            Layout.preferredWidth: 80
                            verticalAlignment: Text.AlignVCenter
                            Layout.fillHeight: true
                        }
                        
                        // 触发条件
                        Row {
                            Layout.preferredWidth: 160
                            Layout.fillHeight: true
                            spacing: 4
                            
                            Text {
                                text: {
                                    var symbols = [">", "≥", "<", "≤"]
                                    return symbols[m.compare_type]
                                }
                                color: "#ffa726"
                                font.pixelSize: 13
                                font.bold: true
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            
                            TextField {
                                id: editTriggerPrice
                                text: m.trigger_price.toFixed(2)
                                width: 100
                                height: 24
                                font.pixelSize: 13
                                color: activeFocus ? "#ffffff" : "#ffa726"
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
                                    if (m.status !== 0 || !root.orderController) return
                                    
                                    var val = parseFloat(text)
                                    if (isNaN(val) || Math.abs(val - m.trigger_price) < 0.0001) return
                                    
                                    var vol = parseInt(editVolume.text)
                                    if (isNaN(vol) || vol <= 0) vol = m.volume
                                    
                                    console.log("Auto Modify Price:", val)
                                    root.orderController.modifyConditionOrder(m.request_id, val, m.limit_price, vol)
                                    focus = false 
                                }

                                // 微调按钮
                                Column {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 1
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 20
                                    height: parent.height - 2
                                    spacing: 0
                                    visible: parent.activeFocus
                                    
                                    Rectangle {
                                        width: 20; height: parent.height/2
                                        color: upAreaP.containsMouse ? "#333333" : "transparent"
                                        Text { text: "▴"; color: "#cccccc"; anchors.centerIn: parent; font.pixelSize: 10 }
                                        MouseArea {
                                            id: upAreaP; anchors.fill: parent; hoverEnabled: true
                                            onClicked: {
                                                editTriggerPrice.forceActiveFocus()
                                                var v = parseFloat(editTriggerPrice.text) || 0
                                                var tick = root.orderController ? root.orderController.getInstrumentPriceTick(m.instrument_id) : 1.0
                                                v = (Math.round((v + tick)/tick) * tick)
                                                editTriggerPrice.text = v.toFixed(2)
                                            }
                                        }
                                    }
                                    Rectangle {
                                        width: 20; height: parent.height/2
                                        color: downAreaP.containsMouse ? "#333333" : "transparent"
                                        Text { text: "▾"; color: "#cccccc"; anchors.centerIn: parent; font.pixelSize: 10 }
                                        MouseArea {
                                            id: downAreaP; anchors.fill: parent; hoverEnabled: true
                                            onClicked: {
                                                editTriggerPrice.forceActiveFocus()
                                                var v = parseFloat(editTriggerPrice.text) || 0
                                                var tick = root.orderController ? root.orderController.getInstrumentPriceTick(m.instrument_id) : 1.0
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
                            Layout.preferredWidth: 240
                            Layout.fillHeight: true
                            
                            Text {
                                text: (m.direction == 0 || m.direction == '0') ? "买" : "卖"
                                color: (m.direction == 0 || m.direction == '0') ? "#f44336" : "#4caf50"
                                font.pixelSize: 13
                                font.bold: true
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            
                            Text {
                                text: {
                                    var f = m.offset_flag
                                    if (f == 0 || f == '0') return "开仓"
                                    if (f == 3 || f == '3') return "平今"
                                    if (f == 4 || f == '4') return "平仓"
                                    return "平仓" 
                                }
                                color: "#cccccc"
                                font.pixelSize: 13
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            
                            TextField {
                                id: editVolume
                                text: m.volume
                                width: 60
                                height: 24
                                font.pixelSize: 13
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
                                    if (m.status !== 0 || !root.orderController) return
                                    
                                    var vol = parseInt(text)
                                    if (isNaN(vol) || vol <= 0 || vol === m.volume) return
                                    
                                    var price = parseFloat(editTriggerPrice.text)
                                    if (isNaN(price)) price = m.trigger_price
                                    
                                    console.log("Auto Modify Volume:", vol)
                                    root.orderController.modifyConditionOrder(m.request_id, price, m.limit_price, vol)
                                    focus = false
                                }

                                // 微调按钮
                                Column {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 1
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 20
                                    height: parent.height - 2
                                    spacing: 0
                                    visible: parent.activeFocus
                                    
                                    Rectangle {
                                        width: 20; height: parent.height/2
                                        color: upAreaV.containsMouse ? "#333333" : "transparent"
                                        Text { text: "▴"; color: "#cccccc"; anchors.centerIn: parent; font.pixelSize: 10 }
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
                                        width: 20; height: parent.height/2
                                        color: downAreaV.containsMouse ? "#333333" : "transparent"
                                        Text { text: "▾"; color: "#cccccc"; anchors.centerIn: parent; font.pixelSize: 10 }
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
                                text: "@"
                                color: "#888888"
                                font.pixelSize: 13
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            TextField {
                                id: editLimitPrice
                                text: m.limit_price > 0 ? m.limit_price.toFixed(2) : "0"
                                width: 80
                                height: 24
                                font.pixelSize: 13
                                color: activeFocus ? "#ffffff" : "#cccccc"
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

                                onEditingFinished: commitChange()

                                function commitChange() {
                                    if (m.status !== 0 || !root.orderController) return
                                    
                                    var val = parseFloat(text)
                                    if (isNaN(val)) return
                                    
                                    // Get latest values from other fields
                                    var triggerP = parseFloat(editTriggerPrice.text)
                                    if (isNaN(triggerP)) triggerP = m.trigger_price
                                    
                                    var vol = parseInt(editVolume.text)
                                    if (isNaN(vol)) vol = m.volume
                                    
                                    console.log("Auto Modify Limit Price:", val)
                                    root.orderController.modifyConditionOrder(m.request_id, triggerP, val, vol)
                                    focus = false
                                }

                                // 微调按钮 
                                Column {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 1
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 20
                                    height: parent.height - 2
                                    spacing: 0
                                    visible: parent.activeFocus
                                    
                                    Rectangle {
                                        width: 20; height: parent.height/2
                                        color: upAreaL.containsMouse ? "#333333" : "transparent"
                                        Text { text: "▴"; color: "#cccccc"; anchors.centerIn: parent; font.pixelSize: 10 }
                                        MouseArea {
                                            id: upAreaL; anchors.fill: parent; hoverEnabled: true
                                            onClicked: {
                                                editLimitPrice.forceActiveFocus()
                                                var v = parseFloat(editLimitPrice.text) || 0
                                                var tick = root.orderController ? root.orderController.getInstrumentPriceTick(m.instrument_id) : 1.0
                                                v = (Math.round((v + tick)/tick) * tick)
                                                editLimitPrice.text = v.toFixed(2)
                                            }
                                        }
                                    }
                                    Rectangle {
                                        width: 20; height: parent.height/2
                                        color: downAreaL.containsMouse ? "#333333" : "transparent"
                                        Text { text: "▾"; color: "#cccccc"; anchors.centerIn: parent; font.pixelSize: 10 }
                                        MouseArea {
                                            id: downAreaL; anchors.fill: parent; hoverEnabled: true
                                            onClicked: {
                                                editLimitPrice.forceActiveFocus()
                                                var v = parseFloat(editLimitPrice.text) || 0
                                                var tick = root.orderController ? root.orderController.getInstrumentPriceTick(m.instrument_id) : 1.0
                                                v = (Math.round((v - tick)/tick) * tick)
                                                if (v < 0) v = 0
                                                editLimitPrice.text = v.toFixed(2)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        
                        // 状态
                        Text {
                            Layout.preferredWidth: 60
                            text: {
                                if (m.status === 0) return "待触发"
                                if (m.status === 1) return "已触发"
                                return "已取消"
                            }
                            color: {
                                if (m.status === 0) return "#2196f3"
                                if (m.status === 1) return "#4caf50"
                                return "#888888"
                            }
                            font.pixelSize: 13
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                            Layout.fillHeight: true
                        }
                        
                        // 操作按钮
                        Row {
                            spacing: 10
                            
                            Button {
                                width: 50
                                height: 24
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
                                    radius: 2
                                }
                                
                                onClicked: {
                                    if (root.orderController) {
                                        root.orderController.cancelConditionOrder(m.request_id)
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
