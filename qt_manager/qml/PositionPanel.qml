// qmllint disable import
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

/**
 * 持仓面板组件
 * 功能：
 * - 显示当前持仓列表（含浮动盈亏、已实现盈亏、持仓均价）
 * - 双击快速平仓
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
    
    // 点击任意位置获取焦点(穿透)
    MouseArea {
        anchors.fill: parent
        z: 99
        propagateComposedEvents: true
        onPressed: (mouse)=> {
            root.forceActiveFocus()
            mouse.accepted = false
        }
    }
    
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
                text: "  💼 持仓记录"
                color: "#cccccc"
                font.pixelSize: 13
                font.bold: true
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
                
                Text { width: parent.width * 0.07; text: "交易所"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.12; text: "合约"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.06; text: "方向"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.14; text: "总/昨/今"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.12; text: "持仓均价"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.12; text: "现价"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.12; text: "浮动盈亏"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.12; text: "平仓盈亏"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.13; text: "保证金"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
            }
        }
        
        // 持仓列表
        ListView {
            id: positionListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.positionModel
            clip: true
            currentIndex: -1
            ScrollBar.vertical: ScrollBar {}
            
            delegate: Rectangle {
                id: positionDelegate
                width: positionListView.width
                height: 40
                
                required property int index
                required property string exchangeId
                required property string instrumentId
                required property string direction
                required property int position
                required property int ydPosition
                required property int todayPosition
                required property string avgPrice
                required property double lastPrice
                required property double bidPrice1
                required property double askPrice1
                required property double priceTick
                required property double upperLimit
                required property double lowerLimit
                required property string posProfit
                required property string closeProfit
                required property string margin
                
                // 统一的选中和悬停样式
                color: {
                    if (positionMouseArea.containsMouse && positionListView.currentIndex === index) return "#3a5a7a"
                    if (positionListView.currentIndex === index) return "#2c5d87"
                    if (positionMouseArea.containsMouse) return "#2a2a2a"
                    return index % 2 === 0 ? "#1e1e1e" : "#252526"
                }
                
                Row {
                    anchors.fill: parent
                    
                    Text {
                        width: parent.width * 0.07
                        height: 40
                        text: exchangeId
                        color: "#aaaaaa"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        width: parent.width * 0.12
                        height: 40
                        text: instrumentId
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        width: parent.width * 0.06
                        height: 40
                        text: direction
                        color: direction === "BUY" ? "#f44336" : "#4caf50"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        width: parent.width * 0.14
                        height: 40
                        text: position + " / " + ydPosition + " / " + todayPosition
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        width: parent.width * 0.12
                        height: 40
                        text: avgPrice
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        width: parent.width * 0.12
                        height: 40
                        text: lastPrice.toFixed(2)
                        color: "#aaaaaa"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        width: parent.width * 0.12
                        height: 40
                        text: posProfit
                        color: parseFloat(posProfit) >= 0 ? "#f44336" : "#4caf50"
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        width: parent.width * 0.12
                        height: 40
                        text: closeProfit
                        color: parseFloat(closeProfit) >= 0 ? "#f44336" : "#4caf50"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        width: parent.width * 0.13
                        height: 40
                        text: margin
                        color: "#aaaaaa"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                
                MouseArea {
                    id: positionMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    
                    // 防误触/重复提交标志
                    property bool processing: false
                    
                    Timer {
                        id: resetTimer
                        interval: 1000 // 1秒防抖
                        onTriggered: parent.processing = false
                    }

                    onClicked: {
                        positionListView.currentIndex = index
                        if (orderController) {
                            orderController.instrumentId = instrumentId
                            orderController.price = lastPrice
                            orderController.volume = position
                        }
                    }
                    onDoubleClicked: {
                        if (processing) return;
                        processing = true;
                        resetTimer.start();

                        if (orderController) {
                            orderController.instrumentId = instrumentId
                            
                            var actionDir = (direction === "BUY") ? "SELL" : "BUY"
                            var finalPrice = lastPrice
                            
                            // 对手价 ± 5 跳 (激进限价)
                            var ticks = 5
                            var tickSize = priceTick > 0 ? priceTick : 1.0
                            
                            if (actionDir === "SELL") { // 平多
                                if (bidPrice1 > 0) {
                                     finalPrice = bidPrice1 - (ticks * tickSize)
                                } else {
                                     finalPrice = lastPrice * 0.98
                                }
                                if (finalPrice < 0) finalPrice = 0.0
                            } else { // 平空
                                if (askPrice1 > 0) {
                                     finalPrice = askPrice1 + (ticks * tickSize)
                                } else {
                                     finalPrice = lastPrice * 1.02
                                }
                            }

                            // 取整到最小变动价位
                            if (priceTick > 0) {
                                var ratio = Math.round(finalPrice / priceTick)
                                finalPrice = ratio * priceTick
                            }
                            
                            // 涨跌停限制
                            if (upperLimit > 0 && finalPrice > upperLimit) finalPrice = upperLimit
                            if (lowerLimit > 0 && finalPrice < lowerLimit) finalPrice = lowerLimit

                            if (finalPrice <= 0.0001) finalPrice = 0.0

                            orderController.price = finalPrice
                            
                            var isShfe = false
                            if (exchangeId && exchangeId !== "") {
                                isShfe = (exchangeId === "SHFE" || exchangeId === "INE")
                            } else {
                                var id = instrumentId.toLowerCase()
                                var prefix = id.replace(/[0-9]+/, "")
                                var shfePrefixes = ["cu","al","zn","pb","ni","sn","au","ag","rb","wr","hc","fu","bu","ru","sp","sc","nr","lu","bc","br","ec"]
                                isShfe = shfePrefixes.indexOf(prefix) !== -1
                            }
                            
                            console.log("[QuickClose] " + instrumentId + " Dir:" + actionDir + " Price:" + finalPrice + " (Bid:" + bidPrice1 + " Ask:" + askPrice1 + ")" + " Limits:" + lowerLimit + "/" + upperLimit)
                            
                            if (isShfe) {
                                if (todayPosition > 0) {
                                    orderController.volume = todayPosition
                                    orderController.sendOrder(actionDir, "CLOSETODAY", "MARKET")
                                }
                                if (ydPosition > 0) {
                                    orderController.volume = ydPosition
                                    orderController.sendOrder(actionDir, "CLOSE", "MARKET")
                                }
                            } else {
                                orderController.volume = position
                                orderController.sendOrder(actionDir, "CLOSE", "MARKET")
                            }
                        }
                    }
                }
            }
        }
    }
}
