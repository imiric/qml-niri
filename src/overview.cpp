#include "overview.h"
#include "niri.h"
#include "logging.h"

Overview::Overview(Niri *niri, QObject *parent) : QObject(parent), m_niri(niri) { }

QVariantMap Overview::toggle()
{
    QJsonObject action;
    action["ToggleOverview"] = QJsonObject{ };

    return m_niri->sendAction(action);
}

void Overview::handleEvent(const QJsonObject &event)
{
    if (event.contains("OverviewOpenedOrClosed")) {
        QJsonObject overviewObj = event["OverviewOpenedOrClosed"].toObject();
        handleOverviewOpenedOrClosed(overviewObj);
    }
}

void Overview::handleOverviewOpenedOrClosed(const QJsonObject &obj)
{
    if (obj.contains("is_open")) {
        bool open = obj["is_open"].toBool();

        if (m_isOpen != open) {
            m_isOpen = open;
            emit isOpenChanged();
        }
    }
}
