#pragma once

#include <QObject>
#include <QHostAddress>
#include <QSet>
#include <QUdpSocket>

struct UdpPeer {
    QHostAddress address;
    quint16 port = 0;
    bool operator==(const UdpPeer &other) const { return address == other.address && port == other.port; }
};

inline uint qHash(const UdpPeer &peer, uint seed = 0) {
    return qHash(peer.address.toString(), seed) ^ qHash(peer.port, seed << 1);
}

class UdpBridge : public QObject {
    Q_OBJECT
public:
    explicit UdpBridge(QObject *parent = nullptr);
    bool start(quint16 port);
    quint16 port() const;
    void broadcastFromTcp(const QByteArray &message);

signals:
    void jsonMessageReceived(const QByteArray &message, const UdpPeer &sender);
    void logMessage(const QString &text);
    void errorOccurred(const QString &error);

private slots:
    void readPendingDatagrams();

private:
    void sendToPeers(const QByteArray &message, const UdpPeer *exclude = nullptr);
    QUdpSocket m_socket;
    QSet<UdpPeer> m_peers;
};
