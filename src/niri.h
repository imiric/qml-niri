#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QVariantMap>
#include "ipcclient.h"
#include "workspacemodel.h"
#include "windowmodel.h"

class Niri : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(WorkspaceModel* workspaces READ workspaces CONSTANT)
    Q_PROPERTY(WindowModel* windows READ windows CONSTANT)
    Q_PROPERTY(Window* focusedWindow READ focusedWindow NOTIFY focusedWindowChanged)

public:
    explicit Niri(QObject *parent = nullptr);
    ~Niri();

    WorkspaceModel* workspaces() const { return m_workspaceModel; }
    WindowModel* windows() const { return m_windowModel; }
    Window* focusedWindow() const;

    Q_INVOKABLE bool connect();
    Q_INVOKABLE bool isConnected() const;

    // Action methods.
    //
    // All action methods return a QVariantMap result of the form:
    //   { "ok": true }                              on success
    //   { "ok": false, "error": "<message>" }       on failure
    //
    // Failures include "not connected", IPC write/read errors, and
    // action rejections from niri itself. Callers that don't care about
    // the result can ignore the return value.
    //
    // Note: per-action failures are NOT emitted via errorOccurred.
    // That signal is reserved for connection-level problems.
    Q_INVOKABLE QVariantMap focusWorkspace(int index);
    Q_INVOKABLE QVariantMap focusWorkspaceById(quint64 id);
    Q_INVOKABLE QVariantMap focusWorkspaceByName(const QString &name);

    Q_INVOKABLE QVariantMap focusWindow(quint64 id);
    Q_INVOKABLE QVariantMap closeWindow(quint64 id);
    Q_INVOKABLE QVariantMap closeWindowOrFocused(quint64 id = 0);

    Q_INVOKABLE QVariantMap toggleOverview();

signals:
    void connected();
    void disconnected();
    // Emitted only for connection-level failures (socket disconnect,
    // event stream subscription rejection). Per-action failures are
    // reported via the return value of the action methods.
    void errorOccurred(const QString &error);
    void rawEventReceived(const QJsonObject &event);
    void focusedWindowChanged();

private:
    QVariantMap sendAction(const QJsonObject &action);
    static QVariantMap okResult();
    static QVariantMap errResult(const QString &error);

    IPCClient *m_ipcClient = nullptr;
    WorkspaceModel *m_workspaceModel = nullptr;
    WindowModel *m_windowModel = nullptr;
};
