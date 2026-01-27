import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * 报单列表面板组件 (Order List)
 * 显示所有报单状态
 * 布局重构：对标专业交易终端
 */
Item {
    id: root
    
    // 对外暴露的属性
    property var orderModel
    property var orderController
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // 表头 (匹配专业终端布局)
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: "#1e1e1e"
            
            Row {
                anchors.fill: parent
                spacing: 0
                
                // 总宽度 100%
                Text { width: parent.width * 0.10; text: "报单编号"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                Text { width: parent.width * 0.08; text: "合约"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                Text { width: parent.width * 0.04; text: "买卖"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                Text { width: parent.width * 0.04; text: "开平"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                Text { width: parent.width * 0.08; text: "状态"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                Text { width: parent.width * 0.08; text: "价格"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                Text { width: parent.width * 0.06; text: "报单"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                Text { width: parent.width * 0.06; text: "未成"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                Text { width: parent.width * 0.06; text: "成交"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                Text { width: parent.width * 0.20; text: "详细状态"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                Text { width: parent.width * 0.10; text: "报单时间"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                Text { width: parent.width * 0.10; text: "最后成交"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
            }
        }
        
        // 列表
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.orderModel
            clip: true
            
            delegate: Rectangle {
                id: orderDelegate
                width: ListView.view.width
                height: 35
                
                // 状态颜色逻辑
                color: {
                    if (ListView.isCurrentItem) return "#2c5d87"
                    if (mouseArea.containsMouse) return "#3a3a3a"
                    return index % 2 === 0 ? "#1e1e1e" : "#252526"
                }

                required property int index
                required property var model
                
                // 辅助函数
                function getStatusColor(status) {
                    if (status === "0") return "#4caf50"; // 全部成交(绿)
                    if (status === "5") return "#9e9e9e"; // 撤单(灰)
                    if (status === "3") return "#ffeb3b"; // 未成交(黄)
                    if (status === "1") return "#8bc34a"; // 部分成交
                    return "#ffffff";
                }
                
                function getStatusText(status) {
                    if (status === "0") return "全部成交";
                    if (status === "1") return "部分成交";
                    if (status === "3") return "未成交";
                    if (status === "5") return "已撤单";
                    if (status === "a") return "未知";
                    return status;
                }
                
                function getDirText(d) { return d === "0" ? "买" : "卖"; }
                
                function getOffsetFlagText(flag) {
                    if (flag === "0") return "开仓";
                    if (flag === "1") return "平仓";
                    if (flag === "3") return "平今";
                    if (flag === "4") return "平昨";
                    return flag;
                }
                
                Row {
                    anchors.fill: parent
                    
                    // 报单编号 (Consolas)
                    Text { width: parent.width * 0.10; text: orderDelegate.model.orderSysId || "-"; color: "#cccccc"; font.family: "Consolas"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    // 合约
                    Text { width: parent.width * 0.08; text: orderDelegate.model.instrumentId; color: "white"; font.bold: true; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    // 买卖
                    Text { width: parent.width * 0.04; text: getDirText(orderDelegate.model.direction); color: orderDelegate.model.direction === "0" ? "#f44336" : "#4caf50"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    // 开平
                    Text { 
                        width: parent.width * 0.04; 
                        text: getOffsetFlagText(orderDelegate.model.offsetFlag)
                        color: "white"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter 
                    }
                    // 状态
                    Text { width: parent.width * 0.08; text: getStatusText(orderDelegate.model.status); color: getStatusColor(orderDelegate.model.status); horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    // 价格
                    Text { width: parent.width * 0.08; text: orderDelegate.model.price.toFixed(2); color: "white"; font.family: "Consolas"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    // 报单 (报单手数)
                    Text { width: parent.width * 0.06; text: orderDelegate.model.volumeOriginal; color: "white"; font.family: "Consolas"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    // 未成 (未成交手数 = 剩余)
                    Text { width: parent.width * 0.06; text: orderDelegate.model.volumeTotal; color: "white"; font.family: "Consolas"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    // 成交 (成交手数)
                    // 如果部分成交，绿色显示
                    Text { width: parent.width * 0.06; text: orderDelegate.model.volumeTraded; color: orderDelegate.model.volumeTraded > 0 ? "#4caf50" : "#888888"; font.family: "Consolas"; font.bold: orderDelegate.model.volumeTraded > 0; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    // 详细状态
                    Text { width: parent.width * 0.20; text: orderDelegate.model.statusMsg; color: "#aaaaaa"; font.pixelSize: 11; elide: Text.ElideRight; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    // 报单时间
                    Text { width: parent.width * 0.10; text: orderDelegate.model.time; color: "#cccccc"; font.family: "Consolas"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    // 最后成交 (暂无)
                    Text { width: parent.width * 0.10; text: "-"; color: "#666666"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true 
                    
                    onClicked: {
                        ListView.view.currentIndex = orderDelegate.index
                        if(root.orderController) {
                            root.orderController.instrumentId = orderDelegate.model.instrumentId
                        }
                    }
                    onDoubleClicked: {
                         // 双击撤单逻辑
                         var s = orderDelegate.model.status;
                         if (s !== "0" && s !== "5") { 
                             if (root.orderController) {
                                 root.orderController.cancelOrder(
                                     orderDelegate.model.instrumentId,
                                     orderDelegate.model.orderSysId || "",
                                     orderDelegate.model.orderRef || "",
                                     orderDelegate.model.exchangeId || "",
                                     orderDelegate.model.frontId || 0,
                                     orderDelegate.model.sessionId || 0
                                 );
                             }
                         }
                    }
                }
            }
        }
    }
}
