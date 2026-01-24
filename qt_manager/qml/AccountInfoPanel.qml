import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

pragma ComponentBehavior: Bound

/**
 * 账户信息面板组件
 * 功能：
 * - 显示账户状态
 * - 显示动态权益、可用资金、持仓盈亏、占用保证金
 * - 显示 CTP 连接状态
 */
Rectangle {
    id: accountRoot
    
    // 对外暴露的属性
    property var accountInfo
    property bool ctpConnected
    property string accountId
    
    height: 60
    color: "#252526"
    
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        spacing: 30
        
        // 账户状态
        Column {
            Text {
                text: "账户状态"
                color: "#aaaaaa"
                font.pixelSize: 11
            }
            Text {
                text: accountRoot.accountId
                color: "white"
                font.pixelSize: 16
                font.bold: true
            }
        }
        
        // 资金信息区域
        RowLayout {
            spacing: 35
            
            // 动态权益
            Column {
                Text {
                    text: "动态权益"
                    color: "#aaaaaa"
                    font.pixelSize: 11
                }
                Text {
                    text: accountInfo ? accountInfo.equity.toFixed(2) : "0.00"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    font.family: "Consolas"
                }
            }
            
            // 可用资金
            Column {
                Text {
                    text: "可用资金"
                    color: "#aaaaaa"
                    font.pixelSize: 11
                }
                Text {
                    text: accountInfo ? accountInfo.available.toFixed(2) : "0.00"
                    color: "#4ec9b0"
                    font.pixelSize: 16
                    font.bold: true
                    font.family: "Consolas"
                }
            }
            
            // 持仓盈亏
            Column {
                Text {
                    text: "持仓盈亏"
                    color: "#aaaaaa"
                    font.pixelSize: 11
                }
                Text {
                    text: {
                        if (!accountInfo) return "0.00"
                        var profit = accountInfo.floatingProfit
                        return (profit >= 0 ? "+" : "") + profit.toFixed(2)
                    }
                    color: {
                        if (!accountInfo) return "white"
                        return accountInfo.floatingProfit >= 0 ? "#f44336" : "#4caf50"
                    }
                    font.pixelSize: 16
                    font.bold: true
                    font.family: "Consolas"
                }
            }
            
            // 占用保证金
            Column {
                Text {
                    text: "占用保证金"
                    color: "#aaaaaa"
                    font.pixelSize: 11
                }
                Text {
                    text: accountInfo ? accountInfo.margin.toFixed(2) : "0.00"
                    color: "#ce9178"
                    font.pixelSize: 16
                    font.bold: true
                    font.family: "Consolas"
                }
            }
        }
        
        
        Item { Layout.fillWidth: true }
    }
}
