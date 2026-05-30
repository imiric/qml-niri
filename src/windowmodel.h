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

    Q_PROPERTY(quint64 id MEMBER id CONSTANT)
    Q_PROPERTY(QString title MEMBER title NOTIFY titleChanged)
    Q_PROPERTY(QString appId MEMBER appId CONSTANT)
    Q_PROPERTY(qint32 pid MEMBER pid CONSTANT)
    Q_PROPERTY(quint64 workspaceId MEMBER workspaceId NOTIFY workspaceIdChanged)
    Q_PROPERTY(bool isFocused MEMBER isFocused NOTIFY isFocusedChanged)
    Q_PROPERTY(bool isFloating MEMBER isFloating NOTIFY isFloatingChanged)
    Q_PROPERTY(bool isUrgent MEMBER isUrgent NOTIFY isUrgentChanged)
    Q_PROPERTY(qint32 columnIndex MEMBER columnIndex NOTIFY layoutChanged)
    Q_PROPERTY(qint32 tileIndex MEMBER tileIndex NOTIFY layoutChanged)
    Q_PROPERTY(qreal tileWidth MEMBER tileWidth NOTIFY layoutChanged)
    Q_PROPERTY(qreal tileHeight MEMBER tileHeight NOTIFY layoutChanged)
    Q_PROPERTY(qint32 windowWidth MEMBER windowWidth NOTIFY layoutChanged)
    Q_PROPERTY(qint32 windowHeight MEMBER windowHeight NOTIFY layoutChanged)
    Q_PROPERTY(qreal tilePosX MEMBER tilePosX NOTIFY layoutChanged)
    Q_PROPERTY(qreal tilePosY MEMBER tilePosY NOTIFY layoutChanged)
    Q_PROPERTY(qreal windowOffsetX MEMBER windowOffsetX NOTIFY layoutChanged)
    Q_PROPERTY(qreal windowOffsetY MEMBER windowOffsetY NOTIFY layoutChanged)
    Q_PROPERTY(QString iconPath MEMBER iconPath CONSTANT)

public:
    explicit Window(QObject *parent = nullptr)
        : QObject(parent), id(0), pid(-1), workspaceId(0),
          isFocused(false), isFloating(false), isUrgent(false),
          columnIndex(0), tileIndex(0),
          tileWidth(0), tileHeight(0),
          windowWidth(0), windowHeight(0),
          tilePosX(std::numeric_limits<qreal>::quiet_NaN()),
          tilePosY(std::numeric_limits<qreal>::quiet_NaN()),
          windowOffsetX(0), windowOffsetY(0) {}

    quint64 id;
    QString title;
    QString appId;
    qint32 pid;
    quint64 workspaceId;
    bool isFocused;
    bool isFloating;
    bool isUrgent;
    qint32 columnIndex;
    qint32 tileIndex;
    qreal tileWidth;
    qreal tileHeight;
    qint32 windowWidth;
    qint32 windowHeight;
    qreal tilePosX;
    qreal tilePosY;
    qreal windowOffsetX;
    qreal windowOffsetY;
    QString iconPath;

signals:
    void titleChanged();
    void workspaceIdChanged();
    void isFocusedChanged();
    void isFloatingChanged();
    void isUrgentChanged();
    void layoutChanged();
};

class WindowModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(Window* focusedWindow READ focusedWindow NOTIFY focusedWindowChanged)

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

    Window* focusedWindow() const;

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

    Window* parseWindow(const QJsonObject &obj);
    int findWindowIndex(quint64 id) const;

    QList<Window*> m_windows;
};
