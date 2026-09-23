#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>
#include <QByteArray>
#include <QString>
#include "udpbridge.h"

class RelayServer : public QObject {
    Q_OBJECT
public:
    explicit RelayServer(QObject *parent = nullptr);
    bool start(quint16 tcpPort, quint16 udpPort);
    bool start(quint16 port);

signals:
    void logMessage(const QString &text);

private slots:
    void onNewConnection();
    void onUdpMessage(const QByteArray &message, const UdpPeer &sender);

private:
    void onReadyRead(QTcpSocket *client);
    void onDisconnected(QTcpSocket *client);
    void routeMessage(const QByteArray &line, QTcpSocket *tcpSender = nullptr, const UdpPeer *udpSender = nullptr);
    void broadcastTcp(const QByteArray &message, QTcpSocket *exclude = nullptr);
    void logJson(const QByteArray &message, const QString &transport);

    QTcpServer m_tcpServer;
    QList<QTcpSocket *> m_tcpClients;
    QHash<QTcpSocket *, QByteArray> m_tcpBuffers;
    QHash<QTcpSocket *, QString> m_tcpLabels;
    UdpBridge m_udp;
};
