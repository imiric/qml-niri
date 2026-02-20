#include <QDebug>
#include <QJsonObject>
#include "niri.h"
#include "logging.h"

Niri::Niri(QObject *parent)
    : QObject(parent)
    , m_ipcClient(new IPCClient(this))
    , m_workspaceModel(new WorkspaceModel(this))
    , m_windowModel(new WindowModel(this))
{
    // Wire up IPC client signals
    QObject::connect(m_ipcClient, &IPCClient::connected,
                     this, &Niri::connected);
    QObject::connect(m_ipcClient, &IPCClient::disconnected,
                     this, &Niri::disconnected);
    QObject::connect(m_ipcClient, &IPCClient::errorOccurred,
                     this, &Niri::errorOccurred);
    QObject::connect(m_ipcClient, &IPCClient::eventReceived,
                     this, &Niri::rawEventReceived);

    // Wire events to workspace model
    QObject::connect(m_ipcClient, &IPCClient::eventReceived,
                     m_workspaceModel, &WorkspaceModel::handleEvent);

    // Wire events to window model
    QObject::connect(m_ipcClient, &IPCClient::eventReceived,
                     m_windowModel, &WindowModel::handleEvent);

    // Forward focused window changes
    QObject::connect(m_windowModel, &WindowModel::focusedWindowChanged,
                     this, &Niri::focusedWindowChanged);

    // Forward window layouts changes
    QObject::connect(m_windowModel, &WindowModel::windowLayoutsChanged,
                     this, &Niri::windowLayoutsChanged);
}

Niri::~Niri()
{
}

bool Niri::connect()
{
    return m_ipcClient->connect();
}

bool Niri::isConnected() const
{
    return m_ipcClient->isConnected();
}

void Niri::focusWorkspace(int index)
{
    QJsonObject reference;
    reference["Index"] = index;

    QJsonObject action;
    action["FocusWorkspace"] = QJsonObject{{"reference", reference}};

    sendAction(action);
}

void Niri::focusWorkspaceById(quint64 id)
{
    QJsonObject reference;
    reference["Id"] = QJsonValue::fromVariant(id);

    QJsonObject action;
    action["FocusWorkspace"] = QJsonObject{{"reference", reference}};

    sendAction(action);
}

void Niri::focusWorkspaceByName(const QString &name)
{
    QJsonObject reference;
    reference["Name"] = name;

    QJsonObject action;
    action["FocusWorkspace"] = QJsonObject{{"reference", reference}};

    sendAction(action);
}

void Niri::focusWindow(quint64 id)
{
    QJsonObject action;
    action["FocusWindow"] = QJsonObject{{"id", QJsonValue::fromVariant(id)}};

    sendAction(action);
}

Window* Niri::focusedWindow() const
{
    return m_windowModel->focusedWindow();
}

void Niri::closeWindow(quint64 id)
{
    QJsonObject action;
    action["CloseWindow"] = QJsonObject{{"id", QJsonValue::fromVariant(id)}};

    sendAction(action);
}

void Niri::closeWindowOrFocused(quint64 id)
{
    QJsonObject action;
    if (id == 0) {
        action["CloseWindow"] = QJsonObject{{"id", QJsonValue()}};
    } else {
        action["CloseWindow"] = QJsonObject{{"id", QJsonValue::fromVariant(id)}};
    }

    sendAction(action);
}

void Niri::sendAction(const QJsonObject &action)
{
    if (!isConnected()) {
        qCWarning(niriLog) << "Cannot send action: not connected to niri";
        return;
    }

    QJsonObject request;
    request["Action"] = action;

    m_ipcClient->sendRequest(request);
}
