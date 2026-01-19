import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * 持仓面板组件
 * 功能：
 * - 显示当前持仓列表
 * - 显示持仓的盈亏、成本、现价等信息
 * - 点击持仓自动选中合约并订阅行情
 */
Item {
    id: root
    
    // 对外暴露的属性
    property var positionModel
    property var orderController
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // 标题栏
        Rectangle {
            Layout.fillWidth: true
            height: 30
            color: "#2d2d30"
            
            Text {
                text: "  💼 当前持仓"
                color: "#cccccc"
                font.pixelSize: 13
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        
        // 表头
        Rectangle {
            Layout.fillWidth: true
            height: 35
            color: "#1e1e1e"
            
            Row {
                anchors.fill: parent
                
                Text {
                    width: parent.width * 0.15
                    text: "合约"
                    color: "#aaaaaa"
                    horizontalAlignment: Text.AlignHCenter
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    width: parent.width * 0.1
                    text: "方向"
                    color: "#aaaaaa"
                    horizontalAlignment: Text.AlignHCenter
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    width: parent.width * 0.15
                    text: "数量"
                    color: "#aaaaaa"
                    horizontalAlignment: Text.AlignHCenter
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    width: parent.width * 0.25
                    text: "盈亏"
                    color: "#aaaaaa"
                    horizontalAlignment: Text.AlignHCenter
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    width: parent.width * 0.35
                    text: "成本/现价"
                    color: "#aaaaaa"
                    horizontalAlignment: Text.AlignHCenter
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
        
        // 持仓列表
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.positionModel
            clip: true
            
            delegate: Rectangle {
                width: parent.width
                height: 40
                
                // 选中态背景色逻辑
                color: {
                    if (orderController && orderController.instrumentId === model.instrumentId) {
                        return "#2c3e50" // 选中深蓝
                    }
                    return index % 2 === 0 ? "#1e1e1e" : "#252526"
                }
                
                // 选中指示条
                Rectangle {
                    width: 3
                    height: parent.height
                    color: "#569cd6"
                    visible: orderController && orderController.instrumentId === model.instrumentId
                }
                
                Row {
                    anchors.fill: parent
                    
                    Text {
                        width: parent.width * 0.15
                        height: 40
                        text: model.instrumentId
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        width: parent.width * 0.1
                        height: 40
                        text: model.direction
                        color: model.direction === "BUY" ? "#f44336" : "#4caf50"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        width: parent.width * 0.15
                        height: 40
                        text: model.position
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        width: parent.width * 0.25
                        height: 40
                        text: model.profit
                        color: parseFloat(model.profit) >= 0 ? "#f44336" : "#4caf50"
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        width: parent.width * 0.35
                        height: 40
                        text: model.cost.toFixed(2) + " / " + model.lastPrice.toFixed(2)
                        color: "#aaaaaa"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (orderController) {
                            orderController.instrumentId = model.instrumentId
                            // 点击持仓时，自动填入价格为最新价
                            orderController.price = model.lastPrice
                            
                            // 确保已订阅行情，否则价格不会动
                            orderController.subscribe(model.instrumentId)
                        }
                    }
                }
            }
        }
    }
}
