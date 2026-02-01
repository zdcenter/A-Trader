import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.settings

Item {
    id: root
    anchors.fill: parent
    visible: false
    z: 999
    
    function open() {
        visible = true
    }
    
    function close() {
        visible = false
    }

    Settings {
        id: settings
        category: "Trade"
        property int defaultVolume: 1
        property int defaultPriceType: 0
        property int themeIndex: 0
    }

    // 半透明遮罩
    Rectangle {
        anchors.fill: parent
        color: "#dd000000"
        
        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    // 主对话框
    Rectangle {
        id: dialogContent
        width: 520
        height: 480
        anchors.centerIn: parent
        color: "#2b2b2b"
        radius: 10
        border.width: 1
        border.color: "#555555"
        
        MouseArea {
            anchors.fill: parent
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // 标题栏
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 64
                color: "#3c3c3c"
                radius: 10
                
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: parent.radius
                    color: parent.color
                }

                Text {
                    anchors.centerIn: parent
                    text: "⚙ 设置"
                    font.pixelSize: 22
                    font.weight: Font.Bold
                    color: "#ffffff"
                }
            }

            // 内容区域
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: 32

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 28



                    // ========== 默认手数设置 ==========
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 20

                        Text {
                            text: "📊 默认下单手数"
                            font.pixelSize: 16
                            font.weight: Font.Medium
                            color: "#ffffff"
                            Layout.preferredWidth: 140
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            color: "#3c3c3c"
                            radius: 6
                            border.width: 2
                            border.color: volSpin.activeFocus ? "#4a9eff" : "#555555"

                            SpinBox {
                                id: volSpin
                                anchors.fill: parent
                                anchors.margins: 2
                                from: 1
                                to: 1000
                                value: settings.defaultVolume
                                stepSize: 1
                                editable: true
                                onValueModified: settings.defaultVolume = value

                                background: Rectangle { color: "transparent" }

                                contentItem: TextInput {
                                    text: volSpin.textFromValue(volSpin.value, volSpin.locale)
                                    font.pixelSize: 18
                                    font.weight: Font.Medium
                                    color: "#ffffff"
                                    horizontalAlignment: Qt.AlignHCenter
                                    verticalAlignment: Qt.AlignVCenter
                                    readOnly: !volSpin.editable
                                    validator: volSpin.validator
                                    inputMethodHints: Qt.ImhFormattedNumbersOnly
                                }

                                up.indicator: Rectangle {
                                    x: volSpin.width - width - 6
                                    y: 6
                                    width: 36
                                    height: parent.height - 12
                                    color: volSpin.up.pressed ? "#555555" : (volSpin.up.hovered ? "#4a4a4a" : "transparent")
                                    radius: 4

                                    Text {
                                        anchors.centerIn: parent
                                        text: "+"
                                        font.pixelSize: 22
                                        font.weight: Font.Bold
                                        color: "#dddddd"
                                    }
                                }

                                down.indicator: Rectangle {
                                    x: 6
                                    y: 6
                                    width: 36
                                    height: parent.height - 12
                                    color: volSpin.down.pressed ? "#555555" : (volSpin.down.hovered ? "#4a4a4a" : "transparent")
                                    radius: 4

                                    Text {
                                        anchors.centerIn: parent
                                        text: "-"
                                        font.pixelSize: 22
                                        font.weight: Font.Bold
                                        color: "#dddddd"
                                    }
                                }
                            }
                        }
                    }

                    // ========== 默认价格类型 ==========
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 20

                        Text {
                            text: "💰 默认价格类型"
                            font.pixelSize: 16
                            font.weight: Font.Medium
                            color: "#ffffff"
                            Layout.preferredWidth: 140
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 48
                            color: "#3c3c3c"
                            radius: 6
                            border.width: 2
                            border.color: priceCombo.activeFocus ? "#4a9eff" : "#555555"

                            ComboBox {
                                id: priceCombo
                                anchors.fill: parent
                                anchors.margins: 2
                                model: ["限价", "市价", "对手价"]
                                currentIndex: settings.defaultPriceType
                                onActivated: settings.defaultPriceType = currentIndex

                                background: Rectangle { color: "transparent" }

                                contentItem: Text {
                                    leftPadding: 18
                                    text: priceCombo.displayText
                                    font.pixelSize: 18
                                    font.weight: Font.Medium
                                    color: "#ffffff"
                                    verticalAlignment: Text.AlignVCenter
                                }

                                indicator: Text {
                                    x: priceCombo.width - width - 18
                                    y: priceCombo.topPadding + (priceCombo.availableHeight - height) / 2
                                    text: "▼"
                                    font.pixelSize: 14
                                    color: "#dddddd"
                                }

                                popup: Popup {
                                    y: priceCombo.height + 2
                                    width: priceCombo.width
                                    padding: 6

                                    background: Rectangle {
                                        color: "#3c3c3c"
                                        border.color: "#555555"
                                        border.width: 2
                                        radius: 6
                                    }

                                    contentItem: ListView {
                                        clip: true
                                        implicitHeight: contentHeight
                                        model: priceCombo.delegateModel
                                        currentIndex: priceCombo.highlightedIndex

                                        delegate: ItemDelegate {
                                            width: ListView.view.width - 12
                                            height: 44

                                            background: Rectangle {
                                                color: parent.highlighted ? "#4a9eff" : (parent.hovered ? "#4a4a4a" : "transparent")
                                                radius: 4
                                            }

                                            contentItem: Text {
                                                text: modelData
                                                color: "#ffffff"
                                                font.pixelSize: 17
                                                font.weight: Font.Medium
                                                verticalAlignment: Text.AlignVCenter
                                                leftPadding: 14
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // 底部按钮区
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 70
                color: "#3c3c3c"
                radius: 10

                Rectangle {
                    anchors.top: parent.top
                    width: parent.width
                    height: parent.radius
                    color: parent.color
                }

                RowLayout {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 28
                    spacing: 14

                    Button {
                        text: "关闭"
                        Layout.preferredWidth: 110
                        Layout.preferredHeight: 44
                        
                        background: Rectangle {
                            color: parent.down ? "#3a7fd5" : (parent.hovered ? "#5aafff" : "#4a9eff")
                            radius: 6
                        }
                        
                        contentItem: Text {
                            text: parent.text
                            font.pixelSize: 16
                            font.weight: Font.Bold
                            color: "#ffffff"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        
                        onClicked: root.close()
                    }
                }
            }
        }
    }
}
