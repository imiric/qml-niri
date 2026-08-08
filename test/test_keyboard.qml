import QtQuick
import Niri
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
  visible: true
  title: "Niri xkb test"

  property var lastActionResult: null
  
  Niri {
    id: niri
    Component.onCompleted: connect();

    onConnected: {
      status.text = "Connected";
      status.color = "green";
      console.log("succefully connected to niri");
    }
    onErrorOccurred: function(error) {
      status.text = "Error";
      status.color = "red";
      console.error("error occurred!", error);
    }
    onDisconnected: {
      status.text = "Disconnected";
      status.color = "red";
      console.warn("disconnected from niri");
    }

    keyboardLayouts.onNamesChanged: {
      console.log("(names chng) names", niri.keyboardLayouts.names);
      console.log("(names chng) currentIndex", niri.keyboardLayouts.currentIndex);
      console.log("(names chng) currentName", niri.keyboardLayouts.currentName);
    }

    keyboardLayouts.onCurrentIndexChanged: {
      console.log("(idx chng) currentName", niri.keyboardLayouts.currentName);
      console.log("(idx chng) currentIndex", niri.keyboardLayouts.currentIndex)
    }
  }

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 10
    spacing: 10

    RowLayout {
      Layout.fillWidth: true

      Text {
        id: status
        text: "Connecting..."
        font.bold: true
      }

      Rectangle {
        Layout.preferredHeight: 20
        Layout.preferredWidth: actionResult.implicitWidth + 10
        radius: 3
        color: lastActionResult === "" ? "#E8F5E9" : "#FFEBEE"
        visible: lastActionResult !== null

        Text {
          id: actionResult
          anchors.centerIn: parent
          font.pixelSize: 11
          text: lastActionResult === "" ? "✓ Action OK"
                                                  : "✗ " + lastActionResult
          color: lastActionResult === "" ? "#2E7D32" : "#C62828"
        }
      }

      Item { Layout.fillWidth: true }

      Text {
        text: "Current layout: " + niri.keyboardLayouts.currentName
        font.pixelSize: 12
      }
    }

 RowLayout {
      Layout.fillWidth: true

      ListView {
        Layout.fillWidth: true
        Layout.preferredHeight: 60
        model: niri.keyboardLayouts ? niri.keyboardLayouts.names : [] // QStringList
        orientation: ListView.Horizontal

        spacing: 5
        clip: true

        delegate: Rectangle {
          readonly property var isCurrent: index === niri.keyboardLayouts.currentIndex

          height: ListView.view.height
          width: 100

          color: isCurrent ? "#4CAF50" : "#F5F5F5";
          radius: 5

          MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
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
                const r = niri.switchKeyboardLayoutByIndex(index);
              lastActionResult = r.ok ? "" : r.error;
            }
          }
        }

        ColumnLayout {
          anchors.fill: parent
          anchors.margins: 8
          spacing: 5

          Text {
            text: modelData
            font.bold: isCurrent
            font.pixelSize: 14
            color: isCurrent ? "white" : "black"
          }

          Text {
            text: "● CURRENT"
            font.bold: true
            color: "white"
            visible: isCurrent
          }
        }
      }
    }
    }

    
    RowLayout {
      Layout.fillWidth: true
      
      Text {
        text: "An placeholder for any text"
      }

      Item { Layout.fillWidth: true }

      RowLayout {
        spacing: 10
        layoutDirection: Qt.RightToLeft

        Button {
          text: "Switch next"
          onClicked: {
            const r = niri.switchKeyboardLayoutNext();
            lastActionResult = r.ok ? "" : r.error;
          }
        }

        Button {
          text: "Switch prev"
          onClicked: {
            const r = niri.switchKeyboardLayoutPrev();
            lastActionResult = r.ok ? "" : r.error;
          }
        }
      }
    }

    TextInput {
      Layout.fillWidth: true
      Layout.fillHeight: true
    }
  }
}

