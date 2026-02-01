// qmllint disable import
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

/**
 * 报单列表面板组件 (Order List)
 * 显示所有报单状态
 * 布局重构：对标专业交易终端
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
    property var orderModel
    property var orderController
    
    // Module title (optional if used as standalone)
    property string title: "委托记录"
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // Module Header
        Rectangle {
            Layout.fillWidth: true
            height: 30
            color: "#2d2d30"
            
            Text {
                text: "  📓 委托记录"
                color: "#cccccc"
                font.pixelSize: 13
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        
        // 表头 (匹配专业终端布局)
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: "#1e1e1e"
            
            Row {
                anchors.fill: parent
                spacing: 0
                
                // 1. 报单编号 (10%)
                Text { width: parent.width * 0.10; text: "报单编号"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                // 2. 合约 (10%)
                Text { width: parent.width * 0.10; text: "合约"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                // 3. 买卖 (5%)
                Text { width: parent.width * 0.05; text: "买卖"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                // 4. 开平 (5%)
                Text { width: parent.width * 0.05; text: "开平"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                // 5. 状态 (8%)
                Text { width: parent.width * 0.08; text: "状态"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                // 6. 价格 (10%)
                Text { width: parent.width * 0.10; text: "价格"; color: "#aaaaaa"; horizontalAlignment: Text.AlignRight; rightPadding: 10; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                // 7. 报单 (6%)
                Text { width: parent.width * 0.06; text: "报单"; color: "#aaaaaa"; horizontalAlignment: Text.AlignRight; rightPadding: 10; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                // 8. 未成 (6%)
                Text { width: parent.width * 0.06; text: "未成"; color: "#aaaaaa"; horizontalAlignment: Text.AlignRight; rightPadding: 10; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                // 9. 成交 (6%)
                Text { width: parent.width * 0.06; text: "成交"; color: "#aaaaaa"; horizontalAlignment: Text.AlignRight; rightPadding: 10; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                // 10. 详细状态 (22%)
                Text { width: parent.width * 0.22; text: "详细状态"; color: "#aaaaaa"; horizontalAlignment: Text.AlignLeft; leftPadding: 10; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
                // 11. 报单时间 (12%)
                Text { width: parent.width * 0.12; text: "报单时间"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter; font.pixelSize: 12 }
            }
        }
        
        // 列表
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.orderModel
            clip: true
            ScrollBar.vertical: ScrollBar {}
            
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
                
                // Explicitly define required properties for roles to fix scope issues
                required property string instrumentId
                required property string orderSysId
                required property string direction
                required property string offsetFlag
                required property string status
                required property double price
                required property int volumeOriginal
                required property int volumeTotal
                required property int volumeTraded
                required property string statusMsg
                required property string time
                required property string orderRef
                required property string exchangeId
                required property int frontId
                required property int sessionId
                
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
                    
                    // 1. 报单编号 (10%) - 优先显示 orderSysId，为空则显示 orderRef
                    Text { 
                        width: parent.width * 0.10; 
                        text: orderSysId || ("#" + orderRef); 
                        color: orderSysId ? "#cccccc" : "#888888"; 
                        font.family: "Consolas"; 
                        horizontalAlignment: Text.AlignHCenter; 
                        anchors.verticalCenter: parent.verticalCenter; 
                        elide: Text.ElideRight 
                    }
                    // 2. 合约 (10%)
                    Text { width: parent.width * 0.10; text: instrumentId; color: "#4ec9b0"; font.bold: true; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    // 3. 买卖 (5%)
                    Text { width: parent.width * 0.05; text: getDirText(direction); color: direction === "0" ? "#f44336" : "#4caf50"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    // 4. 开平 (5%)
                    Text { width: parent.width * 0.05; text: getOffsetFlagText(offsetFlag); color: "white"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    // 6. 价格 (10%)
                    Text { width: parent.width * 0.10; text: price.toFixed(2); color: "white"; font.family: "Consolas"; horizontalAlignment: Text.AlignRight; rightPadding: 10; anchors.verticalCenter: parent.verticalCenter }
                    // 7. 报单 (6%)
                    Text { width: parent.width * 0.06; text: volumeOriginal; color: "white"; font.family: "Consolas"; horizontalAlignment: Text.AlignRight; rightPadding: 10; anchors.verticalCenter: parent.verticalCenter }
                    // 8. 未成 (6%)
                    Text { width: parent.width * 0.06; text: volumeTotal; color: "white"; font.family: "Consolas"; horizontalAlignment: Text.AlignRight; rightPadding: 10; anchors.verticalCenter: parent.verticalCenter }
                    // 9. 成交 (6%)
                    Text { width: parent.width * 0.06; text: volumeTraded; color: volumeTraded > 0 ? "#4caf50" : "#888888"; font.family: "Consolas"; font.bold: volumeTraded > 0; horizontalAlignment: Text.AlignRight; rightPadding: 10; anchors.verticalCenter: parent.verticalCenter }
                    // 5. 状态 (8%)
                    Text { width: parent.width * 0.08; text: getStatusText(status); color: getStatusColor(status); horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    // 10. 详细状态 (22%)
                    Text { width: parent.width * 0.22; text: statusMsg; color: "#aaaaaa"; font.pixelSize: 11; elide: Text.ElideRight; horizontalAlignment: Text.AlignLeft; leftPadding: 10; anchors.verticalCenter: parent.verticalCenter }
                    // 11. 报单时间 (12%)
                    Text { width: parent.width * 0.12; text: time; color: "#cccccc"; font.family: "Consolas"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true 
                    
                    onClicked: {
                        ListView.view.currentIndex = index
                        if(root.orderController) {
                            root.orderController.instrumentId = instrumentId
                        }
                    }
                    onDoubleClicked: {
                         // 双击撤单逻辑
                         var s = status;
                         if (s !== "0" && s !== "5") { 
                             if (root.orderController) {
                                 root.orderController.cancelOrder(
                                     instrumentId,
                                     orderSysId || "",
                                     orderRef || "",
                                     exchangeId || "",
                                     frontId || 0,
                                     sessionId || 0
                                 );
                             }
                         }
                    }
                }
            }
        }
    }
}
