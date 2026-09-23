#include "relayserver.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

RelayServer::RelayServer(QObject *parent) : QObject(parent) {
    connect(&m_server, &QTcpServer::newConnection, this, &RelayServer::onNewConnection);
}

bool RelayServer::start(quint16 port) {
    if (!m_server.listen(QHostAddress::Any, port)) {
        emit logMessage(QString("Impossible d'ecouter sur le port %1 : %2").arg(port).arg(m_server.errorString()));
        return false;
    }
    emit logMessage(QString("Relais demarre sur le port %1. En attente de connexions...").arg(port));
    return true;
}

void RelayServer::onNewConnection() {
    while (m_server.hasPendingConnections()) {
        QTcpSocket *client = m_server.nextPendingConnection();
        m_clients << client;
        m_buffers[client] = QByteArray();
        m_labels[client] = QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());

        emit logMessage(QString("Nouvelle connexion : %1 (%2 client(s) au total)").arg(m_labels[client]).arg(m_clients.size()));

        connect(client, &QTcpSocket::readyRead, this, [this, client] { onReadyRead(client); });
        connect(client, &QTcpSocket::disconnected, this, [this, client] { onDisconnected(client); });
    }
}

void RelayServer::onReadyRead(QTcpSocket *client) {
    QByteArray &buffer = m_buffers[client];
    buffer += client->readAll();

    while (buffer.contains('\n')) {
        const int idx = buffer.indexOf('\n');
        const QByteArray line = buffer.left(idx);
        buffer.remove(0, idx + 1);
        relay(line, client);
    }
}

void RelayServer::onDisconnected(QTcpSocket *client) {
    emit logMessage(QString("Deconnexion : %1").arg(m_labels.value(client, "inconnu")));
    m_clients.removeAll(client);
    m_buffers.remove(client);
    m_labels.remove(client);
    client->deleteLater();
}

void RelayServer::relay(const QByteArray &line, QTcpSocket *from) {
    const auto doc = QJsonDocument::fromJson(line);
    if (doc.isObject()) {
        const auto o = doc.object();
        emit logMessage(QString("[%1] %2 - %3 : %4")
                             .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
                             .arg(o["id"].toString())
                             .arg(o["pseudo"].toString())
                             .arg(o["message"].toString()));
    }

    for (auto *client : m_clients)
        if (client != from) client->write(line + '\n');
}
