import QtQuick
import Niri

Niri {
    Component.onCompleted: connect()

    onConnected: console.log("✓ Connected to niri")

    onDisconnected: console.log("✗ Disconnected from niri")

    onErrorOccurred: function(error) {
        console.log("✗ Error:", error)
    }

    onRawEventReceived: function(event) {
        console.log("✓ Event received:", JSON.stringify(event, null, 2))
    }
}
