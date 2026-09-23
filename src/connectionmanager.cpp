#include "connectionmanager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QHostAddress>

ConnectionManager::ConnectionManager(QObject *parent) : QObject(parent) {
    connect(&m_server, &QTcpServer::newConnection, this, [this] {
        while (m_server.hasPendingConnections()) {
            QTcpSocket *client = m_server.nextPendingConnection();
            m_clients << client;
            m_clientInfo[client] = ClientInfo{};
            m_serverBuffers[client] = QByteArray();

            connect(client, &QTcpSocket::readyRead, this, [this, client] {
                QByteArray &buffer = m_serverBuffers[client];
                buffer += client->readAll();
                while (buffer.contains('\n')) {
                    const int idx = buffer.indexOf('\n');
                    const QByteArray line = buffer.left(idx);
                    buffer.remove(0, idx + 1);
                    handleAcceptedLine(line, client);
                }
            });

            connect(client, &QTcpSocket::disconnected, this, [this, client] {
                const auto info = m_clientInfo.value(client);
                if (info.identified)
                    emit systemMessage(info.id + " - " + info.pseudo + " a quitte le salon.");
                m_clients.removeAll(client);
                m_serverBuffers.remove(client);
                m_clientInfo.remove(client);
                client->deleteLater();
                emit participantsChanged();
            });

            emit participantsChanged();
        }
    });
}

QString ConnectionManager::localIPv4() {
    for (const auto &address : QNetworkInterface::allAddresses())
        if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback())
            return address.toString();
    return QStringLiteral("127.0.0.1");
}

bool ConnectionManager::startHosting(quint16 port) {
    if (m_server.isListening()) m_server.close();

    if (!m_server.listen(QHostAddress::Any, port)) {
        emit errorOccurred(m_server.errorString());
        emit hostingStateChanged(false);
        return false;
    }

    emit hostingStateChanged(true);
    emit systemMessage("Salon ouvert sur le port " + QString::number(port) + ".");
    emit participantsChanged();
    return true;
}

void ConnectionManager::stopHosting() {
    if (!m_server.isListening()) return;
    m_server.close();
    for (auto *client : m_clients) client->deleteLater();
    m_clients.clear();
    m_serverBuffers.clear();
    m_clientInfo.clear();
    emit hostingStateChanged(false);
    emit participantsChanged();
}

bool ConnectionManager::isHosting() const {
    return m_server.isListening();
}

bool ConnectionManager::isConnectedToRemote() const {
    return m_remoteSocket && m_remoteSocket->state() == QAbstractSocket::ConnectedState;
}

void ConnectionManager::joinRemote(const QString &host, quint16 port) {
    if (m_remoteSocket) {
        m_remoteSocket->disconnectFromHost();
        m_remoteSocket->deleteLater();
        m_remoteSocket = nullptr;
        m_remoteBuffer.clear();
    }

    m_remoteSocket = new QTcpSocket(this);

    connect(m_remoteSocket, &QTcpSocket::connected, this, [this, host, port] {
        emit systemMessage("Connecte au salon distant " + host + ":" + QString::number(port) + ".");
        emit remoteConnected();
    });

    connect(m_remoteSocket, &QTcpSocket::disconnected, this, [this] {
        emit systemMessage("Deconnecte du salon distant.");
        emit remoteDisconnected();
    });

    connect(m_remoteSocket, &QTcpSocket::readyRead, this, [this] {
        m_remoteBuffer += m_remoteSocket->readAll();
        while (m_remoteBuffer.contains('\n')) {
            const int idx = m_remoteBuffer.indexOf('\n');
            const QByteArray line = m_remoteBuffer.left(idx);
            m_remoteBuffer.remove(0, idx + 1);

            const auto doc = QJsonDocument::fromJson(line);
            if (doc.isObject()) {
                const auto o = doc.object();
                emit messageReceived(o["id"].toString(), o["pseudo"].toString(), o["message"].toString());
            }
        }
    });

    connect(m_remoteSocket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit errorOccurred(m_remoteSocket->errorString());
    });

    m_remoteSocket->connectToHost(host, port);
}

void ConnectionManager::sendMessage(const QString &id, const QString &pseudo, const QString &message) {
    const QByteArray line = frame(id, pseudo, message);

    if (isHosting()) {
        broadcastToClients(line);
    } else if (isConnectedToRemote()) {
        m_remoteSocket->write(line);
    }
}

QStringList ConnectionManager::participantLabels() const {
    QStringList labels;
    for (auto *client : m_clients) {
        const auto info = m_clientInfo.value(client);
        labels << (info.identified ? (info.id + " - " + info.pseudo) : QStringLiteral("Connexion en cours..."));
    }
    return labels;
}

QByteArray ConnectionManager::frame(const QString &id, const QString &pseudo, const QString &message) const {
    const QJsonObject o{{"id", id}, {"pseudo", pseudo}, {"message", message}};
    return QJsonDocument(o).toJson(QJsonDocument::Compact) + '\n';
}

void ConnectionManager::broadcastToClients(const QByteArray &line, QTcpSocket *exclude) {
    for (auto *client : m_clients)
        if (client != exclude) client->write(line);
}

void ConnectionManager::handleAcceptedLine(const QByteArray &line, QTcpSocket *from) {
    const auto doc = QJsonDocument::fromJson(line);
    if (!doc.isObject()) return;
    const auto o = doc.object();

    auto &info = m_clientInfo[from];
    info.id = o["id"].toString();
    info.pseudo = o["pseudo"].toString();
    info.identified = true;

    emit messageReceived(info.id, info.pseudo, o["message"].toString());
    broadcastToClients(line, from);
    emit participantsChanged();
}
