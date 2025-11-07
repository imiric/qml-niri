#pragma once

#include <QObject>
#include <QQmlEngine>
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

    Q_INVOKABLE void focusWorkspace(int index);
    Q_INVOKABLE void focusWorkspaceById(quint64 id);
    Q_INVOKABLE void focusWorkspaceByName(const QString &name);

    Q_INVOKABLE void focusWindow(quint64 id);
    Q_INVOKABLE void closeWindow(quint64 id);
    Q_INVOKABLE void closeWindowOrFocused(quint64 id = 0);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &error);
    void rawEventReceived(const QJsonObject &event);
    void focusedWindowChanged();

private:
    void sendAction(const QJsonObject &action);

    IPCClient *m_ipcClient = nullptr;
    WorkspaceModel *m_workspaceModel = nullptr;
    WindowModel *m_windowModel = nullptr;
};
