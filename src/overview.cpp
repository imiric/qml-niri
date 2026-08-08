#include "overview.h"
#include "logging.h"

Overview::Overview(QObject *parent) : QObject(parent) { }

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
