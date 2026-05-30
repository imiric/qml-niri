# qml-niri

A QML plugin for interacting with the [niri](https://github.com/niri-wm/niri) Wayland compositor via its IPC protocol.


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
- A recent version of niri (tested with v26.04)


## Disclaimer

The author is an experienced programmer, but not with C++ or Qt. Most of this project was written with the assistance of Large Language Models. That said, **nothing was "vibe-coded", and all code was carefully reviewed and tested**.

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
      url = "github:imiric/qml-niri/main";
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

This section walks through common tasks with runnable examples. For an exhaustive list of every property, method, signal, and model role, see the [API Reference](#api-reference).

### Basic setup

Import the plugin and create a Niri instance:

```qml
import QtQuick
import Niri

Item {
    Niri {
        id: niri
        Component.onCompleted: connect()

        onConnected: console.log("Connected to niri")
        onErrorOccurred: function(error) {
            console.error("Connection error:", error)
        }
    }
}
```

> [!NOTE]
> This requires the `NIRI_SOCKET` environment variable to be set with the path to a
> valid Unix socket.
> See the [niri IPC documentation](https://github.com/niri-wm/niri/wiki/IPC) for details.


### Working with windows

Access window information via the `windows` model. Each delegate exposes the window's
properties as model roles (see [WindowModel roles](#windowmodel-roles)):

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

Focusing and closing:
```qml
niri.focusWindow(windowId)
niri.closeWindow(windowId)
niri.closeWindowOrFocused()         // Close focused window
```


### Working with workspaces

Access workspace information via the `workspaces` model. Each delegate exposes the workspace's properties as model roles (see [WorkspaceModel roles](#workspacemodel-roles)):

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

Focusing:
```qml
niri.focusWorkspace(0)              // By index
niri.focusWorkspaceById(12345)      // By ID
niri.focusWorkspaceByName("code")   // By name
```


#### Limiting workspace count

You can limit the number of workspaces exposed by the model using the `maxCount` property. Only the first `maxCount` workspaces (after sorting) will be available:

```qml
Component.onCompleted: {
    niri.workspaces.maxCount = 11
}
```

This is useful for widget layouts that should only show a fixed number of workspaces.


#### Querying the workspace model

The `WorkspaceModel` provides two helper methods for looking up workspaces by their
visible row or ID:

```qml
// Get a map of all role values for the workspace at visible row 0
const ws = niri.workspaces.get(0)
console.log(ws.name, ws.isFocused)

// Find the visible row index for a workspace ID (-1 if hidden or not found)
const row = niri.workspaces.indexOfId(12345)
```

Both account for `maxCount`: rows hidden by the limit are treated as out of range.


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


### Action results and error handling

All action methods return a result object describing the outcome:

```qml
const result = niri.focusWorkspace(1)
if (!result.ok) {
    console.error("Failed to focus workspace:", result.error)
}
```

The result object has the shape:
- `ok: bool` - `true` on success, `false` on failure
- `error: string` - Error message (only present when `ok` is `false`)

Failures include "not connected", IPC write/read errors, and action rejections from niri itself. Callers that don't care about the result can simply ignore the return value.

Note that per-action failures are **not** reported via the `errorOccurred` signal. That signal is reserved for connection-level problems such as socket disconnects or event stream subscription failures.


### Escape hatch: sendRawAction

For actions not covered by the typed wrappers, `sendRawAction` lets you send an arbitrary niri [`Action`](https://docs.rs/niri-ipc/latest/niri_ipc/enum.Action.html) as a JSON-shaped object:

```qml
const result = niri.sendRawAction({
    "FocusWorkspace": { "reference": { "Index": 2 } }
})
if (!result.ok) {
    console.error(result.error)
}
```

Prefer the typed wrappers when available; `sendRawAction` performs no schema validation and will only report failures returned by niri itself.


### Application icons

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


### Logging

The plugin will output informational and error messages by default. To enable verbose logging for troubleshooting, set the `QT_LOGGING_RULES` environment variable. E.g.:
```shell
export QT_LOGGING_RULES="niri.debug=true"
```

This will also show debug and warning messages for icon lookup, IPC communication, and event handling.


## Examples

### Quickshell bar

This project started because I wanted to integrate niri with [Quickshell](https://quickshell.outfoxxed.me/). So here is an example of a simple bar that showcases a niri workspaces switcher and the currently focused window title:

<details>
  <summary>Show</summary>

```qml
import Quickshell
import QtQuick
import Niri

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


## Testing

The plugin is mostly tested manually, using a few [integration tests](/test). You can run them with:

```bash
# Test event stream
just test events

# Test workspace model
just test workspaces

# Test window model
just test windows

# Test arbitrary actions (sendRawAction)
just test action
```

These test files are also useful examples of common patterns (e.g. sorting and filtering with `SortFilterProxyModel`).

Pull requests to improve the testing situation, add unit tests, CI, etc., are very welcome!


## API Reference

The plugin exposes four QML types: `Niri` (the main entry point), `WorkspaceModel` and `WindowModel` (list models for the `workspaces` and `windows` properties), and `Window` (an individual window object). Only `Niri` is directly instantiable; the others are obtained via `Niri`'s properties.

### Niri

The main object. Connect to niri and issue actions through it.

*Properties:*
- `workspaces`: [WorkspaceModel](#workspacemodel) - List of all workspaces
- `windows`: [WindowModel](#windowmodel) - List of all windows
- `focusedWindow`: [Window](#window) - Currently focused window (`null` if none)

*Methods:*
- `connect()`: bool - Connect to the niri IPC socket. Returns `true` on success.
- `isConnected()`: bool - Check connection status
- `focusWorkspace(index)`: object - Focus workspace by index
- `focusWorkspaceById(id)`: object - Focus workspace by ID
- `focusWorkspaceByName(name)`: object - Focus workspace by name
- `focusWindow(id)`: object - Focus specific window
- `closeWindow(id)`: object - Close specific window
- `closeWindowOrFocused(id = 0)`: object - Close the given window, or the focused window if `id` is `0` (the default)
- `toggleOverview()`: object - Show or hide the workspace overview
- `sendRawAction(action)`: object - Send an arbitrary niri Action

All action methods (everything except `connect()` and `isConnected()`) return a result object of the form `{ ok: bool, error?: string }`. See [Action results and error handling](#action-results-and-error-handling).

*Signals:*
- `connected()` - Emitted on successful connection
- `disconnected()` - Emitted on disconnection
- `errorOccurred(error)` - Emitted on connection-level failures only (socket disconnect, event stream subscription failure)
- `rawEventReceived(event)` - Emitted for all IPC events, with the raw event object
- `focusedWindowChanged()` - Emitted when the focused window changes (including on model resets, focus loss when the focused window closes, and enforcement of the single-focus invariant)


### WorkspaceModel

A `QAbstractListModel` holding all workspaces, sorted by output name and then by index within each output. Accessed via `niri.workspaces`. Use it directly as a `model` for `ListView`/`Repeater`, where each delegate sees the [roles](#workspacemodel-roles) below.

*Properties:*
- `count`: int - Number of visible workspaces (capped by `maxCount`)
- `maxCount`: int - Maximum number of workspaces to expose (default: unlimited; shows all workspaces). Only the first `maxCount` workspaces, after sorting, are visible.

*Methods:*
- `get(row)`: object - Returns a map of role-name → value for the workspace at the given visible row, or an empty map if `row` is out of range (`row < 0` or `row >= count`)
- `indexOfId(id)`: int - Returns the visible row index for the workspace with the given ID, or `-1` if no such workspace exists or it is hidden by `maxCount`

*Signals:*
- `countChanged()` - Emitted when the number of visible workspaces changes
- `maxCountChanged()` - Emitted when `maxCount` changes

#### WorkspaceModel roles

Available to delegates when using `WorkspaceModel` as a view model:
- `id`: Unique workspace identifier
- `index`: Workspace position on its output
- `name`: Optional workspace name
- `output`: Output device name
- `isActive`: Currently active on its output
- `isFocused`: Currently focused workspace
- `isUrgent`: Has windows requesting attention
- `activeWindowId`: ID of the active window (`0` if none)


### WindowModel

A `QAbstractListModel` holding all windows, sorted by window ID. Accessed via `niri.windows`. Use it directly as a `model` for `ListView`/`Repeater`, where each delegate sees the [roles](#windowmodel-roles) below.

*Properties:*
- `count`: int - Number of windows
- `focusedWindow`: [Window](#window) - Currently focused window (`null` if none)

*Signals:*
- `countChanged()` - Emitted when the number of windows changes
- `focusedWindowChanged()` - Emitted when the focused window changes

#### WindowModel roles

Available to delegates when using `WindowModel` as a view model. These mirror the [Window](#window) properties:
- `id`: Unique window identifier
- `title`: Window title
- `appId`: Application identifier
- `pid`: Process ID (`-1` if unavailable)
- `workspaceId`: Current workspace ID
- `isFocused`: Currently focused window
- `isFloating`: Floating window state
- `isUrgent`: Window urgency flag
- `columnIndex`: Tiled window column index in niri's scrolling layout (1-based, `0` if unavailable)
- `tileIndex`: Tiled window index within its column (1-based, `0` if unavailable)
- `tileWidth`: Tile width in logical pixels (includes niri decorations like borders)
- `tileHeight`: Tile height in logical pixels (includes niri decorations like borders)
- `windowWidth`: Window visual geometry width in logical pixels (without niri decorations)
- `windowHeight`: Window visual geometry height in logical pixels (without niri decorations)
- `tilePosX`: Tile X position in current workspace view (`NaN` if unavailable)
- `tilePosY`: Tile Y position in current workspace view (`NaN` if unavailable)
- `windowOffsetX`: Window visual geometry X offset inside its tile
- `windowOffsetY`: Window visual geometry Y offset inside its tile
- `iconPath`: Absolute path to application icon (empty if not found)


### Window

An individual window object, owned by `WindowModel`. Not instantiable from QML; obtained via `niri.focusedWindow` or `niri.windows.focusedWindow`. Its properties match the [WindowModel roles](#windowmodel-roles).

*Properties:*
- `id`: quint64 - Unique window identifier (constant)
- `title`: string - Window title
- `appId`: string - Application identifier (constant)
- `pid`: int - Process ID, `-1` if unavailable (constant)
- `workspaceId`: quint64 - Current workspace ID
- `isFocused`: bool - Currently focused window
- `isFloating`: bool - Floating window state
- `isUrgent`: bool - Window urgency flag
- `columnIndex`: int - Tiled window column index (1-based, `0` if unavailable)
- `tileIndex`: int - Tiled window index within its column (1-based, `0` if unavailable)
- `tileWidth`: real - Tile width in logical pixels (includes niri decorations)
- `tileHeight`: real - Tile height in logical pixels (includes niri decorations)
- `windowWidth`: int - Window visual geometry width in logical pixels (without decorations)
- `windowHeight`: int - Window visual geometry height in logical pixels (without decorations)
- `tilePosX`: real - Tile X position in current workspace view (`NaN` if unavailable)
- `tilePosY`: real - Tile Y position in current workspace view (`NaN` if unavailable)
- `windowOffsetX`: real - Window visual geometry X offset inside its tile
- `windowOffsetY`: real - Window visual geometry Y offset inside its tile
- `iconPath`: string - Absolute path to application icon, empty if not found (constant)

*Signals:*
- `titleChanged()` - Emitted when the title changes
- `workspaceIdChanged()` - Emitted when the window moves to a different workspace
- `isFocusedChanged()` - Emitted when focus state changes
- `isFloatingChanged()` - Emitted when floating state changes
- `isUrgentChanged()` - Emitted when urgency changes
- `layoutChanged()` - Emitted when any layout-related property (column/tile indices, sizes, positions, offsets) changes

Properties marked *(constant)* never change for the lifetime of the window object
and have no corresponding signal.


## Troubleshooting

- `module "Niri" is not installed`:
  Ensure `QML_IMPORT_PATH` includes the directory containing the `Niri` directory (not the `Niri` directory itself), or that you copied to plugin to an existing QML import path (e.g. `/usr/lib64/qt6/qml/`).

  Also, confirm that you're using Qt 6, and not older versions. You can do this with `qml --version`. If the Qt 6 binary is not on your `$PATH` (e.g. on Void Linux it is at `/usr/lib/qt6/bin/qml`), you can symlink it as `qml6` somewhere on your `$PATH`.

- *Connection failed*:
  Ensure niri is actually running. 😄
  Otherwise, verify that the `NIRI_SOCKET` environment variable is set and points to a valid socket. It should be something like `/run/user/<name>/niri.wayland-1.1856.sock`. Note that this is affected by the value of `XDG_RUNTIME_DIR`.


## License

[MIT](/LICENSE)
