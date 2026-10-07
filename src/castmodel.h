#pragma once

#include <QAbstractListModel>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>

// A screencast, as reported by niri's Cast IPC type.
struct Cast
{
    quint64 streamId = 0;
    quint64 sessionId = 0;
    // "PipeWire" or "WlrScreencopy".
    QString kind;
    // "Nothing", "Output" or "Window".
    QString targetType;
    // Set when targetType is "Output".
    QString targetOutput;
    // Set when targetType is "Window"; 0 otherwise.
    quint64 targetWindowId = 0;
    bool isDynamicTarget = false;
    bool isActive = false;
    // -1 when unknown.
    qint32 pid = -1;
    // -1 when unknown.
    qint64 pwNodeId = -1;

    bool operator==(const Cast &other) const;
    bool operator!=(const Cast &other) const { return !(*this == other); }
};

class CastModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool anyActive READ anyActive NOTIFY anyActiveChanged)

public:
    enum CastRoles {
        StreamIdRole = Qt::UserRole + 1,
        SessionIdRole,
        KindRole,
        TargetTypeRole,
        TargetOutputRole,
        TargetWindowIdRole,
        IsDynamicTargetRole,
        IsActiveRole,
        PidRole,
        PwNodeIdRole
    };

    explicit CastModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool anyActive() const { return m_anyActive; }

public slots:
    void handleEvent(const QJsonObject &event);

signals:
    void countChanged();
    void anyActiveChanged();

private:
    void handleCastsChanged(const QJsonArray &casts);
    void handleCastStartedOrChanged(const QJsonObject &castObj);
    void handleCastStopped(quint64 streamId);
    void updateAnyActive();

    static Cast parseCast(const QJsonObject &obj);
    int findCastIndex(quint64 streamId) const;

    QList<Cast> m_casts;
    bool m_anyActive = false;
};
