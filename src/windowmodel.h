#pragma once

#include <QAbstractListModel>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>

class Window : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(quint64 id MEMBER id CONSTANT)
    Q_PROPERTY(QString title MEMBER title CONSTANT)
    Q_PROPERTY(QString appId MEMBER appId CONSTANT)
    Q_PROPERTY(qint32 pid MEMBER pid CONSTANT)
    Q_PROPERTY(quint64 workspaceId MEMBER workspaceId CONSTANT)
    Q_PROPERTY(bool isFocused MEMBER isFocused CONSTANT)
    Q_PROPERTY(bool isFloating MEMBER isFloating CONSTANT)
    Q_PROPERTY(bool isUrgent MEMBER isUrgent CONSTANT)
    Q_PROPERTY(QString iconPath MEMBER iconPath CONSTANT)
    Q_PROPERTY(QJsonObject layout MEMBER layout CONSTANT)

public:
    explicit Window(QObject *parent = nullptr)
        : QObject(parent), id(0), pid(-1), workspaceId(0),
          isFocused(false), isFloating(false), isUrgent(false),
          layout(QJsonObject()) {}

    quint64 id;
    QString title;
    QString appId;
    qint32 pid;
    quint64 workspaceId;
    bool isFocused;
    bool isFloating;
    bool isUrgent;
    QString iconPath;
    QJsonObject layout;
};

class WindowModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(Window* focusedWindow READ focusedWindow NOTIFY focusedWindowChanged)
    Q_PROPERTY(QJsonObject windowLayouts READ windowLayouts NOTIFY windowLayoutsChanged)

public:
    enum WindowRoles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        AppIdRole,
        PidRole,
        WorkspaceIdRole,
        IsFocusedRole,
        IsFloatingRole,
        IsUrgentRole,
        IconPathRole,
        LayoutRole
    };

    explicit WindowModel(QObject *parent = nullptr);
    ~WindowModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Window* focusedWindow() const { return m_focusedWindow; }
    QJsonObject windowLayouts() const { return m_windowLayouts; }

public slots:
    void handleEvent(const QJsonObject &event);

signals:
    void countChanged();
    void focusedWindowChanged();
    void windowLayoutsChanged();

private:
    void handleWindowsChanged(const QJsonArray &windows);
    void handleWindowOpenedOrChanged(const QJsonObject &window);
    void handleWindowClosed(quint64 id);
    void handleWindowFocusChanged(const QJsonValue &idValue);
    void handleWindowUrgencyChanged(quint64 id, bool urgent);
    void handleWindowLayoutsChanged(const QJsonArray &changes);

    Window* parseWindow(const QJsonObject &obj);
    int findWindowIndex(quint64 id) const;
    void updateFocusedWindow();

    QList<Window*> m_windows;
    Window *m_focusedWindow = nullptr;
    QJsonObject m_windowLayouts;
};
