import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * 报单列表面板组件 (Order List)
 * 显示所有报单状态
 */
Item {
    id: root
    
    // 对外暴露的属性
    property var orderModel
    property var orderController
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // 表头
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: "#1e1e1e"
            
            Row {
                anchors.fill: parent
                
                Text { width: parent.width * 0.15; text: "合约"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.10; text: "方向"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.10; text: "开平"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.15; text: "价格"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.15; text: "状态"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.15; text: "报/成/撤"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.20; text: "报单编号"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
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
                color: index % 2 === 0 ? "#1e1e1e" : "#252526"
                
                // Qt 6 推荐：显式声明 Delegate 属性
                required property int index
                required property var model
                
                // 辅助函数：状态颜色
                function getStatusColor(status) {
                    if (status === "0") return "#4caf50"; // 全部成交
                    if (status === "5") return "#9e9e9e"; // 已撤单
                    if (status === "3") return "#ffeb3b"; // 未成交
                    if (status === "1") return "#8bc34a"; // 部分成交
                    return "#ffffff";
                }
                
                // 辅助函数：状态文字
                function getStatusText(status) {
                    if (status === "0") return "全部成交";
                    if (status === "1") return "部分成交";
                    if (status === "3") return "未成交";
                    if (status === "5") return "已撤单";
                    if (status === "a") return "未知";
                    return status;
                }
                
                // 辅助函数：方向文字
                function getDirText(d) {
                    return d === "0" ? "买" : "卖";
                }
                
                // 辅助函数：开平文字
                function getOffsetFlagText(flag) {
                    if (flag === "0") return "开仓";
                    if (flag === "1") return "平仓";
                    if (flag === "3") return "平今";
                    if (flag === "4") return "平昨";
                    return flag;
                }
                
                Row {
                    anchors.fill: parent
                    
                    Text { width: parent.width * 0.15; text: orderDelegate.model.instrumentId; color: "white"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    Text { width: parent.width * 0.10; text: getDirText(orderDelegate.model.direction); color: orderDelegate.model.direction === "0" ? "#f44336" : "#4caf50"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    Text { width: parent.width * 0.10; text: getOffsetFlagText(orderDelegate.model.offsetFlag); color: "white"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    Text { width: parent.width * 0.15; text: orderDelegate.model.price.toFixed(2); color: "white"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    Text { width: parent.width * 0.15; text: getStatusText(orderDelegate.model.status); color: getStatusColor(orderDelegate.model.status); horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    Text { width: parent.width * 0.15; text: orderDelegate.model.volumeOriginal + "/" + orderDelegate.model.volumeTraded + "/" + (orderDelegate.model.volumeOriginal - orderDelegate.model.volumeTraded - (orderDelegate.model.volumeTotal || 0)); color: "white"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    Text { width: parent.width * 0.20; text: orderDelegate.model.orderSysId; color: "#888888"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                }

                // 点击委托单切换合约，双击撤单
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if(root.orderController) {
                            root.orderController.instrumentId = orderDelegate.model.instrumentId
                            root.orderController.subscribe(orderDelegate.model.instrumentId)
                        }
                    }
                }
            }
        }
    }
}
