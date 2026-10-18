#include <algorithm>
#include <QDebug>
#include <QJsonArray>
#include "castmodel.h"
#include "logging.h"

CastModel::CastModel(QObject *parent) : QAbstractListModel(parent) { }

int CastModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_casts.count();
}

QVariant CastModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_casts.count())
        return QVariant();

    const Cast &cast = m_casts.at(index.row());

    switch (role) {
    case StreamIdRole:
        return QVariant::fromValue(cast.streamId);
    case SessionIdRole:
        return QVariant::fromValue(cast.sessionId);
    case KindRole:
        return cast.kind;
    case TargetTypeRole:
        return cast.targetType;
    case TargetOutputRole:
        return cast.targetOutput;
    case TargetWindowIdRole:
        return QVariant::fromValue(cast.targetWindowId);
    case IsDynamicTargetRole:
        return cast.isDynamicTarget;
    case IsActiveRole:
        return cast.isActive;
    case PidRole:
        return cast.pid;
    case PwNodeIdRole:
        return QVariant::fromValue(cast.pwNodeId);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> CastModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[StreamIdRole] = "streamId";
    roles[SessionIdRole] = "sessionId";
    roles[KindRole] = "kind";
    roles[TargetTypeRole] = "targetType";
    roles[TargetOutputRole] = "targetOutput";
    roles[TargetWindowIdRole] = "targetWindowId";
    roles[IsDynamicTargetRole] = "isDynamicTarget";
    roles[IsActiveRole] = "isActive";
    roles[PidRole] = "pid";
    roles[PwNodeIdRole] = "pwNodeId";
    return roles;
}

void CastModel::handleEvent(const QJsonObject &event)
{
    if (event.contains("CastsChanged")) {
        QJsonArray casts = event["CastsChanged"].toObject()["casts"].toArray();
        handleCastsChanged(casts);
    } else if (event.contains("CastStartedOrChanged")) {
        QJsonObject cast = event["CastStartedOrChanged"].toObject()["cast"].toObject();
        handleCastStartedOrChanged(cast);
    } else if (event.contains("CastStopped")) {
        quint64 streamId = event["CastStopped"].toObject()["stream_id"].toInteger();
        handleCastStopped(streamId);
    }
}

void CastModel::handleCastsChanged(const QJsonArray &casts)
{
    const int oldCount = m_casts.count();

    beginResetModel();
    m_casts.clear();

    for (const QJsonValue &value : casts) {
        if (value.isObject()) {
            m_casts.append(parseCast(value.toObject()));
        }
    }

    // Sort by stream ID for consistent ordering
    std::sort(m_casts.begin(), m_casts.end(),
              [](const Cast &a, const Cast &b) { return a.streamId < b.streamId; });

    endResetModel();

    if (m_casts.count() != oldCount)
        emit countChanged();
}

void CastModel::handleCastStartedOrChanged(const QJsonObject &castObj)
{
    const Cast cast = parseCast(castObj);
    const int idx = findCastIndex(cast.streamId);

    if (idx == -1) {
        // New cast: insert at the position that keeps m_casts sorted by stream ID.
        const auto it = std::lower_bound(
                m_casts.begin(), m_casts.end(), cast.streamId,
                [](const Cast &c, quint64 streamId) { return c.streamId < streamId; });
        const int insertAt = static_cast<int>(it - m_casts.begin());
        beginInsertRows(QModelIndex(), insertAt, insertAt);
        m_casts.insert(insertAt, cast);
        endInsertRows();
        emit countChanged();
    } else if (m_casts[idx] != cast) {
        m_casts[idx] = cast;
        const QModelIndex modelIdx = index(idx);
        emit dataChanged(modelIdx, modelIdx);
    }
}

void CastModel::handleCastStopped(quint64 streamId)
{
    const int idx = findCastIndex(streamId);
    if (idx == -1) {
        qCWarning(niriLog) << "Cast not found for stop:" << streamId;
        return;
    }

    beginRemoveRows(QModelIndex(), idx, idx);
    m_casts.removeAt(idx);
    endRemoveRows();

    emit countChanged();
}

Cast CastModel::parseCast(const QJsonObject &obj)
{
    Cast cast;
    cast.streamId = obj["stream_id"].toInteger();
    cast.sessionId = obj["session_id"].toInteger();
    cast.kind = obj["kind"].toString();
    cast.isDynamicTarget = obj["is_dynamic_target"].toBool();
    cast.isActive = obj["is_active"].toBool();

    const QJsonValue pid = obj["pid"];
    cast.pid = pid.isNull() || pid.isUndefined() ? -1 : pid.toInt();

    const QJsonValue pwNodeId = obj["pw_node_id"];
    cast.pwNodeId = pwNodeId.isNull() || pwNodeId.isUndefined() ? -1 : pwNodeId.toInteger();

    // Target is an externally tagged enum: {"Nothing":{}}, {"Output":{"name":...}}
    // or {"Window":{"id":...}}.
    const QJsonObject target = obj["target"].toObject();
    if (target.contains("Output")) {
        cast.targetType = QStringLiteral("Output");
        cast.targetOutput = target["Output"].toObject()["name"].toString();
    } else if (target.contains("Window")) {
        cast.targetType = QStringLiteral("Window");
        cast.targetWindowId = target["Window"].toObject()["id"].toInteger();
    } else {
        cast.targetType = QStringLiteral("Nothing");
    }

    return cast;
}

int CastModel::findCastIndex(quint64 streamId) const
{
    for (int i = 0; i < m_casts.count(); ++i) {
        if (m_casts[i].streamId == streamId)
            return i;
    }
    return -1;
}
