import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * 成交列表面板组件 (Trade List)
 * 显示此会话的所有成交记录
 */
Item {
    id: root
    
    property var tradeModel
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // 表头
        Rectangle {
            Layout.fillWidth: true
            height: 30
            color: "#1e1e1e"
            
            Row {
                anchors.fill: parent
                Text { width: parent.width * 0.12; text: "合约"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.08; text: "方向"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.08; text: "开平"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.15; text: "价格"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.10; text: "手数"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.12; text: "手续费"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.15; text: "平仓盈亏"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                Text { width: parent.width * 0.20; text: "成交时间"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
            }
        }
        
        // 列表
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: root.tradeModel
            clip: true
            
            delegate: Rectangle {
                width: parent.width
                height: 35
                color: index % 2 === 0 ? "#1e1e1e" : "#252526"
                
                function getDirText(d) { return d === "0" ? "买" : "卖"; }
                function getOffText(o) { return o === "0" ? "开" : "平"; } // 简略
                
                Row {
                    anchors.fill: parent
                    
                    Text { width: parent.width * 0.12; text: model.instrumentId; color: "white"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    Text { width: parent.width * 0.08; text: getDirText(model.direction); color: model.direction === "0" ? "#f44336" : "#4caf50"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    Text { width: parent.width * 0.08; text: getOffText(model.offsetFlag); color: "white"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    Text { width: parent.width * 0.15; text: model.price.toFixed(2); color: "white"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    Text { width: parent.width * 0.10; text: model.volume; color: "white"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    Text { width: parent.width * 0.12; text: model.commission.toFixed(2); color: "white"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    Text { 
                        width: parent.width * 0.15; 
                        text: model.closeProfit !== 0 ? model.closeProfit.toFixed(2) : "--"; 
                        color: model.closeProfit > 0 ? "#f44336" : (model.closeProfit < 0 ? "#4caf50" : "white"); 
                        horizontalAlignment: Text.AlignHCenter; 
                        anchors.verticalCenter: parent.verticalCenter 
                    }
                    Text { width: parent.width * 0.20; text: model.time; color: "#888888"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                }
            }
        }
    }
}
