// qmllint disable import
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "."

/**
 * 成交列表面板组件 (Trade List)
 * 支持「明细」和「合计」两种显示模式
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
    property bool showSummary: false  // false=明细, true=合计
    
    // 合计数据缓存
    property var summaryData: []
    
    function refreshSummary() {
        if (showSummary && tradeModel) {
            summaryData = tradeModel.getTradeSummary()
        }
    }
    
    // 监听 summaryChanged 信号
    Connections {
        target: tradeModel
        function onSummaryChanged() {
            if (showSummary) refreshSummary()
        }
    }
    
    onShowSummaryChanged: refreshSummary()
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // Module Header
        Rectangle {
            Layout.fillWidth: true
            height: 30
            color: "#2d2d30"
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 0
                
                Text {
                    text: "✅ 成交记录"
                    color: "#cccccc"
                    font.pixelSize: 13
                    font.bold: true
                }
                
                Item { Layout.fillWidth: true }
                
                // 明细/合计 切换按钮
                Row {
                    spacing: 4
                    
                    Rectangle {
                        width: 50; height: 22
                        radius: 3
                        color: !showSummary ? "#2c5d87" : "#3a3a3d"
                        border.color: !showSummary ? "#5599cc" : "#555555"
                        border.width: 1
                        
                        Text {
                            anchors.centerIn: parent
                            text: "明细"
                            color: !showSummary ? "white" : "#aaaaaa"
                            font.pixelSize: 11
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: showSummary = false
                        }
                    }
                    
                    Rectangle {
                        width: 50; height: 22
                        radius: 3
                        color: showSummary ? "#2c5d87" : "#3a3a3d"
                        border.color: showSummary ? "#5599cc" : "#555555"
                        border.width: 1
                        
                        Text {
                            anchors.centerIn: parent
                            text: "合计"
                            color: showSummary ? "white" : "#aaaaaa"
                            font.pixelSize: 11
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: showSummary = true
                        }
                    }
                }
            }
        }

        // ================ 明细视图 ================
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !showSummary
            
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
                
                // 明细列表
                ListView {
                    id: tradeListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: root.tradeModel
                    clip: true
                    currentIndex: -1
                    ScrollBar.vertical: ScrollBar {}
                    
                    delegate: Rectangle {
                        id: tradeDelegate
                        width: tradeListView.width
                        height: 35
                        
                        color: {
                            if (mouseArea.containsMouse && ListView.isCurrentItem) return "#3a5a7a"
                            if (ListView.isCurrentItem) return "#2c5d87"
                            if (mouseArea.containsMouse) return "#2a2a2a"
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
                        function getOffText(o) { return o === "0" ? "开" : "平"; }
                        
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
                                tradeListView.currentIndex = index
                            }
                        }
                    }
                }
            }
        }

        // ================ 合计视图 ================
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: showSummary
            
            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                
                // 合计表头
                Rectangle {
                    Layout.fillWidth: true
                    height: 30
                    color: "#1e1e1e"
                    
                    Row {
                        anchors.fill: parent
                        Text { width: parent.width * 0.15; text: "合约"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                        Text { width: parent.width * 0.10; text: "方向"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                        Text { width: parent.width * 0.10; text: "开平"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                        Text { width: parent.width * 0.15; text: "成交均价"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                        Text { width: parent.width * 0.10; text: "成交手数"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                        Text { width: parent.width * 0.15; text: "手续费"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                        Text { width: parent.width * 0.25; text: "平仓盈亏"; color: "#aaaaaa"; horizontalAlignment: Text.AlignHCenter; anchors.verticalCenter: parent.verticalCenter }
                    }
                }
                
                // 合计列表
                ListView {
                    id: summaryListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: summaryData.length
                    clip: true
                    currentIndex: -1
                    ScrollBar.vertical: ScrollBar {}
                    
                    delegate: Rectangle {
                        required property int index
                        width: summaryListView.width
                        height: 35
                        color: index % 2 === 0 ? "#1e1e1e" : "#252526"
                        
                        property var rowData: summaryData[index] || ({})
                        
                        Row {
                            anchors.fill: parent
                            
                            Text { 
                                width: parent.width * 0.15
                                text: rowData.instrumentId || ""
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text { 
                                width: parent.width * 0.10
                                text: rowData.direction === "0" ? "买" : "卖"
                                color: rowData.direction === "0" ? "#f44336" : "#4caf50"
                                horizontalAlignment: Text.AlignHCenter
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text { 
                                width: parent.width * 0.10
                                text: rowData.offsetFlag === "0" ? "开仓" : "平仓"
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text { 
                                width: parent.width * 0.15
                                text: (rowData.avgPrice || 0).toFixed(2)
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text { 
                                width: parent.width * 0.10
                                text: rowData.totalVolume || 0
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text { 
                                width: parent.width * 0.15
                                text: (rowData.totalCommission || 0).toFixed(2)
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text { 
                                width: parent.width * 0.25
                                property double cp: rowData.totalCloseProfit || 0
                                text: cp !== 0 ? cp.toFixed(2) : "--"
                                color: cp > 0 ? "#f44336" : (cp < 0 ? "#4caf50" : "white")
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }
            }
        }
    }
}
