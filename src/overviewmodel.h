#pragma once

#include <QObject>
#include <QJsonObject>
#include <QQmlEngine>

class OverviewState : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("State is managed by the Niri IPC compositor")
    Q_PROPERTY(bool isOpen MEMBER isOpen NOTIFY isOpenChanged)

public:
    explicit OverviewState(QObject *parent = nullptr);

public slots:
    void handleEvent(const QJsonObject &event);

signals:
    void isOpenChanged();

public:
    bool isOpen;

private:
    void handleOverviewOpenedOrClosed(const QJsonObject &obj);
};
