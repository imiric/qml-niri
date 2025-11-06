# qml-niri

A QML plugin for interacting with the [niri](https://github.com/YaLTeR/niri) Wayland compositor via its IPC protocol.

<details>
  <summary>Why?</summary>

I really like the niri compositor/WM, but there are no good integrations for it for building UI widgets, status bars, etc. There are several options mentioned in the [awesome-niri](https://github.com/Vortriz/awesome-niri) list, but none of them are great IMO.

[Quickshell](https://quickshell.outfoxxed.me/) and Qt Quick stand out above the rest, but it currently only supports Hyprland. There is interest in adding [support for niri](https://github.com/quickshell-mirror/quickshell/issues/47), but the feature is blocked by the author's desire for compositors to implement a set of [generic Wayland protocols](https://github.com/quickshell-mirror/shell-protocols), which is in progress for niri. This is understandable, as it would avoid projects like Quickshell having to add support for custom IPC protocols of each compositor, but in the meantime, niri users are left without a good solution. If, and when, Quickshell officially supports niri via these generic protocols, there will likely be little need for qml-niri to exist.

The [DankMaterialShell](https://github.com/AvengeMedia/DankMaterialShell) project uses Quickshell and integrates with niri, but it is too complex and fancy for my personal needs. Extracting their [`NiriService`](https://github.com/AvengeMedia/DankMaterialShell/blob/a17343f40e2c2788d775aef08e55f73ce6e20ae2/Services/NiriService.qml) could've been an option, but I'd rather keep my QML configuration simple, with the IPC implementation at a lower level.
</details>


## Features

- Real-time window and workspace monitoring and switching
- Tracking of focus, urgency, layout changes, etc.
- Application icon lookup via XDG desktop entries
- Event-driven updates for all compositor changes
- Native QML integration with Qt 6


## Requirements

- Qt 6 (Core, GUI, and QML modules)
- CMake 3.16 or newer
- C++17 compatible compiler
- A recent version of niri (tested with v25.08)


## Disclaimer

The author is an experienced programmer, but not with C++ or Qt. Most of this project was written with the assistance of LLM tools such as Claude Sonnet 4.5. That said, **nothing was "vibe-coded", and all code was carefully reviewed and tested**.

If you do run into any issues, or have improvement suggestions, creating a [GitHub issue](https://github.com/imiric/qml-niri/issues) would be appreciated.


## Installation

### Nix

Add this flake to your `inputs` in `flake.nix`:

```nix
{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

	quickshell = {
	  url = "git+https://git.outfoxxed.me/outfoxxed/quickshell";
	  inputs.nixpkgs.follows = "nixpkgs";
	};

	qml-niri = {
      url = "github:juuyokka/qml-niri/feat-nix-flake";
      inputs.nixpkgs.follows = "nixpkgs";
	  inputs.quickshell.follows = "quickshell";
    };
  };

  # ...
}
```

The plugin itself is available under `qml-niri.packages.<system>.default`. For those wishing to use it with Quickshell, the flake also provides a build of Quickshell with the plugin under `qml-niri.packages.<system>.quickshell`.

### Building from source

Install [just](https://github.com/casey/just) and run:
```bash
git clone https://github.com/imiric/qml-niri.git
cd qml-niri
just build
```

The `just build` command will create a `build` directory and compile the plugin. The built plugin will be located in `build/Niri/`.

### Installing system-wide

After building, copy the plugin to your QML import path:

```bash
# Find your QML import path
qtpaths6 --qt-query QT_INSTALL_QML

# Copy the plugin (adjust path as needed)
sudo cp -r build/Niri /usr/lib64/qt6/qml/
```

Alternatively, you can set the `QML_IMPORT_PATH` environment variable to include the build directory when running your QML applications.


## Usage

### Basic setup

Import the plugin and create a Niri instance:

```qml
import QtQuick
import Niri 0.1

Item {
    Niri {
        id: niri
        Component.onCompleted: connect()
        
        onConnected: console.log("Connected to niri")
        onErrorOccurred: function(error) {
            console.error("Error:", error)
        }
    }
}
```

> [!NOTE]
> This requires the `NIRI_SOCKET` environment variable to be set with the path to a
> valid Unix socket.
> See the [niri IPC documentation](https://github.com/YaLTeR/niri/wiki/IPC) for details.


### Logging

The plugin will output informational and error messages by default. To enable verbose
logging for troubleshooting, set the `QT_LOGGING_RULES` environment variable. E.g.:
```shell
export QT_LOGGING_RULES="niri.debug=true"
```

This will also show debug and warning messages for icon lookup, IPC communication, and event handling.


### Working with workspaces

Access workspace information via the `workspaces` model:

```qml
ListView {
    model: niri.workspaces
    delegate: Rectangle {
        Text {
            text: "Workspace " + model.index + 
                  (model.isFocused ? " (focused)" : "")
        }
        MouseArea {
            anchors.fill: parent
            onClicked: niri.focusWorkspaceById(model.id)
        }
    }
}
```

Available workspace properties:
- `id`: Unique workspace identifier
- `index`: Workspace position on its output
- `name`: Optional workspace name
- `output`: Output device name
- `isActive`: Currently active on its output
- `isFocused`: Currently focused workspace
- `isUrgent`: Has windows requesting attention
- `activeWindowId`: ID of the active window
- `maxCount`: Maximum number of workspaces to expose (default: unlimited, shows all workspaces)

#### Limiting workspace count

You can limit the number of workspaces exposed by the model using the `maxCount` property. Only the first `maxCount` workspaces (after sorting) will be available:

```qml
Component.onCompleted: {
    niri.workspaces.maxCount = 11
}
```

This is useful for widget layouts that should only show a fixed number of workspaces.


### Working with windows

Access window information via the `windows` model:

```qml
ListView {
    model: niri.windows
    delegate: Rectangle {
        color: model.isFocused ? "lightblue" : "white"
        
        Text {
            text: model.title + " (" + model.appId + ")"
        }
        
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: function(mouseEvent) {
                if (mouseEvent.button === Qt.LeftButton) {
                    niri.focusWindow(model.id)
                } else {
                    niri.closeWindow(model.id)
                }
            }
        }
    }
}
```

Available window properties:
- `id`: Unique window identifier
- `title`: Window title
- `appId`: Application identifier
- `pid`: Process ID (-1 if unavailable)
- `workspaceId`: Current workspace ID
- `isFocused`: Currently focused window
- `isFloating`: Floating window state
- `isUrgent`: Window urgency flag
- `iconPath`: Absolute path to application icon (empty if not found)

#### Application icons

Application icons are automatically looked up using XDG desktop entries, and can be rendered like so:

```qml
ListView {
    model: niri.windows
    delegate: Rectangle {
        RowLayout {
            spacing: 5
            
            Image {
                source: model.iconPath ? "file://" + model.iconPath : ""
                sourceSize.width: 24
                sourceSize.height: 24
                visible: model.iconPath !== ""
                smooth: true
            }
            
            // Fallback for missing icons
            Rectangle {
                width: 24
                height: 24
                color: "#CCC"
                visible: model.iconPath === ""
                radius: 4
            }
            
            Text {
                text: model.title
            }
        }
    }
}
```

If an icon is not found (e.g. for AppImage, Flatpak, Snap apps), you can manually place an SVG or PNG file in a general XDG path, such as `~/.local/share/icons/hicolor/scalable/apps`. Ensure that it's named after the application ID that niri reports (check with `niri msg pick-window`). Although a lowercase string, or having the name anywhere in the file name should work as well.

For example, for app ID "LibreWolf", the file `~/.local/share/icons/hicolor/scalable/apps/librewolf.svg` would be resolved.

The implementation attempts to handle several path and naming variations, but it might not work in all scenarios, so a manual override is preferred over handling all scenarios correctly.

### Convenience properties

Access the currently focused window and all of its properties:

```qml
Text {
    text: niri.focusedWindow?.title ?? "No focused window"
}

Text {
    text: "App: " + (niri.focusedWindow?.appId ?? "none")
}

Text {
    text: "PID: " + (niri.focusedWindow?.pid ?? -1)
}
```

Count of total windows and workspaces:
```qml
Text {
    text: "Total windows: " + niri.windows.count
}

Text {
    text: "Total workspaces: " + niri.workspaces.count
}
```

### Available methods

Workspace control:
```qml
niri.focusWorkspace(0)              // By index
niri.focusWorkspaceById(12345)      // By ID
niri.focusWorkspaceByName("code")   // By name
```

Window control:
```qml
niri.focusWindow(windowId)
niri.closeWindow(windowId)
niri.closeWindowOrFocused()         // Close focused window
```


## Testing

The plugin was mostly tested manually, using a few [integration tests](/test). You can run them with:

```bash
# Test event stream
just test events

# Test workspace model
just test workspaces

# Test window model
just test windows
```

Pull requests to improve the testing situation, add unit tests, etc., are very welcome!


## API Reference

### Niri Object

*Properties:*
- `workspaces`: WorkspaceModel - List of all workspaces
- `windows`: WindowModel - List of all windows
- `focusedWindow`: Window - Currently focused window (null if none)

*Methods:*
- `connect()`: bool - Connect to niri IPC socket
- `isConnected()`: bool - Check connection status
- `focusWorkspace(index)` - Focus workspace by index
- `focusWorkspaceById(id)` - Focus workspace by ID
- `focusWorkspaceByName(name)` - Focus workspace by name
- `focusWindow(id)` - Focus specific window
- `closeWindow(id)` - Close specific window
- `closeWindowOrFocused()` - Close focused window

*Signals:*
- `connected()` - Emitted on successful connection
- `disconnected()` - Emitted on disconnection
- `errorOccurred(error)` - Emitted on error
- `rawEventReceived(event)` - Emitted for all IPC events
- `focusedWindowChanged()` - Emitted when focused window changes or its properties update


## Quickshell integration

This project started because I wanted to integrate niri with [Quickshell](https://quickshell.outfoxxed.me/). So here is an example of a simple bar that showcases a niri workspaces switcher and the currently focused window title:

<details>
  <summary>Show</summary>

```qml
import Quickshell
import QtQuick
import Niri 0.1

ShellRoot {
    PanelWindow {
        anchors {
            top: true
            left: true
            right: true
        }
        implicitHeight: 30
        color: "#1C1F22"

        Niri {
            id: niri
            Component.onCompleted: connect()

            onConnected: console.log("Connected to niri")
            onErrorOccurred: function(error) {
                console.error("Niri error:", error)
            }
        }

        // Limit to first 10 workspaces
        Component.onCompleted: niri.workspaces.maxCount = 10

        Row {
            spacing: 10
            anchors {
                left: parent.left
                leftMargin: 5
                verticalCenter: parent.verticalCenter
            }

            Row {
                spacing: 2

                Repeater {
                    model: niri.workspaces

                    Rectangle {
                        width: 30
                        height: 20
                        color: model.isFocused ? "#106DAA" :
                               model.isActive ? "#377B86" : "#222225"
                        border.color: model.isUrgent ? "red" : "#16181A"
                        border.width: 2
                        radius: 3

                        Text {
                            anchors.centerIn: parent
                            text: model.name || model.index
                            font.family: "Barlow Medium"
                            color: model.isFocused || model.isActive ? "white" : "#89919A"
                            font.pixelSize: 14
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: niri.focusWorkspaceById(model.id)
                            cursorShape: Qt.PointingHandCursor
                        }
                    }
                }
            }

            Text {
                text: niri.focusedWindow?.title ?? ""
                font.family: "Barlow Medium"
                font.pixelSize: 16
                color: "#89919A"
            }
        }
    }
}
```
</details>

Save this as a `.qml` file somewhere on your filesystem, and run `quickshell --path /path/to/file.qml` to see it in action.

Assuming you have the [Barlow font](https://tribby.com/fonts/barlow/) installed, it
should look something like this:

![Quickshell simple bar](/assets/quickshell-simple-bar.png)

For more elaborate examples, see my [quickshell-niri](https://github.com/imiric/quickshell-niri) project.


## Troubleshooting

- `module "Niri" is not installed`:
  Ensure `QML_IMPORT_PATH` includes the directory containing the `Niri` directory (not the `Niri` directory itself), or that you copied to plugin to an existing QML import path (e.g. `/usr/lib64/qt6/qml/`).
  
  Also, confirm that you're using Qt 6, and not older versions. You can do this with `qml --version`. If the Qt 6 binary is not on your `$PATH` (e.g. on Void Linux it is at `/usr/lib/qt6/bin/qml`), you can symlink it as `qml6` somewhere on your `$PATH`.

- *Connection failed*:
  Ensure niri is actually running. 😄
  Otherwise, verify that the `NIRI_SOCKET` environment variable is set and points to a valid socket. It should be something like `/run/user/<name>/niri.wayland-1.1856.sock`. Note that this is affected by the value of `XDG_RUNTIME_DIR`.


## License

[MIT](/LICENSE)
