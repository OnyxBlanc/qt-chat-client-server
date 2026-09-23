#include "relayserver.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QHostAddress>
#include <QDateTime>

RelayServer::RelayServer(QObject *parent) : QObject(parent), m_udp(this) {
    connect(&m_tcpServer, &QTcpServer::newConnection, this, &RelayServer::onNewConnection);
    connect(&m_udp, &UdpBridge::jsonMessageReceived, this, &RelayServer::onUdpMessage);
    connect(&m_udp, &UdpBridge::logMessage, this, &RelayServer::logMessage);
    connect(&m_udp, &UdpBridge::errorOccurred, this, &RelayServer::logMessage);
}

bool RelayServer::start(quint16 port) { return start(port, port); }

bool RelayServer::start(quint16 tcpPort, quint16 udpPort) {
    if (!m_tcpServer.listen(QHostAddress::AnyIPv4, tcpPort)) {
        emit logMessage(QString("TCP : impossible d'ecouter sur %1 : %2").arg(tcpPort).arg(m_tcpServer.errorString()));
        return false;
    }
    if (!m_udp.start(udpPort)) {
        m_tcpServer.close();
        return false;
    }
    emit logMessage(QString("Relais mixte actif : TCP %1 / UDP %2.").arg(tcpPort).arg(udpPort));
    return true;
}

void RelayServer::onNewConnection() {
    while (m_tcpServer.hasPendingConnections()) {
        QTcpSocket *client = m_tcpServer.nextPendingConnection();
        m_tcpClients << client;
        m_tcpBuffers[client] = QByteArray();
        m_tcpLabels[client] = QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());
        emit logMessage(QString("TCP connexion : %1 (%2 client(s))").arg(m_tcpLabels[client]).arg(m_tcpClients.size()));
        connect(client, &QTcpSocket::readyRead, this, [this, client] { onReadyRead(client); });
        connect(client, &QTcpSocket::disconnected, this, [this, client] { onDisconnected(client); });
    }
}

void RelayServer::onReadyRead(QTcpSocket *client) {
    QByteArray &buffer = m_tcpBuffers[client];
    buffer += client->readAll();
    while (buffer.contains('\n')) {
        const int idx = buffer.indexOf('\n');
        const QByteArray line = buffer.left(idx);
        buffer.remove(0, idx + 1);
        routeMessage(line, client, nullptr);
    }
}

void RelayServer::onDisconnected(QTcpSocket *client) {
    emit logMessage("TCP deconnexion : " + m_tcpLabels.value(client, "inconnu"));
    m_tcpClients.removeAll(client);
    m_tcpBuffers.remove(client);
    m_tcpLabels.remove(client);
    client->deleteLater();
}

void RelayServer::onUdpMessage(const QByteArray &message, const UdpPeer &sender) {
    Q_UNUSED(sender);
    routeMessage(message, nullptr, &sender);
}

void RelayServer::routeMessage(const QByteArray &line, QTcpSocket *tcpSender, const UdpPeer *udpSender) {
    const auto document = QJsonDocument::fromJson(line);
    if (!document.isObject()) return;
    const QByteArray normalized = QJsonDocument(document.object()).toJson(QJsonDocument::Compact);
    logJson(normalized, tcpSender ? "TCP" : "UDP");
    if (tcpSender) {
        broadcastTcp(normalized + '\n', tcpSender);
        m_udp.broadcastFromTcp(normalized);
    } else if (udpSender) {
        broadcastTcp(normalized + '\n');
    }
}

void RelayServer::broadcastTcp(const QByteArray &message, QTcpSocket *exclude) {
    for (QTcpSocket *client : m_tcpClients)
        if (client != exclude) client->write(message);
}

void RelayServer::logJson(const QByteArray &message, const QString &transport) {
    const auto object = QJsonDocument::fromJson(message).object();
    emit logMessage(QString("[%1] %2 %3 - %4 : %5")
                        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
                        .arg(transport)
                        .arg(object["id"].toString())
                        .arg(object["pseudo"].toString())
                        .arg(object["message"].toString()));
}
