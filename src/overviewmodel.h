#pragma once

#include <QObject>
#include <QJsonObject>
#include <QQmlEngine>

class OverviewState : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("State is managed by the Niri IPC compositor")
    Q_PROPERTY(bool isOpen READ isOpen NOTIFY overviewChanged)

public:
    explicit OverviewState(QObject *parent = nullptr);
    bool isOpen() const;

public slots:
    void handleEvent(const QJsonObject &event);

signals:
    void overviewChanged();

private:
    bool m_isOpen = false;
};
