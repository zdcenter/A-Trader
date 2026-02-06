import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.settings 1.0

Window {
    id: settingsWindow
    width: 800
    height: 600
    minimumWidth: 700
    minimumHeight: 500
    title: "设置"
    color: "#1e1e1e"

    // 引用主窗口以实现即时主题切换
    property var mainWindow: null

    onClosing: {
        visible = false
    }

    // 交易设置（从外部传入，不在这里定义）
    // property var tradeSettings 已在 main.qml 中传入
    
    // UI 设置（仅用于持久化，实际值从 mainWindow 读取）
    Settings {
        id: uiSettings
        category: "UI"
        property int themeIndex: 0
        property int fontSize: 16
    }
    
    // 连接设置
    Settings {
        id: connectionSettings
        category: "Connection"
        property string serverAddress: "localhost"
        property int pubPort: 5555
        property int repPort: 5556
    }
    
    // 声音设置（从外部传入）
    property var soundSettings

    
    // 主题列表（与 main.qml 保持一致）
    readonly property var themeNames: [
        "深色经典",
        "文华财经", 
        "经典黑金",
        "深蓝专业",
        "暖夜护眼",
        "高对比度"
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 标题栏
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#1e1e1e"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 24
                spacing: 16

                Text {
                    text: "⚙️ 设置"
                    font.pixelSize: 20
                    font.weight: Font.Bold
                    color: "#ffffff"
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "关闭"
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 36
                    onClicked: settingsWindow.visible = false

                    background: Rectangle {
                        color: parent.hovered ? "#3c3c3c" : "#2b2b2b"
                        radius: 4
                    }

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 14
                        color: "#ffffff"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        // 主内容区
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            RowLayout {
                anchors.fill: parent
                spacing: 0

                // 左侧导航
                Rectangle {
                    Layout.preferredWidth: 200
                    Layout.fillHeight: true
                    color: "#252525"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.topMargin: 16
                        spacing: 4

                        Repeater {
                            model: [
                                {icon: "🎨", text: "界面设置"},
                                {icon: "📊", text: "交易设置"},
                                {icon: "🌐", text: "连接设置"},
                                {icon: "🔊", text: "声音设置"}
                            ]

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 48
                                color: tabView.currentIndex === index ? "#3c3c3c" : "transparent"

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: tabView.currentIndex = index
                                    cursorShape: Qt.PointingHandCursor
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 20
                                    spacing: 12

                                    Text {
                                        text: modelData.icon
                                        font.pixelSize: 18
                                    }

                                    Text {
                                        text: modelData.text
                                        font.pixelSize: 14
                                        font.weight: tabView.currentIndex === index ? Font.Bold : Font.Normal
                                        color: tabView.currentIndex === index ? "#ffffff" : "#999999"
                                    }
                                }

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 4
                                    height: parent.height * 0.6
                                    color: "#4a9eff"
                                    visible: tabView.currentIndex === index
                                }
                            }
                        }

                        Item { Layout.fillHeight: true }
                    }
                }

                // 右侧内容
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#1e1e1e"

                    StackLayout {
                        id: tabView
                        anchors.fill: parent
                        anchors.margins: 32
                        currentIndex: 0

                        // ========== 界面设置 ==========
                        ScrollView {
                            clip: true

                            ColumnLayout {
                                width: tabView.width - 64
                                spacing: 24

                                // 配色方案
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 12

                                    Text {
                                        text: "配色方案"
                                        font.pixelSize: 16
                                        font.weight: Font.Bold
                                        color: "#ffffff"
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 48
                                        color: "#2b2b2b"
                                        radius: 6
                                        border.width: 1
                                        border.color: "#3c3c3c"

                                        ComboBox {
                                            anchors.fill: parent
                                            anchors.margins: 4
                                            model: themeNames
                                            currentIndex: mainWindow ? mainWindow.themeIndex : 0
                                            onActivated: {
                                                if (mainWindow) {
                                                    mainWindow.themeIndex = currentIndex
                                                    uiSettings.themeIndex = currentIndex
                                                }
                                            }

                                            background: Rectangle { color: "transparent" }
                                            contentItem: Text {
                                                text: parent.displayText
                                                font.pixelSize: 14
                                                color: "#ffffff"
                                                verticalAlignment: Text.AlignVCenter
                                                leftPadding: 12
                                            }
                                        }
                                    }
                                }

                                // 字体大小
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 12

                                    Text {
                                        text: "字体大小"
                                        font.pixelSize: 16
                                        font.weight: Font.Bold
                                        color: "#ffffff"
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 48
                                        color: "#2b2b2b"
                                        radius: 6
                                        border.width: 1
                                        border.color: "#3c3c3c"

                                        ComboBox {
                                            id: fontSizeCombo
                                            anchors.fill: parent
                                            anchors.margins: 4
                                            
                                            model: [
                                                {"label": "极小 (12px)", "value": 12},
                                                {"label": "较小 (14px)", "value": 14},
                                                {"label": "标准 (16px)", "value": 16},
                                                {"label": "较大 (18px)", "value": 18},
                                                {"label": "大 (20px)", "value": 20},
                                                {"label": "极大 (24px)", "value": 24}
                                            ]
                                            
                                            textRole: "label"
                                            
                                            Component.onCompleted: {
                                                for (var i = 0; i < model.length; i++) {
                                                    if (model[i].value === uiSettings.fontSize) {
                                                        currentIndex = i
                                                        break
                                                    }
                                                }
                                            }
                                            
                                            onActivated: {
                                                uiSettings.fontSize = model[currentIndex].value
                                                Qt.application.font.pixelSize = uiSettings.fontSize
                                            }

                                            background: Rectangle { color: "transparent" }
                                            contentItem: Text {
                                                text: fontSizeCombo.displayText
                                                font.pixelSize: 14
                                                color: "#ffffff"
                                                verticalAlignment: Text.AlignVCenter
                                                leftPadding: 12
                                            }
                                        }
                                    }


                                }

                                Item { Layout.fillHeight: true }
                            }
                        }

                        // ========== 交易设置 ==========
                        ScrollView {
                            clip: true

                            ColumnLayout {
                                width: tabView.width - 64
                                spacing: 24

                                // 默认下单手数
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 12

                                    Text {
                                        text: "默认下单手数"
                                        font.pixelSize: 16
                                        font.weight: Font.Bold
                                        color: "#ffffff"
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 48
                                        color: "#2b2b2b"
                                        radius: 6
                                        border.width: 1
                                        border.color: "#3c3c3c"

                                        SpinBox {
                                            id: volumeSpinBox
                                            anchors.fill: parent
                                            anchors.margins: 4
                                            from: 1
                                            to: 1000
                                            value: tradeSettings.defaultVolume
                                            stepSize: 1
                                            editable: true
                                            onValueModified: tradeSettings.defaultVolume = value

                                            // 上箭头按钮
                                            up.indicator: Rectangle {
                                                x: volumeSpinBox.width - width - 4
                                                y: 4
                                                width: 32
                                                height: parent.height / 2 - 6
                                                color: volumeSpinBox.up.pressed ? "#3c3c3c" : "#2b2b2b"
                                                border.color: "#555555"
                                                border.width: 1
                                                radius: 3
                                                
                                                Text {
                                                    text: "▲"
                                                    font.pixelSize: 10
                                                    color: volumeSpinBox.up.hovered ? "#ffffff" : "#aaaaaa"
                                                    anchors.centerIn: parent
                                                }
                                            }

                                            // 下箭头按钮
                                            down.indicator: Rectangle {
                                                x: volumeSpinBox.width - width - 4
                                                y: parent.height / 2 + 2
                                                width: 32
                                                height: parent.height / 2 - 6
                                                color: volumeSpinBox.down.pressed ? "#3c3c3c" : "#2b2b2b"
                                                border.color: "#555555"
                                                border.width: 1
                                                radius: 3
                                                
                                                Text {
                                                    text: "▼"
                                                    font.pixelSize: 10
                                                    color: volumeSpinBox.down.hovered ? "#ffffff" : "#aaaaaa"
                                                    anchors.centerIn: parent
                                                }
                                            }

                                            background: Rectangle { color: "transparent" }
                                            contentItem: TextInput {
                                                text: parent.textFromValue(parent.value, parent.locale)
                                                font.pixelSize: 14
                                                color: "#ffffff"
                                                horizontalAlignment: Qt.AlignHCenter
                                                verticalAlignment: Qt.AlignVCenter
                                                readOnly: !parent.editable
                                                validator: parent.validator
                                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                            }
                                        }
                                    }
                                }

                                // 默认价格类型
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 12

                                    Text {
                                        text: "默认价格类型"
                                        font.pixelSize: 16
                                        font.weight: Font.Bold
                                        color: "#ffffff"
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 48
                                        color: "#2b2b2b"
                                        radius: 6
                                        border.width: 1
                                        border.color: "#3c3c3c"

                                        ComboBox {
                                            anchors.fill: parent
                                            anchors.margins: 4
                                            model: ["限价", "市价", "对手价"]
                                            currentIndex: tradeSettings.defaultPriceType
                                            onActivated: tradeSettings.defaultPriceType = currentIndex

                                            background: Rectangle { color: "transparent" }
                                            contentItem: Text {
                                                text: parent.displayText
                                                font.pixelSize: 14
                                                color: "#ffffff"
                                                verticalAlignment: Text.AlignVCenter
                                                leftPadding: 12
                                            }
                                        }
                                    }
                                }

                                Item { Layout.fillHeight: true }
                            }
                        }

                        // ========== 连接设置 ==========
                        ScrollView {
                            clip: true

                            ColumnLayout {
                                width: tabView.width - 64
                                spacing: 24

                                // 服务器地址
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 12

                                    Text {
                                        text: "服务器地址"
                                        font.pixelSize: 16
                                        font.weight: Font.Bold
                                        color: "#ffffff"
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 48
                                        color: "#2b2b2b"
                                        radius: 6
                                        border.width: 1
                                        border.color: "#3c3c3c"

                                        TextField {
                                            anchors.fill: parent
                                            anchors.margins: 4
                                            text: connectionSettings.serverAddress
                                            placeholderText: "例如: localhost 或 192.168.1.100"
                                            onEditingFinished: connectionSettings.serverAddress = text

                                            background: Rectangle { color: "transparent" }
                                            color: "#ffffff"
                                            font.pixelSize: 14
                                            leftPadding: 12
                                        }
                                    }
                                }

                                // PUB 端口
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 12

                                    Text {
                                        text: "PUB 端口（行情推送）"
                                        font.pixelSize: 16
                                        font.weight: Font.Bold
                                        color: "#ffffff"
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 48
                                        color: "#2b2b2b"
                                        radius: 6
                                        border.width: 1
                                        border.color: "#3c3c3c"

                                        SpinBox {
                                            anchors.fill: parent
                                            anchors.margins: 4
                                            from: 1024
                                            to: 65535
                                            value: connectionSettings.pubPort
                                            stepSize: 1
                                            editable: true
                                            onValueModified: connectionSettings.pubPort = value

                                            background: Rectangle { color: "transparent" }
                                            contentItem: TextInput {
                                                text: parent.textFromValue(parent.value, parent.locale)
                                                font.pixelSize: 14
                                                color: "#ffffff"
                                                horizontalAlignment: Qt.AlignHCenter
                                                verticalAlignment: Qt.AlignVCenter
                                                readOnly: !parent.editable
                                                validator: parent.validator
                                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                            }
                                        }
                                    }
                                }

                                // REP 端口
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 12

                                    Text {
                                        text: "REP 端口（交易请求）"
                                        font.pixelSize: 16
                                        font.weight: Font.Bold
                                        color: "#ffffff"
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 48
                                        color: "#2b2b2b"
                                        radius: 6
                                        border.width: 1
                                        border.color: "#3c3c3c"

                                        SpinBox {
                                            anchors.fill: parent
                                            anchors.margins: 4
                                            from: 1024
                                            to: 65535
                                            value: connectionSettings.repPort
                                            stepSize: 1
                                            editable: true
                                            onValueModified: connectionSettings.repPort = value

                                            background: Rectangle { color: "transparent" }
                                            contentItem: TextInput {
                                                text: parent.textFromValue(parent.value, parent.locale)
                                                font.pixelSize: 14
                                                color: "#ffffff"
                                                horizontalAlignment: Qt.AlignHCenter
                                                verticalAlignment: Qt.AlignVCenter
                                                readOnly: !parent.editable
                                                validator: parent.validator
                                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 80
                                    color: "#2b2b2b"
                                    radius: 6

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.margins: 16
                                        spacing: 12

                                        Text {
                                            text: "⚠️"
                                            font.pixelSize: 24
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            text: "连接设置修改后需要重启应用才能生效。\n请确保 CTP Core 使用相同的端口配置。"
                                            font.pixelSize: 13
                                            color: "#ffaa00"
                                            wrapMode: Text.WordWrap
                                        }
                                    }
                                }


                        
                                Item { Layout.fillHeight: true }
                            }
                        }
                        
                        // ========== 声音设置 ==========
                        ScrollView {
                            clip: true

                            ColumnLayout {
                                width: tabView.width - 64
                                spacing: 24

                                // 声音开关
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 12

                                    Text {
                                        text: "声音反馈"
                                        font.pixelSize: 16
                                        font.weight: Font.Bold
                                        color: "#ffffff"
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 100
                                        color: "#2b2b2b"
                                        radius: 6
                                        border.width: 1
                                        border.color: "#3c3c3c"

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 16
                                            spacing: 16

                                            RowLayout {
                                                Layout.fillWidth: true
                                                
                                                Text {
                                                    text: "下单反馈音（成功/失败/撤单）"
                                                    font.pixelSize: 14
                                                    color: "#ffffff"
                                                    Layout.fillWidth: true
                                                }
                                                
                                                Switch {
                                                    checked: soundSettings.enableOrderSound
                                                    onCheckedChanged: soundSettings.enableOrderSound = checked
                                                }
                                            }
                                            
                                            Rectangle { height: 1; Layout.fillWidth: true; color: "#3c3c3c" }

                                            RowLayout {
                                                Layout.fillWidth: true
                                                
                                                Text {
                                                    text: "成交反馈音"
                                                    font.pixelSize: 14
                                                    color: "#ffffff"
                                                    Layout.fillWidth: true
                                                }
                                                
                                                Switch {
                                                    checked: soundSettings.enableTradeSound
                                                    onCheckedChanged: soundSettings.enableTradeSound = checked
                                                }
                                            }
                                        }
                                    }
                                }
                                
                                // 音量设置
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 12

                                    Text {
                                        text: "音量设置"
                                        font.pixelSize: 16
                                        font.weight: Font.Bold
                                        color: "#ffffff"
                                    }

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 60
                                        color: "#2b2b2b"
                                        radius: 6
                                        border.width: 1
                                        border.color: "#3c3c3c"

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.margins: 16
                                            spacing: 16
                                            
                                            Text {
                                                text: "🔈" 
                                                font.pixelSize: 16 
                                            }

                                            Slider {
                                                Layout.fillWidth: true
                                                from: 0
                                                to: 100
                                                value: soundSettings.volume
                                                stepSize: 1
                                                onMoved: soundSettings.volume = value
                                            }
                                            
                                            Text {
                                                text: soundSettings.volume + "%"
                                                font.pixelSize: 14
                                                color: "#ffffff"
                                                Layout.preferredWidth: 40
                                                horizontalAlignment: Text.AlignRight
                                            }
                                        }
                                    }
                                }

                                Item { Layout.fillHeight: true }
                            }
                        }
                    }
                }
            }
        }
    }
}
