#include <QDebug>
#include <QJsonArray>
#include "overviewmodel.h"
#include "logging.h"

OverviewState::OverviewState(QObject *parent)
    : QObject(parent)
{
}

bool OverviewState::isOpen() const
{
    return m_isOpen;
}

void OverviewState::handleEvent(const QJsonObject &event)
{
    if (event.contains("OverviewOpenedOrClosed")) {
        QJsonObject overviewObj = event["OverviewOpenedOrClosed"].toObject();
        
        if (overviewObj.contains("is_open")) {
            bool open = overviewObj["is_open"].toBool();
            if (m_isOpen != open) {
                m_isOpen = open;
                emit overviewChanged();
            }
        }
    }
}
