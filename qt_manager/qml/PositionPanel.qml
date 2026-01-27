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
                
                Text { width: parent.width * 0.15; text: "合约"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.08; text: "方向"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.22; text: "总/昨/今"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.15; text: "持仓均价"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.15; text: "现价"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.25; text: "持仓盈亏"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
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
                        width: parent.width * 0.08
                        height: 40
                        text: model.direction
                        color: model.direction === "BUY" ? "#f44336" : "#4caf50"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        width: parent.width * 0.22
                        height: 40
                        text: model.position + " / " + model.ydPosition + " / " + model.todayPosition
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        width: parent.width * 0.15
                        height: 40
                        text: model.avgPrice
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        width: parent.width * 0.15
                        height: 40
                        text: model.lastPrice.toFixed(2)
                        color: "#aaaaaa"
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
                }
                
                MouseArea {
                    anchors.fill: parent
                    
                    // 防误触/重复提交标志
                    property bool processing: false
                    
                    Timer {
                        id: resetTimer
                        interval: 1000 // 1秒防抖
                        onTriggered: parent.processing = false
                    }

                    onClicked: {
                        if (orderController) {
                            orderController.instrumentId = model.instrumentId
                            // 点击持仓时，自动填入价格为最新价
                            orderController.price = model.lastPrice
                            
                            // 确保已订阅行情，否则价格不会动
                            orderController.subscribe(model.instrumentId)
                            // 自动填入本次持仓的手数
                            orderController.volume = model.position
                        }
                    }
                    onDoubleClicked: {
                        if (processing) return; // 防止连点
                        processing = true;
                        resetTimer.start();

                        if (orderController) {
                            orderController.instrumentId = model.instrumentId
                            orderController.price = model.lastPrice // 双击全平使用最新价
                            orderController.subscribe(model.instrumentId)
                            
                            // 1. 确定平仓方向 (持仓的反向)
                            var actionDir = (model.direction === "BUY") ? "SELL" : "BUY"
                            
                            // 2. 判定是否为上期所/能源中心合约 (需区分平今/平昨)
                            var id = model.instrumentId.toLowerCase()
                            var prefix = id.replace(/[0-9]+/, "")
                            var shfePrefixes = ["cu","al","zn","pb","ni","sn","au","ag","rb","wr","hc","fu","bu","ru","sp","sc","nr","lu","bc","br","ec"]
                            var isShfe = shfePrefixes.indexOf(prefix) !== -1
                            
                            console.log("[QuickClose] DoubleClick: " + model.instrumentId + " Dir:" + actionDir + " IsShfe:" + isShfe)
                            
                            if (isShfe) {
                                // 上期所优先平今
                                if (model.todayPosition > 0) {
                                    orderController.volume = model.todayPosition
                                    orderController.sendOrder(actionDir, "CLOSETODAY")
                                    console.log(" -> CloseToday Vol:" + model.todayPosition)
                                }
                                // 再平昨
                                if (model.ydPosition > 0) {
                                    orderController.volume = model.ydPosition
                                    orderController.sendOrder(actionDir, "CLOSE")
                                    console.log(" -> CloseYesterday Vol:" + model.ydPosition)
                                }
                            } else {
                                // 其他交易所直接平仓
                                orderController.volume = model.position
                                orderController.sendOrder(actionDir, "CLOSE")
                                console.log(" -> Close Vol:" + model.position)
                            }
                        }
                    }
                }
            }
        }
    }
}
