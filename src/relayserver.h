#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHash>
#include <QByteArray>
#include <QString>

// Coeur du serveur relais : accepte plusieurs clients et retransmet
// chaque message JSON recu (une ligne = un message) a tous les autres
// clients connectes. Utilise le meme protocole que ConnectionManager
// (src/connectionmanager.h), donc compatible avec QtChatApp.
class RelayServer : public QObject {
    Q_OBJECT
public:
    explicit RelayServer(QObject *parent = nullptr);

    bool start(quint16 port);

signals:
    void logMessage(const QString &text);

private slots:
    void onNewConnection();

private:
    void onReadyRead(QTcpSocket *client);
    void onDisconnected(QTcpSocket *client);
    void relay(const QByteArray &line, QTcpSocket *from);

    QTcpServer m_server;
    QList<QTcpSocket *> m_clients;
    QHash<QTcpSocket *, QByteArray> m_buffers;
    QHash<QTcpSocket *, QString> m_labels;
};
