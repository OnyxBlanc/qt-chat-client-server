#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>
#include <QByteArray>
#include <QString>
#include <QStringList>

// Gere la partie reseau du salon de discussion :
//  - en mode hote : ecoute sur un port et relaie chaque message
//    recu a tous les autres clients connectes (diffusion multi-clients).
//  - en mode invite : se connecte a un hote (ou a QtChatRelay) distant
//    et envoie/recoit les messages via cette connexion unique.
// Protocole : une ligne JSON par message -> {"id":"...","pseudo":"...","message":"..."}
class ConnectionManager : public QObject {
    Q_OBJECT
public:
    explicit ConnectionManager(QObject *parent = nullptr);

    bool startHosting(quint16 port);
    void stopHosting();
    void joinRemote(const QString &host, quint16 port);

    bool isHosting() const;
    bool isConnectedToRemote() const;

    void sendMessage(const QString &id, const QString &pseudo, const QString &message);
    QStringList participantLabels() const;

    static QString localIPv4();

signals:
    void messageReceived(const QString &id, const QString &pseudo, const QString &message);
    void systemMessage(const QString &text);
    void participantsChanged();
    void hostingStateChanged(bool hosting);
    void remoteConnected();
    void remoteDisconnected();
    void errorOccurred(const QString &error);

private:
    struct ClientInfo {
        QString id;
        QString pseudo;
        bool identified = false;
    };

    void handleAcceptedLine(const QByteArray &line, QTcpSocket *from);
    void broadcastToClients(const QByteArray &line, QTcpSocket *exclude = nullptr);
    QByteArray frame(const QString &id, const QString &pseudo, const QString &message) const;

    QTcpServer m_server;
    QList<QTcpSocket *> m_clients;
    QHash<QTcpSocket *, QByteArray> m_serverBuffers;
    QHash<QTcpSocket *, ClientInfo> m_clientInfo;

    QTcpSocket *m_remoteSocket = nullptr;
    QByteArray m_remoteBuffer;
};
