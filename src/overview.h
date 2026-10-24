#pragma once

#include <QObject>
#include <QJsonObject>
#include <QQmlEngine>

class Niri;

class Overview : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("State is managed by the Niri IPC compositor")
    Q_PROPERTY(bool isOpen READ isOpen NOTIFY isOpenChanged)

public:
    explicit Overview(Niri *niri, QObject *parent = nullptr);

    bool isOpen() const { return m_isOpen; }

    Q_INVOKABLE QVariantMap toggle();

public slots:
    void handleEvent(const QJsonObject &event);

signals:
    void isOpenChanged();

private:
    void handleOverviewOpenedOrClosed(const QJsonObject &obj);

    Niri *m_niri = nullptr;
    bool m_isOpen = false;
};
