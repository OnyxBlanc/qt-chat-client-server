#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHash>
#include <QDateTime>
#include <QTextStream>

// Serveur relais sans interface graphique : accepte plusieurs clients
// et retransmet chaque message JSON recu (une ligne = un message)
// a tous les autres clients connectes. Compatible avec le protocole
// utilise par QtChatApp (src/main.cpp).
class Relay : public QObject {
    Q_OBJECT
public:
    explicit Relay(QObject *parent = nullptr) : QObject(parent) {
        connect(&m_server, &QTcpServer::newConnection, this, &Relay::onNewConnection);
    }

    bool start(quint16 port) {
        if (!m_server.listen(QHostAddress::Any, port)) {
            log(QString("Impossible d'ecouter sur le port %1 : %2").arg(port).arg(m_server.errorString()));
            return false;
        }
        log(QString("Relais demarre sur le port %1. En attente de connexions...").arg(port));
        return true;
    }

private slots:
    void onNewConnection() {
        while (m_server.hasPendingConnections()) {
            QTcpSocket *client = m_server.nextPendingConnection();
            m_clients << client;
            m_buffers[client] = QByteArray();
            m_labels[client] = QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());

            log(QString("Nouvelle connexion : %1 (%2 client(s) au total)").arg(m_labels[client]).arg(m_clients.size()));

            connect(client, &QTcpSocket::readyRead, this, [this, client] { onReadyRead(client); });
            connect(client, &QTcpSocket::disconnected, this, [this, client] { onDisconnected(client); });
        }
    }

    void onReadyRead(QTcpSocket *client) {
        QByteArray &buffer = m_buffers[client];
        buffer += client->readAll();

        while (buffer.contains('\n')) {
            int idx = buffer.indexOf('\n');
            const QByteArray line = buffer.left(idx);
            buffer.remove(0, idx + 1);
            relay(line, client);
        }
    }

    void onDisconnected(QTcpSocket *client) {
        log(QString("Deconnexion : %1").arg(m_labels.value(client, "inconnu")));
        m_clients.removeAll(client);
        m_buffers.remove(client);
        m_labels.remove(client);
        client->deleteLater();
    }

private:
    void relay(const QByteArray &line, QTcpSocket *from) {
        const auto doc = QJsonDocument::fromJson(line);
        if (doc.isObject()) {
            const auto o = doc.object();
            log(QString("[%1] %2 - %3 : %4")
                    .arg(QDateTime::currentDateTime().toString("HH:mm:ss"))
                    .arg(o["id"].toString())
                    .arg(o["pseudo"].toString())
                    .arg(o["message"].toString()));
        }

        for (auto *c : m_clients) {
            if (c != from) c->write(line + '\n');
        }
    }

    void log(const QString &text) {
        QTextStream(stdout) << text << Qt::endl;
    }

    QTcpServer m_server;
    QList<QTcpSocket *> m_clients;
    QHash<QTcpSocket *, QByteArray> m_buffers;
    QHash<QTcpSocket *, QString> m_labels;
};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("QtChatRelay");

    QCommandLineParser parser;
    parser.setApplicationDescription("Serveur relais pour QtChatApp : diffuse les messages a tous les clients connectes.");
    parser.addHelpOption();
    QCommandLineOption portOption(QStringList() << "p" << "port", "Port TCP d'ecoute (defaut : 5000).", "port", "5000");
    parser.addOption(portOption);
    parser.process(app);

    bool ok = false;
    const int port = parser.value(portOption).toInt(&ok);
    if (!ok || port < 1 || port > 65535) {
        QTextStream(stderr) << "Port invalide." << Qt::endl;
        return 1;
    }

    Relay relay;
    if (!relay.start(static_cast<quint16>(port))) {
        return 1;
    }

    return app.exec();
}

#include "relay_server.moc"
