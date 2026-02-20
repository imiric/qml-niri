#include <algorithm>
#include <QDebug>
#include <QJsonArray>
#include "icon.h"
#include "logging.h"
#include "windowmodel.h"

WindowModel::WindowModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

WindowModel::~WindowModel()
{
    qDeleteAll(m_windows);
}

int WindowModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_windows.count();
}

QVariant WindowModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_windows.count())
        return QVariant();

    const Window *win = m_windows.at(index.row());

    switch (role) {
    case IdRole:
        return QVariant::fromValue(win->id);
    case TitleRole:
        return win->title;
    case AppIdRole:
        return win->appId;
    case PidRole:
        return win->pid;
    case WorkspaceIdRole:
        return QVariant::fromValue(win->workspaceId);
    case IsFocusedRole:
        return win->isFocused;
    case IsFloatingRole:
        return win->isFloating;
    case IsUrgentRole:
        return win->isUrgent;
    case IconPathRole:
        return win->iconPath;
    case LayoutRole:
        return QVariant::fromValue(win->layout);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> WindowModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[TitleRole] = "title";
    roles[AppIdRole] = "appId";
    roles[PidRole] = "pid";
    roles[WorkspaceIdRole] = "workspaceId";
    roles[IsFocusedRole] = "isFocused";
    roles[IsFloatingRole] = "isFloating";
    roles[IsUrgentRole] = "isUrgent";
    roles[IconPathRole] = "iconPath";
    roles[LayoutRole] = "layout";

    return roles;
}

void WindowModel::handleEvent(const QJsonObject &event)
{
    if (event.contains("WindowsChanged")) {
        QJsonArray windows = event["WindowsChanged"].toObject()["windows"].toArray();
        handleWindowsChanged(windows);
    }
    else if (event.contains("WindowOpenedOrChanged")) {
        QJsonObject window = event["WindowOpenedOrChanged"].toObject()["window"].toObject();
        handleWindowOpenedOrChanged(window);
    }
    else if (event.contains("WindowClosed")) {
        quint64 id = event["WindowClosed"].toObject()["id"].toInteger();
        handleWindowClosed(id);
    }
    else if (event.contains("WindowFocusChanged")) {
        QJsonValue id = event["WindowFocusChanged"].toObject()["id"];
        handleWindowFocusChanged(id);
    }
    else if (event.contains("WindowUrgencyChanged")) {
        QJsonObject data = event["WindowUrgencyChanged"].toObject();
        quint64 id = data["id"].toInteger();
        bool urgent = data["urgent"].toBool();
        handleWindowUrgencyChanged(id, urgent);
    }
    else if (event.contains("WindowLayoutsChanged")) {
        QJsonArray changes = event["WindowLayoutsChanged"].toObject()["changes"].toArray();
        handleWindowLayoutsChanged(changes);
    }
}

void WindowModel::handleWindowsChanged(const QJsonArray &windows)
{
    beginResetModel();
    qDeleteAll(m_windows);
    m_windows.clear();

    for (const QJsonValue &value : windows) {
        if (value.isObject()) {
            m_windows.append(parseWindow(value.toObject()));
        }
    }

    // Sort by window ID for consistent ordering
    std::sort(m_windows.begin(), m_windows.end(),
              [](const Window *a, const Window *b) {
                return a->id < b->id;
              });

    endResetModel();
    emit countChanged();
    updateFocusedWindow();
}

void WindowModel::handleWindowOpenedOrChanged(const QJsonObject &windowObj)
{
    Window *window = parseWindow(windowObj);
    int idx = findWindowIndex(window->id);

    if (idx == -1) {
        // New window
        beginInsertRows(QModelIndex(), m_windows.count(), m_windows.count());
        m_windows.append(window);
        endInsertRows();
        emit countChanged();
    } else {
        // Replace existing window pointer. This deallocates and reallocates even for
        // minor changes (e.g. title updates), but avoids property-by-property copying.
        delete m_windows[idx];
        m_windows[idx] = window;
        QModelIndex modelIdx = index(idx);
        emit dataChanged(modelIdx, modelIdx);
    }

    // If this window is focused, update all other windows
    if (window->isFocused) {
        for (int i = 0; i < m_windows.count(); ++i) {
            if (m_windows[i]->id != window->id && m_windows[i]->isFocused) {
                m_windows[i]->isFocused = false;
                QModelIndex modelIdx = index(i);
                emit dataChanged(modelIdx, modelIdx, {IsFocusedRole});
            }
        }
        updateFocusedWindow();
    }
}

void WindowModel::handleWindowClosed(quint64 id)
{
    int idx = findWindowIndex(id);
    if (idx == -1) {
        qCWarning(niriLog) << "Window not found for close:" << id;
        return;
    }

    bool wasFocused = m_windows[idx]->isFocused;

    beginRemoveRows(QModelIndex(), idx, idx);
    delete m_windows.takeAt(idx);
    endRemoveRows();

    emit countChanged();

    if (wasFocused) {
        updateFocusedWindow();
    }
}

void WindowModel::handleWindowFocusChanged(const QJsonValue &idValue)
{
    quint64 newFocusedId = idValue.isNull() ? 0 : idValue.toInteger();

    for (int i = 0; i < m_windows.count(); ++i) {
        bool shouldBeFocused = (m_windows[i]->id == newFocusedId);
        if (m_windows[i]->isFocused != shouldBeFocused) {
            m_windows[i]->isFocused = shouldBeFocused;
            QModelIndex modelIdx = index(i);
            emit dataChanged(modelIdx, modelIdx, {IsFocusedRole});
        }
    }

    updateFocusedWindow();
}

void WindowModel::handleWindowUrgencyChanged(quint64 id, bool urgent)
{
    int idx = findWindowIndex(id);
    if (idx == -1) {
        qCWarning(niriLog) << "Window not found for urgency change:" << id;
        return;
    }

    if (m_windows[idx]->isUrgent != urgent) {
        m_windows[idx]->isUrgent = urgent;
        QModelIndex modelIdx = index(idx);
        emit dataChanged(modelIdx, modelIdx, {IsUrgentRole});
    }
}

void WindowModel::handleWindowLayoutsChanged(const QJsonArray &changes)
{
    qCWarning(niriLog) << "handleWindowLayoutsChanged() changes: " << changes;

    for (const QJsonValue &entry : changes) {
        const QJsonArray pair = entry.toArray();
        quint64 idValue = pair[0].toInt();
        QJsonObject layoutObj = pair[1].toObject();

        int idx = findWindowIndex(idValue);
        if (idx == -1) {
            qCWarning(niriLog) << "Window not found for layout change, id:" << idValue;
            return;
        }

        Window *win = m_windows.at(idx);
        win->layout = layoutObj;
    }

    emit windowLayoutsChanged();
}

Window* WindowModel::parseWindow(const QJsonObject &obj)
{
    Window *win = new Window(this);
    win->id = obj["id"].toInteger();
    win->title = obj["title"].toString();
    win->appId = obj["app_id"].toString();

    QJsonValue pidValue = obj["pid"];
    win->pid = pidValue.isNull() ? -1 : pidValue.toInt();

    QJsonValue workspaceIdValue = obj["workspace_id"];
    win->workspaceId = workspaceIdValue.isNull() ? 0 : workspaceIdValue.toInteger();

    win->isFocused = obj["is_focused"].toBool();
    win->isFloating = obj["is_floating"].toBool();
    win->isUrgent = obj["is_urgent"].toBool();
    win->iconPath = IconLookup::lookup(win->appId);
    win->layout = obj["layout"].toObject();

    return win;
}

int WindowModel::findWindowIndex(quint64 id) const
{
    for (int i = 0; i < m_windows.count(); ++i) {
        if (m_windows[i]->id == id)
            return i;
    }
    return -1;
}

void WindowModel::updateFocusedWindow()
{
    Window *newFocused = nullptr;

    for (Window *win : m_windows) {
        if (win->isFocused) {
            newFocused = win;
            break;
        }
    }

    // Emit if focus changed to a different window, or if the focused window's
    // properties changed.
    bool shouldEmit = (m_focusedWindow != newFocused) || (newFocused != nullptr);
    m_focusedWindow = newFocused;

    if (shouldEmit) {
        emit focusedWindowChanged();
    }
}
