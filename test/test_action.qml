import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Niri

ApplicationWindow {
    visible: true
    width: 700
    height: 600
    title: "Niri sendRawAction Test"

    // Tracks the result of the most recent send attempt.
    // null = no attempt yet, "" = success, non-empty = error message.
    property var lastResult: null

    // Action templates the user can pick from and edit.
    readonly property var templates: [
        { name: "ToggleOverview",
          json: '{"ToggleOverview": {}}' },
        { name: "FocusWorkspace (by index)",
          json: '{"FocusWorkspace": {"reference": {"Index": 2}}}' },
        { name: "FocusWorkspace (by name)",
          json: '{"FocusWorkspace": {"reference": {"Name": "2"}}}' },
        { name: "CloseWindow (focused)",
          json: '{"CloseWindow": {"id": null}}' },
        { name: "FocusColumnLeft",
          json: '{"FocusColumnLeft": {}}' },
        { name: "FocusColumnRight",
          json: '{"FocusColumnRight": {}}' },
        { name: "MoveColumnLeft",
          json: '{"MoveColumnLeft": {}}' },
        { name: "MoveColumnRight",
          json: '{"MoveColumnRight": {}}' },
        { name: "Spawn (foot)",
          json: '{"Spawn": {"command": ["foot"]}}' },
        { name: "Wrong action",
          json: '{"blah": 1}' }
    ]

    Niri {
        id: niri
        Component.onCompleted: connect()

        onConnected: {
            console.log("✓ Connected to niri")
            sendButton.enabled = true
            statusText.text = "Connected"
            statusText.color = "green"
        }

        onDisconnected: {
            console.log("✗ Disconnected from niri")
            sendButton.enabled = false
            statusText.text = "Disconnected"
            statusText.color = "red"
        }

        onErrorOccurred: function(error) {
            console.log("✗ Connection error:", error)
            statusText.text = "Connection error: " + error
            statusText.color = "red"
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true

            Text {
                id: statusText
                text: "Connecting..."
                font.bold: true
            }

            Item { Layout.fillWidth: true }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#CCC"
        }

        // Template picker
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: "Template:"
                font.bold: true
            }

            ComboBox {
                id: templateCombo
                Layout.fillWidth: true
                model: templates.map(t => t.name)
                onActivated: {
                    payloadInput.text = templates[currentIndex].json
                    lastResult = null
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: 'Action JSON payload (must match <a href="https://docs.rs/niri-ipc/latest/niri_ipc/enum.Action.html">niri\'s Action schema</a>):'
            textFormat: Text.RichText
            font.bold: true
            onLinkActivated: link => Qt.openUrlExternally(link)
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.NoButton
                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: 180

            TextArea {
                id: payloadInput
                text: templates[0].json
                font.family: "monospace"
                font.pixelSize: 12
                wrapMode: TextArea.Wrap
                selectByMouse: true
                placeholderText: "Enter JSON object here..."
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
                id: sendButton
                text: "Send Action"
                onClicked: {
                    let action
                    try {
                        action = JSON.parse(payloadInput.text)
                    } catch (e) {
                        lastResult = e.message
                        return
                    }

                    if (typeof action !== "object" || action === null || Array.isArray(action)) {
                        lastResult = "Payload must be a JSON object"
                        return
                    }

                    console.log("Sending action:", JSON.stringify(action))
                    const result = niri.sendRawAction(action)
                    lastResult = result.ok ? "" : result.error
                }
            }

            Button {
                text: "Clear"
                onClicked: {
                    payloadInput.text = ""
                    lastResult = null
                }
            }

            Item { Layout.fillWidth: true }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#CCC"
        }

        Text {
            text: "Result:"
            font.bold: true
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            radius: 5
            border.width: 2

            color: lastResult === null ? "#F5F5F5" :
                   lastResult === ""   ? "#E8F5E9" : "#FFEBEE"
            border.color: lastResult === null ? "#CCC" :
                          lastResult === ""   ? "#4CAF50" : "#F44336"

            Text {
                anchors.fill: parent
                anchors.margins: 10
                wrapMode: Text.Wrap
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 12
                font.family: "monospace"

                text: lastResult === null ? "(no action sent yet)" :
                      lastResult === ""   ? "✓ OK — action sent successfully" :
                                            "✗ " + lastResult
                color: lastResult === null ? "#666" :
                       lastResult === ""   ? "#2E7D32" : "#C62828"
            }
        }

        Item { Layout.fillHeight: true }
    }
}
