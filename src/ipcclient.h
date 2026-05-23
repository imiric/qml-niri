#pragma once

#include <QObject>
#include <QLocalSocket>
#include <QSocketNotifier>
#include <QJsonDocument>

class IPCClient : public QObject
{
    Q_OBJECT

public:
    explicit IPCClient(QObject *parent = nullptr);
    ~IPCClient();

    bool connect(QString *errorOut = nullptr);
    bool isConnected() const;
    bool sendRequest(const QJsonObject &request, QString *errorOut = nullptr);

signals:
    void connected();
    void disconnected();
    // Emitted only for connection-level problems: socket errors, unexpected
    // disconnects, or failures of the initial EventStream subscription.
    // Per-request failures are reported synchronously via sendRequest's return
    // value and errorOut parameter, not through this signal.
    void errorOccurred(const QString &error);
    void eventReceived(const QJsonObject &event);

private slots:
    void onReadyRead();
    void onSocketError();

private:
    QLocalSocket *m_eventSocket = nullptr;
    QLocalSocket *m_requestSocket = nullptr;
    QByteArray m_readBuffer;
    QString m_socketPath;
};
