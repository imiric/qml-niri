#include "keyboardlayouts.h"
#include "logging.h"
#include <QJsonArray>

KeyboardLayouts::KeyboardLayouts(QObject *parent) : QObject(parent) { }

// Get name of the currently active keyboard layout.
QString KeyboardLayouts::currentName() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_names.count())
        return QString();
    return m_names.at(m_currentIndex);
}

// Changing (mb from compositor on config change) or switching handler
void KeyboardLayouts::handleEvent(const QJsonObject &event)
{
    if (event.contains("KeyboardLayoutsChanged")) {
        QJsonObject data =
                event["KeyboardLayoutsChanged"].toObject()["keyboard_layouts"].toObject();
        handleLayoutsChanged(data);
    } else if (event.contains("KeyboardLayoutSwitched")) {
        QJsonObject data = event["KeyboardLayoutSwitched"].toObject();
        handleLayoutSwitched(data["idx"].toInt());
    }
}

// On xkb config change
void KeyboardLayouts::handleLayoutsChanged(const QJsonObject &data)
{
    QStringList names;

    for (const QJsonValue &value : data["names"].toArray())
        names.append(value.toString());

    int idx = data["current_idx"].toInt();

    bool namesEqual = (names == m_names);
    bool idxEqual = (idx == m_currentIndex);

    if (!namesEqual) {
        m_names = names;
        emit namesChanged();
    }

    if (!idxEqual) {
        m_currentIndex = idx;
        emit currentIndexChanged();
    } else if (!namesEqual) {
        emit currentIndexChanged();
    }
}

// On user switching
void KeyboardLayouts::handleLayoutSwitched(int idx)
{
    if (idx == m_currentIndex)
        return;

    if (idx < 0 || idx >= m_names.count()) {
        qCWarning(niriLog) << "KeyboardLayoutSwitched: index out of names range, index:" << idx
                           << "(have" << m_names.count() << "layout names)";
        return;
    }

    m_currentIndex = idx;
    emit currentIndexChanged();
}
