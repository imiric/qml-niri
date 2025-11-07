#pragma once

#include <QAbstractListModel>
#include <QJsonObject>
#include <QQmlEngine>

struct Workspace {
    quint64 id;
    quint8 index;
    QString name;
    QString output;
    bool isActive;
    bool isFocused;
    bool isUrgent;
    quint64 activeWindowId;
};

class WorkspaceModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int maxCount READ maxCount WRITE setMaxCount NOTIFY maxCountChanged)

public:
    enum WorkspaceRoles {
        IdRole = Qt::UserRole + 1,
        IndexRole,
        NameRole,
        OutputRole,
        IsActiveRole,
        IsFocusedRole,
        IsUrgentRole,
        ActiveWindowIdRole
    };

    explicit WorkspaceModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    int maxCount() const;
    void setMaxCount(int maxCount);

public slots:
    void handleEvent(const QJsonObject &event);

signals:
    void countChanged();
    void maxCountChanged();

private:
    void handleWorkspacesChanged(const QJsonArray &workspaces);
    void handleWorkspaceActivated(quint64 id, bool focused);
    void handleWorkspaceUrgencyChanged(quint64 id, bool urgent);
    void handleWorkspaceActiveWindowChanged(quint64 workspaceId, const QJsonValue &activeWindowId);

    Workspace parseWorkspace(const QJsonObject &obj);
    int findWorkspaceIndex(quint64 id) const;

    QList<Workspace> m_workspaces;
    int m_maxCount = INT_MAX;
};
