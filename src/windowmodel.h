#pragma once

#include <QAbstractListModel>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>
#include <limits>

class Window : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Window objects are owned by WindowModel")

    Q_PROPERTY(quint64 id READ id CONSTANT)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString appId READ appId CONSTANT)
    Q_PROPERTY(qint32 pid READ pid CONSTANT)
    Q_PROPERTY(quint64 workspaceId READ workspaceId NOTIFY workspaceIdChanged)
    Q_PROPERTY(bool isFocused READ isFocused NOTIFY isFocusedChanged)
    Q_PROPERTY(bool isFloating READ isFloating NOTIFY isFloatingChanged)
    Q_PROPERTY(bool isUrgent READ isUrgent NOTIFY isUrgentChanged)
    Q_PROPERTY(qint32 columnIndex READ columnIndex NOTIFY layoutChanged)
    Q_PROPERTY(qint32 tileIndex READ tileIndex NOTIFY layoutChanged)
    Q_PROPERTY(qreal tileWidth READ tileWidth NOTIFY layoutChanged)
    Q_PROPERTY(qreal tileHeight READ tileHeight NOTIFY layoutChanged)
    Q_PROPERTY(qint32 windowWidth READ windowWidth NOTIFY layoutChanged)
    Q_PROPERTY(qint32 windowHeight READ windowHeight NOTIFY layoutChanged)
    Q_PROPERTY(qreal tilePosX READ tilePosX NOTIFY layoutChanged)
    Q_PROPERTY(qreal tilePosY READ tilePosY NOTIFY layoutChanged)
    Q_PROPERTY(qreal windowOffsetX READ windowOffsetX NOTIFY layoutChanged)
    Q_PROPERTY(qreal windowOffsetY READ windowOffsetY NOTIFY layoutChanged)
    Q_PROPERTY(QString iconPath READ iconPath CONSTANT)

public:
    explicit Window(QObject *parent = nullptr)
        : QObject(parent),
          m_id(0),
          m_pid(-1),
          m_workspaceId(0),
          m_isFocused(false),
          m_isFloating(false),
          m_isUrgent(false),
          m_columnIndex(0),
          m_tileIndex(0),
          m_tileWidth(0),
          m_tileHeight(0),
          m_windowWidth(0),
          m_windowHeight(0),
          m_tilePosX(std::numeric_limits<qreal>::quiet_NaN()),
          m_tilePosY(std::numeric_limits<qreal>::quiet_NaN()),
          m_windowOffsetX(0),
          m_windowOffsetY(0)
    {
    }

    quint64 id() const { return m_id; }
    QString title() const { return m_title; }
    QString appId() const { return m_appId; }
    qint32 pid() const { return m_pid; }
    quint64 workspaceId() const { return m_workspaceId; }
    bool isFocused() const { return m_isFocused; }
    bool isFloating() const { return m_isFloating; }
    bool isUrgent() const { return m_isUrgent; }
    qint32 columnIndex() const { return m_columnIndex; }
    qint32 tileIndex() const { return m_tileIndex; }
    qreal tileWidth() const { return m_tileWidth; }
    qreal tileHeight() const { return m_tileHeight; }
    qint32 windowWidth() const { return m_windowWidth; }
    qint32 windowHeight() const { return m_windowHeight; }
    qreal tilePosX() const { return m_tilePosX; }
    qreal tilePosY() const { return m_tilePosY; }
    qreal windowOffsetX() const { return m_windowOffsetX; }
    qreal windowOffsetY() const { return m_windowOffsetY; }
    QString iconPath() const { return m_iconPath; }

signals:
    void titleChanged();
    void workspaceIdChanged();
    void isFocusedChanged();
    void isFloatingChanged();
    void isUrgentChanged();
    void layoutChanged();

private:
    friend class WindowModel;

    quint64 m_id;
    QString m_title;
    QString m_appId;
    qint32 m_pid;
    quint64 m_workspaceId;
    bool m_isFocused;
    bool m_isFloating;
    bool m_isUrgent;
    qint32 m_columnIndex;
    qint32 m_tileIndex;
    qreal m_tileWidth;
    qreal m_tileHeight;
    qint32 m_windowWidth;
    qint32 m_windowHeight;
    qreal m_tilePosX;
    qreal m_tilePosY;
    qreal m_windowOffsetX;
    qreal m_windowOffsetY;
    QString m_iconPath;
};

class WindowModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(Window *focusedWindow READ focusedWindow NOTIFY focusedWindowChanged)

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
        ColumnIndexRole,
        TileIndexRole,
        TileWidthRole,
        TileHeightRole,
        WindowWidthRole,
        WindowHeightRole,
        TilePosXRole,
        TilePosYRole,
        WindowOffsetXRole,
        WindowOffsetYRole
    };

    explicit WindowModel(QObject *parent = nullptr);
    ~WindowModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Window *focusedWindow() const;

public slots:
    void handleEvent(const QJsonObject &event);

signals:
    void countChanged();
    void focusedWindowChanged();

private:
    void handleWindowsChanged(const QJsonArray &windows);
    void handleWindowOpenedOrChanged(const QJsonObject &window);
    void handleWindowClosed(quint64 id);
    void handleWindowFocusChanged(const QJsonValue &idValue);
    void handleWindowUrgencyChanged(quint64 id, bool urgent);
    void handleWindowLayoutsChanged(const QJsonArray &changes);
    void parseWindowLayout(Window *window, const QJsonObject &layoutObj);
    QList<int> updateWindow(Window *win, const QJsonObject &obj);
    bool clearOtherFocus(quint64 focusedId);

    Window *parseWindow(const QJsonObject &obj);
    int findWindowIndex(quint64 id) const;

    QList<Window *> m_windows;
};
