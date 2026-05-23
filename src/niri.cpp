#include <QDebug>
#include <QJsonObject>
#include <QVariantMap>
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
}

Niri::~Niri()
{
}

bool Niri::connect()
{
    QString err;
    if (!m_ipcClient->connect(&err)) {
        emit errorOccurred(err);
        return false;
    }
    return true;
}

bool Niri::isConnected() const
{
    return m_ipcClient->isConnected();
}

QVariantMap Niri::okResult()
{
    return QVariantMap{{"ok", true}};
}

QVariantMap Niri::errResult(const QString &error)
{
    return QVariantMap{{"ok", false}, {"error", error}};
}

QVariantMap Niri::focusWorkspace(int index)
{
    QJsonObject reference;
    reference["Index"] = index;

    QJsonObject action;
    action["FocusWorkspace"] = QJsonObject{{"reference", reference}};

    return sendAction(action);
}

QVariantMap Niri::focusWorkspaceById(quint64 id)
{
    QJsonObject reference;
    reference["Id"] = QJsonValue::fromVariant(id);

    QJsonObject action;
    action["FocusWorkspace"] = QJsonObject{{"reference", reference}};

    return sendAction(action);
}

QVariantMap Niri::focusWorkspaceByName(const QString &name)
{
    QJsonObject reference;
    reference["Name"] = name;

    QJsonObject action;
    action["FocusWorkspace"] = QJsonObject{{"reference", reference}};

    return sendAction(action);
}

QVariantMap Niri::focusWindow(quint64 id)
{
    QJsonObject action;
    action["FocusWindow"] = QJsonObject{{"id", QJsonValue::fromVariant(id)}};

    return sendAction(action);
}

Window* Niri::focusedWindow() const
{
    return m_windowModel->focusedWindow();
}

QVariantMap Niri::closeWindow(quint64 id)
{
    QJsonObject action;
    action["CloseWindow"] = QJsonObject{{"id", QJsonValue::fromVariant(id)}};

    return sendAction(action);
}

QVariantMap Niri::closeWindowOrFocused(quint64 id)
{
    QJsonObject action;
    if (id == 0) {
        action["CloseWindow"] = QJsonObject{{"id", QJsonValue()}};
    } else {
        action["CloseWindow"] = QJsonObject{{"id", QJsonValue::fromVariant(id)}};
    }

    return sendAction(action);
}

QVariantMap Niri::toggleOverview()
{
    QJsonObject action;
    action["ToggleOverview"] = QJsonObject{};

    return sendAction(action);
}

QVariantMap Niri::sendAction(const QJsonObject &action)
{
    if (!isConnected()) {
        return errResult(QStringLiteral("Cannot send action: not connected to niri"));
    }

    QJsonObject request;
    request["Action"] = action;

    QString err;
    if (!m_ipcClient->sendRequest(request, &err)) {
        return errResult(err.isEmpty() ? QStringLiteral("Unknown IPC error") : err);
    }

    return okResult();
}
