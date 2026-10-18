import Niri
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
  visible: true
  width: 800
  height: 500
  title: "Niri Casts Test"

  // Tracks the result of the most recent action attempt.
  // null = no attempt yet, "" = success, non-empty = error message.
  property var lastActionResult: null

  Niri {
    id: niri
    Component.onCompleted: connect()

    onConnected: {
      console.log("✓ Connected to niri");
      statusText.text = "Connected";
      statusText.color = "green";
    }

    onDisconnected: {
      console.log("✗ Disconnected from niri");
      statusText.text = "Disconnected";
      statusText.color = "red";
    }

    onErrorOccurred: function (error) {
      console.log("✗ Connection error:", error);
      statusText.text = "Connection error: " + error;
      statusText.color = "red";
    }
  }

  Connections {
    target: niri.casts

    function onCountChanged() {
      console.log("Cast count changed:", niri.casts.count);
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
          text: lastActionResult === "" ? "✓ Action OK" : "✗ " + lastActionResult
          color: lastActionResult === "" ? "#2E7D32" : "#C62828"
        }
      }

      Item {
        Layout.fillWidth: true
      }

      Text {
        text: "Total casts: " + niri.casts.count
        font.pixelSize: 12
      }
    }

    Rectangle {
      Layout.fillWidth: true
      height: 1
      color: "#CCC"
    }

    // Screen sharing indicator
    Rectangle {
      Layout.fillWidth: true
      height: 40
      radius: 5
      color: niri.casts.count > 0 ? "#FFEBEE" : "#F5F5F5"
      border.width: 2
      border.color: niri.casts.count > 0 ? "#F44336" : "#CCC"

      Text {
        anchors.centerIn: parent
        font.bold: true
        text: niri.casts.count > 0 ? "● Screen is being shared" : "No active screencasts"
        color: niri.casts.count > 0 ? "#C62828" : "#666"
      }
    }

    Text {
      text: "Start a screencast (e.g. share your screen in a browser, or run wf-recorder) to see it here."
      font.pixelSize: 11
      color: "#666"
    }

    ListView {
      Layout.fillWidth: true
      Layout.fillHeight: true
      clip: true
      spacing: 5
      model: niri.casts

      delegate: Rectangle {
        required property var streamId
        required property var sessionId
        required property string kind
        required property string targetType
        required property string targetOutput
        required property var targetWindowId
        required property bool isDynamicTarget
        required property bool isActive
        required property int pid
        required property var pwNodeId

        width: ListView.view.width
        height: 70
        radius: 5
        color: isActive ? "#E3F2FD" : "#FAFAFA"
        border.width: 1
        border.color: isActive ? "#2196F3" : "#DDD"

        RowLayout {
          anchors.fill: parent
          anchors.margins: 10

          ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Text {
              font.bold: true
              text: {
                if (targetType === "Output")
                  return "Output: " + targetOutput;
                if (targetType === "Window")
                  return "Window: " + targetWindowId;
                return "No target";
              }
            }

            Text {
              font.pixelSize: 11
              text: "Kind: " + kind + " | Stream: " + streamId + " | Session: " + sessionId
                  + (isDynamicTarget ? " | Dynamic target" : "")
            }

            Text {
              font.pixelSize: 11
              color: "#666"
              text: (isActive ? "Active" : "Paused") + " | PID: " + (pid === -1 ? "n/a" : pid)
                  + " | PipeWire node: " + (pwNodeId === -1 ? "n/a" : pwNodeId)
            }
          }

          Button {
            text: "Stop"
            // wlr-screencopy casts cannot be stopped via IPC.
            enabled: kind === "PipeWire"
            onClicked: {
              const result = niri.stopCast(sessionId);
              lastActionResult = result.ok ? "" : result.error;
            }
          }
        }
      }
    }
  }
}
