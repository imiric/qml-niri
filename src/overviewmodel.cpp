#include "overviewmodel.h"
#include "logging.h"

OverviewState::OverviewState(QObject *parent)
    : QObject(parent), isOpen(false)
{
}

void OverviewState::handleEvent(const QJsonObject &event)
{
    if (event.contains("OverviewOpenedOrClosed")) {
        QJsonObject overviewObj = event["OverviewOpenedOrClosed"].toObject();
        handleOverviewOpenedOrClosed(overviewObj);
    }
}

void OverviewState::handleOverviewOpenedOrClosed(const QJsonObject &obj)
{
    if (obj.contains("is_open")) {
        bool open = obj["is_open"].toBool();
        
        if (isOpen != open) {
            isOpen = open;
            emit isOpenChanged();
        }
    }
}
