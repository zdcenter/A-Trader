import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * 成交列表面板组件 (Trade List)
 * 显示此会话的所有成交记录
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
    
    property var tradeModel
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // Module Header
        Rectangle {
            Layout.fillWidth: true
            height: 30
            color: "#2d2d30"
            
            Text {
                text: "  ✅ 成交记录"
                color: "#cccccc"
                font.pixelSize: 13
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
        }

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
            ScrollBar.vertical: ScrollBar {}
            
            delegate: Rectangle {
                id: tradeDelegate
                width: ListView.view.width
                height: 35
                
                // 选中和悬停样式
                color: {
                    if (ListView.isCurrentItem) return "#2c5d87"
                    if (mouseArea.containsMouse) return "#3a3a3a"
                    return index % 2 === 0 ? "#1e1e1e" : "#252526"
                }
                required property int index
                
                required property string instrumentId
                required property string direction
                required property string offsetFlag
                required property double price
                required property int volume
                required property double commission
                required property double closeProfit
                required property string time
                
                function getDirText(d) { return d === "0" ? "买" : "卖"; }
                function getOffText(o) { return o === "0" ? "开" : "平"; } // 简略
                
                Row {
                    anchors.fill: parent
                    
                    Text { width: parent.width * 0.12; text: instrumentId; color: "white"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    Text { width: parent.width * 0.08; text: getDirText(direction); color: direction === "0" ? "#f44336" : "#4caf50"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    Text { width: parent.width * 0.08; text: getOffText(offsetFlag); color: "white"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    Text { width: parent.width * 0.15; text: price.toFixed(2); color: "white"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    Text { width: parent.width * 0.10; text: volume; color: "white"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    Text { width: parent.width * 0.12; text: commission.toFixed(2); color: "white"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    Text { 
                        width: parent.width * 0.15; 
                        text: closeProfit !== 0 ? closeProfit.toFixed(2) : "--"; 
                        color: closeProfit > 0 ? "#f44336" : (closeProfit < 0 ? "#4caf50" : "white"); 
                        horizontalAlignment: Text.AlignHCenter; 
                        anchors.verticalCenter: parent.verticalCenter 
                    }
                    Text { width: parent.width * 0.20; text: time; color: "#888888"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        ListView.view.currentIndex = index
                    }
                }
            }
        }
    }
}
