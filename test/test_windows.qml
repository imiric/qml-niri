import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Niri 0.1

ApplicationWindow {
    visible: true
    width: 800
    height: 600
    title: "Niri Windows Test"

    // Tracks the result of the most recent action attempt.
    // null = no attempt yet, "" = success, non-empty = error message.
    property var lastActionResult: null

    Niri {
        id: niri
        Component.onCompleted: connect()

        onConnected: {
            console.log("✓ Connected to niri")
            statusText.text = "Connected"
            statusText.color = "green"
        }

        onDisconnected: {
            console.log("✗ Disconnected from niri")
            statusText.text = "Disconnected"
            statusText.color = "red"
        }

        onErrorOccurred: function(error) {
            console.log("✗ Connection error:", error)
            statusText.text = "Connection error: " + error
            statusText.color = "red"
        }

        onFocusedWindowChanged: {
            console.log("Focused window changed:", niri.focusedWindow?.title)
        }

        readonly property SortFilterProxyModel sortedWindows: SortFilterProxyModel {
            model: niri.windows
            sorters: [
                RoleSorter {
                    roleName: "workspaceId"
                },
                RoleSorter {
                    roleName: "columnIndex"
                },
                RoleSorter {
                    roleName: "tileIndex"
                }
            ]
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10

        // Status header
        RowLayout {
            Layout.fillWidth: true

            Text {
                id: statusText
                text: "Connecting..."
                font.bold: true
            }

            // Last action result indicator
            Rectangle {
                Layout.preferredHeight: 20
                Layout.preferredWidth: actionResultText.implicitWidth + 10
                visible: lastActionResult !== null
                radius: 3
                color: lastActionResult === "" ? "#E8F5E9" : "#FFEBEE"
                border.width: 1
                border.color: lastActionResult === "" ? "#4CAF50" : "#F44336"

                Text {
                    id: actionResultText
                    anchors.centerIn: parent
                    font.pixelSize: 11
                    text: lastActionResult === "" ? "✓ Action OK"
                                                  : "✗ " + lastActionResult
                    color: lastActionResult === "" ? "#2E7D32" : "#C62828"
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: "Total windows: " + niri.windows.count
                font.pixelSize: 12
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#CCC"
        }

        // Focused window info
        Rectangle {
            Layout.fillWidth: true
            height: 100
            color: "#E8F5E9"
            border.color: "#4CAF50"
            border.width: 2
            radius: 5

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10

                Text {
                    text: "Currently Focused Window"
                    font.bold: true
                    font.pixelSize: 14
                    color: "#2E7D32"
                }

                Text {
                    text: "Title: " + (niri.focusedWindow?.title ?? "(none)")
                    font.pixelSize: 12
                    font.italic: !niri.focusedWindow
                }

                Text {
                    text: "App ID: " + (niri.focusedWindow?.appId ?? "(none)")
                    font.pixelSize: 12
                    font.italic: !niri.focusedWindow
                }
            }
        }

        Text {
            text: "All Windows (click to focus, right-click to close)"
            font.pixelSize: 10
            color: "#666"
            font.italic: true
        }

        // All windows list
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            model: niri.sortedWindows
            spacing: 5
            clip: true

            delegate: Rectangle {
                width: ListView.view.width
                height: 70
                color: model.isFocused ? "#4CAF50" : "#F5F5F5"
                border.color: model.isUrgent ? "red" : "#CCC"
                border.width: model.isUrgent ? 3 : 1
                radius: 5

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true

                    onEntered: {
                        parent.opacity = 0.8
                    }

                    onExited: {
                        parent.opacity = 1.0
                    }

                    onClicked: function(mouse) {
                        if (mouse.button === Qt.LeftButton) {
                            console.log("Focusing window", model.id, "-", model.title)
                            const r = niri.focusWindow(model.id)
                            lastActionResult = r.ok ? "" : r.error
                        } else if (mouse.button === Qt.RightButton) {
                            console.log("Closing window", model.id, "-", model.title)
                            const r = niri.closeWindow(model.id)
                            lastActionResult = r.ok ? "" : r.error
                        }
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 2

                    RowLayout {
                        spacing: 5

                        Image {
                            source: model.iconPath ? "file://" + model.iconPath : ""
                            sourceSize.width: 16
                            sourceSize.height: 16
                            visible: model.iconPath !== ""
                            smooth: true
                        }

                        // Fallback for missing icons
                        Rectangle {
                            width: 16
                            height: 16
                            color: model.isFocused ? "#999" : "#CCC"
                            visible: model.iconPath === ""
                            radius: 2
                        }

                        Text {
                            text: model.title || "(no title)"
                            font.bold: model.isFocused
                            font.pixelSize: 14
                            font.italic: !model.title
                            color: model.isFocused ? "white" : "black"
                        }

                        Text {
                            text: "● FOCUSED"
                            font.bold: true
                            color: "white"
                            visible: model.isFocused
                        }

                        Text {
                            text: "⚠ URGENT"
                            color: "darkred"
                            font.bold: true
                            visible: model.isUrgent
                        }

                        Text {
                            text: "[Floating]"
                            color: model.isFocused ? "white" : "#666"
                            font.italic: true
                            visible: model.isFloating
                        }
                    }

                    RowLayout {
                        Text {
                            text: "App: " + (model.appId || "unknown")
                            font.pixelSize: 10
                            color: model.isFocused ? "white" : "#666"
                        }
                        Text {
                            text: "PID: " + (model.pid >= 0 ? model.pid : "unknown")
                            font.pixelSize: 10
                            color: model.isFocused ? "white" : "#666"
                        }
                    }

                    Text {
                        text: [
                            `ID: ${model.id}`,
                            `Workspace: ${model.workspaceId || "none"}`,
                            `Size: ${model.windowWidth}x${model.windowHeight}`,
                            `Column index: ${model.columnIndex}`,
                            `Tile index: ${model.tileIndex}`,
                            `Tile size: ${model.tileWidth}x${model.tileHeight}`,
                            `Offset in tile: ${model.windowOffsetX},${model.windowOffsetY}`,
                            `Tile position: ${model.tilePosX},${model.tilePosY}`
                        ].join(" | ")
                        font.pixelSize: 10
                        color: model.isFocused ? "white" : "#888"
                    }
                }
            }
        }
    }
}
