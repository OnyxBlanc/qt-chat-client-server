#include "udpbridge.h"

#include <QJsonDocument>
#include <QJsonObject>

UdpBridge::UdpBridge(QObject *parent) : QObject(parent) {
    connect(&m_socket, &QUdpSocket::readyRead, this, &UdpBridge::readPendingDatagrams);
}

bool UdpBridge::start(quint16 port) {
    if (!m_socket.bind(QHostAddress::AnyIPv4, port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        emit errorOccurred(m_socket.errorString());
        return false;
    }
    emit logMessage(QString("UDP actif sur le port %1.").arg(port));
    return true;
}

quint16 UdpBridge::port() const { return m_socket.localPort(); }

void UdpBridge::readPendingDatagrams() {
    while (m_socket.hasPendingDatagrams()) {
        const qint64 size = m_socket.pendingDatagramSize();
        if (size <= 0) break;
        QByteArray datagram(static_cast<int>(size), Qt::Uninitialized);
        QHostAddress address;
        quint16 port = 0;
        m_socket.readDatagram(datagram.data(), datagram.size(), &address, &port);
        const auto document = QJsonDocument::fromJson(datagram);
        if (!document.isObject()) {
            emit logMessage(QString("Datagramme UDP ignore : JSON invalide depuis %1:%2").arg(address.toString()).arg(port));
            continue;
        }
        const auto object = document.object();
        if (object.value("type").toString("chat") != "chat") continue;
        const UdpPeer sender{address, port};
        m_peers.insert(sender);
        emit jsonMessageReceived(datagram, sender);
        sendToPeers(datagram, &sender);
    }
}

void UdpBridge::broadcastFromTcp(const QByteArray &message) { sendToPeers(message); }

void UdpBridge::sendToPeers(const QByteArray &message, const UdpPeer *exclude) {
    for (const UdpPeer &peer : m_peers) {
        if (exclude && peer == *exclude) continue;
        m_socket.writeDatagram(message, peer.address, peer.port);
    }
}
