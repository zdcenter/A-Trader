import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.settings 1.0

/**
 * 增强版实时行情面板
 * 功能：
 * - 动态列配置（显示/隐藏）
 * - 列拖拽排序
 * - 丰富的数据展示（涨跌、买卖价等）
 * - 配置持久化（列顺序、可见性、订阅列表）
 */
Item {
    id: root
    
    // 对外暴露的属性
    property var marketModel
    property var orderController
    property var zmqWorker
    
    // 多选集合 {id: true}
    property var selectedSet: ({})
    
    // Settings 持久化配置
    Settings {
        id: marketSettings
        category: "MarketQuotePanel"
        
        // 保存列配置（JSON 字符串）
        property string columnConfig: ""
        // 保存订阅的合约列表
        property string subscribedInstruments: ""
    }
    
    // 列配置模型：定义所有可用列
    ListModel {
        id: columnModel
        
        Component.onCompleted: {
            // 尝试从 Settings 恢复列配置
            if (marketSettings.columnConfig) {
                try {
                    var savedConfig = JSON.parse(marketSettings.columnConfig)
                    clear()
                    for (var i = 0; i < savedConfig.length; i++) {
                        append(savedConfig[i])
                    }
                    console.log("✅ 已恢复列配置")
                } catch (e) {
                    console.log("⚠️ 恢复列配置失败，使用默认配置:", e)
                    loadDefaultColumns()
                }
            } else {
                console.log("📋 首次启动，使用默认列配置")
                loadDefaultColumns()
            }
        }
    }
    
    // 加载默认列配置
    function loadDefaultColumns() {
        columnModel.clear()
        columnModel.append({ role: "instrumentId", title: "合约", width: 80, visible: true, resizable: false })
        columnModel.append({ role: "lastPrice", title: "最新价", width: 80, visible: true, resizable: true })
        columnModel.append({ role: "change", title: "涨跌", width: 70, visible: true, resizable: true })
        columnModel.append({ role: "changePercent", title: "幅度%", width: 70, visible: true, resizable: true })
        columnModel.append({ role: "volume", title: "成交量", width: 80, visible: true, resizable: true })
        columnModel.append({ role: "openInterest", title: "持仓量", width: 80, visible: false, resizable: true })
        columnModel.append({ role: "bidPrice1", title: "买一价", width: 80, visible: true, resizable: true })
        columnModel.append({ role: "bidVolume1", title: "买一量", width: 60, visible: false, resizable: true })
        columnModel.append({ role: "askPrice1", title: "卖一价", width: 80, visible: true, resizable: true })
        columnModel.append({ role: "askVolume1", title: "卖一量", width: 60, visible: false, resizable: true })
        columnModel.append({ role: "preClose", title: "昨收", width: 80, visible: false, resizable: true })
        columnModel.append({ role: "turnover", title: "成交额", width: 90, visible: false, resizable: true })
        columnModel.append({ role: "upperLimit", title: "涨停价", width: 80, visible: false, resizable: true })
        columnModel.append({ role: "lowerLimit", title: "跌停价", width: 80, visible: false, resizable: true })
        columnModel.append({ role: "openPrice", title: "开盘", width: 70, visible: false, resizable: true })
        columnModel.append({ role: "highestPrice", title: "最高", width: 70, visible: false, resizable: true })
        columnModel.append({ role: "lowestPrice", title: "最低", width: 70, visible: false, resizable: true })
        columnModel.append({ role: "averagePrice", title: "均价", width: 70, visible: false, resizable: true })
        columnModel.append({ role: "updateTime", title: "时间", width: 90, visible: false, resizable: true })
    }
    
    // 保存列配置到 Settings
    function saveColumnConfig() {
        var config = []
        for (var i = 0; i < columnModel.count; i++) {
            var item = columnModel.get(i)
            config.push({
                role: item.role,
                title: item.title,
                width: item.width,
                visible: item.visible,
                resizable: item.resizable
            })
        }
        marketSettings.columnConfig = JSON.stringify(config)
        console.log("💾 已保存列配置")
    }
    
    // 保存订阅列表
    function saveSubscribedInstruments() {
        if (!marketModel) return
        
        var instruments = marketModel.getAllInstruments()
        marketSettings.subscribedInstruments = JSON.stringify(instruments)
        console.log("💾 已保存订阅列表:", instruments.length, "个合约")
    }
    
    // 恢复订阅列表
    function restoreSubscribedInstruments() {
        if (!marketSettings.subscribedInstruments || !orderController) return
        
        try {
            var instruments = JSON.parse(marketSettings.subscribedInstruments)
            if (instruments.length === 0) {
                console.log("📭 无已保存的订阅")
                return
            }
            
            console.log("🔄 正在恢复订阅:", instruments.length, "个合约")
            
            for (var i = 0; i < instruments.length; i++) {
                var id = instruments[i];
                if (marketModel) {
                     marketModel.addInstrument(id);
                }
                
                var subscribeCmd = JSON.stringify({
                    "type": "SUBSCRIBE",
                    "id": id
                })
                orderController.sendCommand(subscribeCmd)
            }
            console.log("✅ 订阅恢复完成")
        } catch (e) {
            console.log("⚠️ 恢复订阅列表失败:", e)
        }
    }
    
    // 计算列的实际显示宽度（自动填充可用空间）
    property real availableWidth: 0  // 可用宽度，由外部设置
    
    function getColumnWidth(index) {
        if (!columnModel.get(index).visible) return 0
        
        // 如果还没有可用宽度，使用默认宽度
        if (availableWidth <= 0) {
            return columnModel.get(index).width
        }
        
        // 计算可见列的数量
        var visibleCount = 0
        for (var i = 0; i < columnModel.count; i++) {
            if (columnModel.get(i).visible) visibleCount++
        }
        
        if (visibleCount === 0) return 0
        
        // 平均分配可用宽度
        return Math.max(60, availableWidth / visibleCount)  // 最小宽度 60px
    }
    
    // 组件加载完成后恢复订阅
    Component.onCompleted: {
        // 延迟恢复订阅，确保 zmqWorker 已就绪
        restoreTimer.start()
    }
    
    // 组件销毁前保存配置
    Component.onDestruction: {
        saveColumnConfig()
        saveSubscribedInstruments()
    }
    
    Timer {
        id: restoreTimer
        interval: 1000
        repeat: false
        onTriggered: restoreSubscribedInstruments()
    }

    // 辅助函数：根据 role 获取当前行的值
    function getCellValue(role, model) {
        switch(role) {
            case "instrumentId": return model.instrumentId;
            case "lastPrice": return model.lastPrice.toFixed(2);
            case "change": return (model.change > 0 ? "+" : "") + model.change.toFixed(2);
            case "changePercent": return (model.changePercent > 0 ? "+" : "") + model.changePercent.toFixed(2) + "%";
            case "volume": return model.volume;
            case "openInterest": return model.openInterest;
            case "bidPrice1": return model.bidPrice1.toFixed(2);
            case "bidVolume1": return model.bidVolume1;
            case "askPrice1": return model.askPrice1.toFixed(2);
            case "askVolume1": return model.askVolume1;
            case "preClose": return model.preClose.toFixed(2);
            case "turnover": return (model.turnover / 10000).toFixed(2) + "万";
            case "upperLimit": return model.upperLimit.toFixed(2);
            case "lowerLimit": return model.lowerLimit.toFixed(2);
            case "openPrice": return model.openPrice.toFixed(2);
            case "highestPrice": return model.highestPrice.toFixed(2);
            case "lowestPrice": return model.lowestPrice.toFixed(2);
            case "averagePrice": return model.averagePrice.toFixed(2);
            case "updateTime": return model.updateTime;
            default: return "";
        }
    }

    // 辅助函数：获取文本颜色
    function getCellColor(role, model) {
        if (role === "instrumentId") return "#569cd6";
        if (role === "volume" || role === "openInterest" || role === "bidVolume1" || role === "askVolume1" || role === "updateTime") return "#cccccc";
        
        // 价格相关字段
        var val = 0;
        if (role === "change" || role === "changePercent") {
            val = model.change;
        } else if (role === "lastPrice" || role === "bidPrice1" || role === "askPrice1") {
            // 对比昨收或其他逻辑，这里简单用涨跌额判断
            if (model.change > 0) return "#f44336";
            if (model.change < 0) return "#4caf50";
            return "white";
        }
        
        if (role === "upperLimit") return "#f44336";
        if (role === "lowerLimit") return "#4caf50";
        
        // 其他价格字段：与昨收/昨结比较
        if (role === "openPrice" || role === "highestPrice" || role === "lowestPrice" || role === "averagePrice") {
             var p = 0;
             if (role === "openPrice") p = model.openPrice;
             if (role === "highestPrice") p = model.highestPrice;
             if (role === "lowestPrice") p = model.lowestPrice;
             if (role === "averagePrice") p = model.averagePrice;
             
             if (p > model.preClose && model.preClose > 0.1) return "#f44336";
             if (p < model.preClose && model.preClose > 0.1) return "#4caf50";
             return "white";
        }
        
        if (role === "change" || role === "changePercent") {
            if (val > 0) return "#f44336"; // 红涨
            if (val < 0) return "#4caf50"; // 绿跌
        }
        
        return "#cccccc";
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // 顶部操作栏
        Rectangle { 
            Layout.fillWidth: true
            height: 35
            color: "#2d2d30"
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                
                Text { 
                    text: "📊 实时行情"
                    color: "#cccccc"
                    font.pixelSize: 13
                }
                
                Item { Layout.fillWidth: true }
                
                TextField {
                    id: subInput
                    placeholderText: "代码..."
                    font.pixelSize: 12
                    color: "white"
                    background: Rectangle {
                        color: "#1e1e1e"
                        radius: 4
                        border.color: "#333333"
                    }
                    Layout.preferredWidth: 100
                    Layout.preferredHeight: 26
                    
                    onAccepted: {
                        if (text.trim() !== "") {
                            var id = text.trim()
                            
                            // 1. 直接通过 Controller 发送订阅指令
                            if (orderController) {
                                var cmd = JSON.stringify({
                                    "type": "SUBSCRIBE",
                                    "id": id
                                })
                                console.log("📡 发送订阅指令:", cmd)
                                orderController.sendCommand(cmd)
                            } else {
                                console.error("❌ orderController 未连接，无法订阅")
                            }
                            
                            // 2. 立即在界面上添加占位行
                            if (marketModel) {
                                marketModel.addInstrument(id)
                            }
                            text = ""
                        }
                    }
                }
                
                // 列配置按钮
                Button {
                    id: configButton
                    text: "⚙️"
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 26
                    background: Rectangle {
                        color: parent.hovered ? "#3e3e42" : "#333333"
                        radius: 4
                    }
                    contentItem: Text { 
                        text: parent.text; color: "#cccccc"; 
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter 
                    }
                    onClicked: {
                        // 在按钮右下方弹出菜单
                        columnConfigMenu.x = configButton.x + configButton.width - columnConfigMenu.width
                        columnConfigMenu.y = configButton.y + configButton.height + 5
                        columnConfigMenu.open()
                    }
                }
            }
        }
        
        // 可拖拽表头
        Rectangle {
            Layout.fillWidth: true
            height: 30
            color: "#1e1e1e"
            
            // 更新可用宽度
            onWidthChanged: {
                root.availableWidth = width
            }
            
            ListView {
                id: headerView
                anchors.fill: parent
                orientation: ListView.Horizontal
                model: columnModel
                interactive: false // 同样禁止滚动，保持与内容一致（如果内容横向滚动，这里也要同步）
                clip: true
                
                // 同步横向滚动位置
                contentX: marketListView.contentX
                
                delegate: Rectangle {
                    visible: model.visible
                    width: getColumnWidth(index)  // 使用动态计算的宽度
                    height: 30
                    color: "#252526"
                    border.width: 1
                    border.color: "#333333"
                    
                    Text {
                        anchors.centerIn: parent
                        text: model.title
                        color: "#aaaaaa"
                        font.pixelSize: 12
                    }
                    
                    // 拖拽逻辑
                    MouseArea {
                        id: headerDragArea
                        anchors.fill: parent
                        drag.target: dragItem  // 拖拽时显示的影子项
                        drag.axis: Drag.XAxis
                        
                        property int dragStartIndex: -1
                        
                        onPressed: {
                            dragStartIndex = index
                            dragItem.x = parent.mapToItem(headerView, 0, 0).x
                            dragItem.text = model.title
                            dragItem.width = model.width
                            dragItem.visible = true
                        }
                        
                        onReleased: {
                            dragItem.visible = false
                            // 保存列顺序
                            saveColumnConfig()
                        }
                        
                        onPositionChanged: {
                            if (drag.active) {
                                var currentX = parent.mapToItem(headerView, mouse.x, 0).x
                                var targetIndex = headerView.indexAt(currentX, 10)
                                if (targetIndex !== -1 && targetIndex !== index) {
                                    columnModel.move(index, targetIndex, 1)
                                }
                            }
                        }
                    }
                }
            }
            
            // 拖拽时的影子组件
            Rectangle {
                id: dragItem
                visible: false
                height: 30
                color: "#2c3e50"
                opacity: 0.8
                z: 100
                parent: headerView
                
                property alias text: dragLabel.text
                
                Text {
                    id: dragLabel
                    anchors.centerIn: parent
                    color: "white"
                    font.bold: true
                }
            }
        }
        
        // 行拖拽影子
        Rectangle {
            id: rowDragItem
            visible: false
            height: 36
            width: marketListView.width
            color: "#007acc" // 使用高亮色区分
            opacity: 0.8
            z: 200
            parent: marketListView
            
            property string text: ""
            
            Text {
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                text: parent.text
                color: "white"
                font.bold: true
                font.family: "Consolas"
            }
        }

        // 数据列表
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            
            ListView {
                id: marketListView
                model: root.marketModel
                boundsBehavior: Flickable.StopAtBounds
                
                delegate: Rectangle {
                    id: rowDelegate
                    width: marketListView.width
                    height: 36
                    
                    // 显式引用当前行的数据模型，供内部 Repeater 使用
                    property var rowData: model
                    
                    // 选中和斑马纹背景
                    color: {
                        if (root.selectedSet[model.instrumentId]) return "#3e4452" // 多选高亮
                        if (orderController && orderController.instrumentId === model.instrumentId) {
                            return "#2c3e50"
                        }
                        return index % 2 === 0 ? "#1e1e1e" : "#252526"
                    }
                    
                    // 行数据 Repeat
                    Row {
                        anchors.fill: parent
                        
                        Repeater {
                            model: columnModel
                            
                            delegate: Rectangle {
                                visible: model.visible
                                width: getColumnWidth(index)
                                height: 36
                                color: "transparent"
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: getCellValue(model.role, rowDelegate.rowData) // 使用 rowData
                                    color: getCellColor(model.role, rowDelegate.rowData)
                                    font.bold: true
                                    font.family: "Consolas"
                                    font.pixelSize: 14
                                }
                                
                                // 分隔线
                                Rectangle {
                                    width: 1
                                    height: parent.height
                                    color: "#333333"
                                    anchors.right: parent.right
                                    visible: index < columnModel.count - 1
                                }
                            }
                        }
                    }
                    
                    // 左侧选中指示条
                    Rectangle {
                        width: 3
                        height: parent.height
                        color: "#569cd6"
                        visible: orderController && orderController.instrumentId === model.instrumentId
                        z: 2
                    }
                    
                    // 点击交互
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        
                        // 拖拽配置
                        drag.target: rowDragItem
                        drag.axis: Drag.YAxis
                        drag.threshold: 10
                        
                        onPressed: function(mouse) {
                            if (mouse.button === Qt.LeftButton) {
                                // 准备拖拽
                                var posInList = parent.mapToItem(marketListView, 0, 0)
                                rowDragItem.y = posInList.y
                                rowDragItem.text = model.instrumentId
                            }
                        }
                        
                        onPositionChanged: {
                            if (drag.active) {
                                rowDragItem.visible = true
                                var posInList = parent.mapToItem(marketListView, mouse.x, mouse.y)
                                var targetIndex = marketListView.indexAt(10, posInList.y)
                                if (targetIndex !== -1 && targetIndex !== index) {
                                    marketModel.move(index, targetIndex)
                                }
                            }
                        }
                        
                        onReleased: {
                            if (rowDragItem.visible) {
                                rowDragItem.visible = false
                                saveSubscribedInstruments()
                            }
                        }
                        
                        onClicked: function(mouse) {
                            if (rowDragItem.visible) return;
                            
                            var id = model.instrumentId
                            
                            if (mouse.button === Qt.LeftButton) {
                                // 处理多选
                                if (mouse.modifiers & Qt.ControlModifier) {
                                    var newSet = Object.assign({}, root.selectedSet)
                                    if (newSet[id]) delete newSet[id]
                                    else newSet[id] = true
                                    root.selectedSet = newSet
                                    
                                    // 即使是多选，也将最后点击的设为当前活动
                                    if (orderController) orderController.instrumentId = id
                                } else {
                                    // 单选：清空其他，选中当前
                                    root.selectedSet = {}
                                    var s = {}
                                    s[id] = true
                                    root.selectedSet = s
                                    
                                    if (orderController) orderController.instrumentId = id
                                    // 尝试设置价格
                                    if (model.lastPrice > 0 && orderController) orderController.price = model.lastPrice
                                }
                            } else if (mouse.button === Qt.RightButton) {
                                // 右键：如果未选中当前行，则单选当前行；如果已选中，则保持现有选中状态（针对批量操作）
                                if (!root.selectedSet[id]) {
                                    root.selectedSet = {}
                                    var s = {}
                                    s[id] = true
                                    root.selectedSet = s
                                    if (orderController) orderController.instrumentId = id
                                }
                                
                                itemContextMenu.instrumentId = id
                                itemContextMenu.popup()
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 列配置菜单（使用 Popup 替代 Menu，确保不自动关闭）
    Popup {
        id: columnConfigMenu
        width: 180
        height: Math.min(columnModel.count * 35 + 40, 400)
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        
        background: Rectangle {
            color: "#2d2d30"
            border.color: "#555555"
            border.width: 1
            radius: 4
        }
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 5
            spacing: 0
            
            // 标题
            Rectangle {
                Layout.fillWidth: true
                height: 30
                color: "#3e3e42"
                radius: 3
                
                Text {
                    anchors.centerIn: parent
                    text: "配置显示的列"
                    color: "#cccccc"
                    font.pixelSize: 13
                    font.bold: true
                }
            }
            
            // 列表
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                
                ColumnLayout {
                    width: parent.width
                    spacing: 2
                    
                    Repeater {
                        model: columnModel
                        
                        Rectangle {
                            Layout.fillWidth: true
                            height: 32
                            color: checkBoxArea.containsMouse ? "#3e3e42" : "transparent"
                            radius: 3
                            
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: 10
                                
                                // 自定义复选框
                                Rectangle {
                                    width: 16
                                    height: 16
                                    color: "#1e1e1e"
                                    border.color: model.visible ? "#4ec9b0" : "#666666"
                                    border.width: 2
                                    radius: 3
                                    
                                    // 勾选标记
                                    Text {
                                        anchors.centerIn: parent
                                        text: "✓"
                                        color: "#4ec9b0"
                                        font.pixelSize: 12
                                        font.bold: true
                                        visible: model.visible
                                    }
                                }
                                
                                // 列名
                                Text {
                                    Layout.fillWidth: true
                                    text: model.title
                                    color: "#cccccc"
                                    font.pixelSize: 12
                                }
                            }
                            
                            // 点击区域
                            MouseArea {
                                id: checkBoxArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                
                                onClicked: {
                                    // 切换可见性
                                    columnModel.setProperty(index, "visible", !model.visible)
                                    // 保存配置
                                    saveColumnConfig()
                                    // 不关闭菜单
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // 行右键菜单（保持原有功能）
    Menu {
        id: itemContextMenu
        property string instrumentId: ""
        
        MenuItem {
            text: {
                var count = Object.keys(root.selectedSet).length
                return count > 1 ? "❌ 批量取消订阅 (" + count + ")" : "❌ 取消订阅"
            }
            onTriggered: {
                var ids = Object.keys(root.selectedSet)
                // 如果为空（理论上不可能），就用 context menu item id
                if (ids.length === 0 && itemContextMenu.instrumentId) ids = [itemContextMenu.instrumentId]
                
                for (var i = 0; i < ids.length; i++) {
                    var id = ids[i]
                    if (orderController) {
                        var unsubscribeCmd = JSON.stringify({
                            "type": "UNSUBSCRIBE",
                            "id": id
                        })
                        orderController.sendCommand(unsubscribeCmd)
                    }
                    if (marketModel) {
                        marketModel.removeInstrument(id)
                    }
                }
                // 清选
                root.selectedSet = {}
            }
        }
    }
    
    // 新增合约对话框（保持原有功能）... (此处代码与原版相同，但为简洁我省略了部分，实际应保留)
    // 为了完整性，这里我还是简单加上，确保功能不丢失
    Menu {
        id: emptyAreaMenu // 如果点击空白处需要
        MenuItem {
            text: "➕ 新增合约"
            onTriggered: addInstrumentDialog.open()
        }
    }
    
    Dialog {
        id: addInstrumentDialog
        title: "新增合约"
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        
        contentItem: TextField {
            id: newInstrumentInput
            placeholderText: "合约代码 (如 rb2605)"
        }
        
        onAccepted: {
            if (newInstrumentInput.text && orderController) {
                 var subscribeCmd = JSON.stringify({
                    "type": "SUBSCRIBE",
                    "id": newInstrumentInput.text
                })
                orderController.sendCommand(subscribeCmd)
                newInstrumentInput.text = ""
            }
        }
    }
}
