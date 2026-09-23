#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QDateTime>
#include <QDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QPushButton>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextEdit>
#include <QVBoxLayout>

// Chat multi-utilisateurs : un serveur TCP local relaie les messages
// a tous les clients connectes sur le meme canal. Chaque client garde
// aussi une connexion sortante s'il rejoint un serveur distant.
class Chat : public QWidget {
    struct ClientInfo { QString id; QString pseudo; bool identified = false; };

    QTcpServer server;
    QList<QTcpSocket*> clients;
    QHash<QTcpSocket*, QByteArray> buffers;
    QHash<QTcpSocket*, ClientInfo> clientInfo;

    QTcpSocket *outgoing = nullptr;
    QByteArray outgoingBuffer;

    QString id, pseudo;
    int port;
    bool automatic;

    QTextEdit *history;
    QLineEdit *input;
    QLabel *ip, *portLabel, *countLabel;
    QListWidget *userList;

public:
    Chat() {
        QSettings s("OnyxBlanc", "QtChatApp");
        id = s.value("id", "PC-001").toString();
        pseudo = s.value("pseudo", "Utilisateur").toString();
        port = s.value("port", 5000).toInt();
        automatic = s.value("automatic", true).toBool();

        setWindowTitle("Qt Chat - Salon multi-utilisateurs");
        resize(1000, 660);
        build();
        applyStyle();

        connect(&server, &QTcpServer::newConnection, this, [this] { acceptClients(); });

        if (automatic) listen();
    }

    QString localIp() {
        for (const auto &a : QNetworkInterface::allAddresses())
            if (a.protocol() == QAbstractSocket::IPv4Protocol && !a.isLoopback())
                return a.toString();
        return "127.0.0.1";
    }

    void build() {
        auto *settings = new QPushButton("⚙ Paramètres");
        connect(settings, &QPushButton::clicked, this, [this] { configure(); });
        auto *title = new QLabel("SALON DE DISCUSSION LOCAL");
        title->setAlignment(Qt::AlignCenter);
        auto *top = new QHBoxLayout;
        top->addWidget(settings);
        top->addWidget(title, 1);

        history = new QTextEdit;
        history->setReadOnly(true);
        history->setPlaceholderText("Les messages du salon apparaîtront ici...");
        input = new QLineEdit;
        input->setPlaceholderText("Écrire un message pour tout le salon...");
        auto *send = new QPushButton("Envoyer  ➜");
        connect(send, &QPushButton::clicked, this, [this] { sendMessage(); });
        connect(input, &QLineEdit::returnPressed, this, [this] { sendMessage(); });
        auto *write = new QHBoxLayout;
        write->addWidget(input);
        write->addWidget(send);

        ip = new QLabel(localIp());
        portLabel = new QLabel;
        auto *copyIp = new QPushButton("Copier IP");
        auto *copyPort = new QPushButton("Copier port");
        connect(copyIp, &QPushButton::clicked, this, [this] { QApplication::clipboard()->setText(ip->text()); });
        connect(copyPort, &QPushButton::clicked, this, [this] { QApplication::clipboard()->setText(QString::number(port)); });
        auto *network = new QGroupBox("CONNEXION LOCALE");
        auto *f = new QFormLayout(network);
        f->addRow("IP locale", ip);
        f->addRow("Port", portLabel);
        f->addRow(copyIp, copyPort);

        auto *left = new QVBoxLayout;
        left->addLayout(top);
        left->addWidget(history, 1);
        left->addLayout(write);
        left->addWidget(network);

        userList = new QListWidget;
        countLabel = new QLabel("0 connecté(s)");
        auto *add = new QPushButton("＋ Rejoindre un autre salon");
        connect(add, &QPushButton::clicked, this, [this] { join(); });
        auto *rightBox = new QGroupBox("PARTICIPANTS DU SALON");
        auto *r = new QVBoxLayout(rightBox);
        r->addWidget(countLabel);
        r->addWidget(userList, 1);
        r->addWidget(add);

        auto *game = new QPushButton("ZONE DE JEUX\nBientôt disponible");
        game->setEnabled(false);

        auto *right = new QVBoxLayout;
        right->addWidget(rightBox, 1);
        right->addWidget(game);

        auto *layout = new QHBoxLayout(this);
        layout->addLayout(left, 3);
        layout->addLayout(right, 1);
        updateStatus();
    }

    void applyStyle() {
        setStyleSheet(
            "QWidget{background:#101427;color:#e8ecff;font:10pt Segoe UI;}"
            "QGroupBox{border:1px solid #424C8F;border-radius:10px;margin-top:12px;padding:10px;}"
            "QGroupBox::title{color:#a1aded;}"
            "QTextEdit,QLineEdit,QListWidget{background:#181f3c;border:1px solid #4b5a9e;border-radius:8px;padding:7px;}"
            "QPushButton{background:#424C8F;border:0;border-radius:8px;padding:9px 12px;font-weight:600;}"
            "QPushButton:hover{background:#5969b4;}"
            "QPushButton:disabled{background:#252b45;color:#69708e;}"
        );
    }

    void updateStatus() {
        portLabel->setText(server.isListening() ? QString::number(port) + " • salon ouvert" : "fermé");
    }

    void log(const QString &who, const QString &text) {
        history->append("[" + QDateTime::currentDateTime().toString("HH:mm") + "] " + who + " : " + text.toHtmlEscaped());
    }

    void refreshUserList() {
        userList->clear();
        userList->addItem("● " + id + " - " + pseudo + " (vous, hôte)");
        for (auto *c : clients) {
            const auto info = clientInfo.value(c);
            if (info.identified)
                userList->addItem("○ " + info.id + " - " + info.pseudo);
            else
                userList->addItem("○ Connexion en cours...");
        }
        countLabel->setText(QString::number(clients.size() + 1) + " connecté(s)");
    }

    QByteArray frame(const QJsonObject &o) {
        return QJsonDocument(o).toJson(QJsonDocument::Compact) + '\n';
    }

    void broadcastToClients(const QByteArray &line, QTcpSocket *exclude = nullptr) {
        for (auto *c : clients)
            if (c != exclude) c->write(line);
    }

    void handleIncomingLine(const QByteArray &line, QTcpSocket *from) {
        const auto doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) return;
        const auto o = doc.object();

        auto &info = clientInfo[from];
        info.id = o["id"].toString();
        info.pseudo = o["pseudo"].toString();
        info.identified = true;

        log(info.id + " - " + info.pseudo, o["message"].toString());
        broadcastToClients(line, from);
        refreshUserList();
    }

    void acceptClients() {
        while (server.hasPendingConnections()) {
            QTcpSocket *c = server.nextPendingConnection();
            clients << c;
            clientInfo[c] = ClientInfo{};

            connect(c, &QTcpSocket::readyRead, this, [this, c] {
                auto &b = buffers[c];
                b += c->readAll();
                while (b.contains('\n')) {
                    int n = b.indexOf('\n');
                    const auto line = b.left(n);
                    b.remove(0, n + 1);
                    handleIncomingLine(line, c);
                }
            });

            connect(c, &QTcpSocket::disconnected, this, [this, c] {
                const auto info = clientInfo.value(c);
                if (info.identified) log("SYSTÈME", info.id + " - " + info.pseudo + " a quitté le salon.");
                clients.removeAll(c);
                buffers.remove(c);
                clientInfo.remove(c);
                c->deleteLater();
                refreshUserList();
            });

            refreshUserList();
        }
    }

    void listen() {
        if (server.isListening()) server.close();
        if (server.listen(QHostAddress::Any, port)) {
            log("SYSTÈME", "Salon ouvert sur le port " + QString::number(port) + " (hôte : " + id + " - " + pseudo + ")");
        } else {
            QMessageBox::warning(this, "Port indisponible", server.errorString());
        }
        updateStatus();
        refreshUserList();
    }

    void sendMessage() {
        const auto text = input->text().trimmed();
        if (text.isEmpty()) return;

        const QJsonObject o{{"id", id}, {"pseudo", pseudo}, {"message", text}};
        const auto line = frame(o);

        if (server.isListening()) {
            log(id + " - " + pseudo, text);
            broadcastToClients(line);
        } else if (outgoing && outgoing->state() == QAbstractSocket::ConnectedState) {
            outgoing->write(line);
            log(id + " - " + pseudo, text);
        } else {
            QMessageBox::information(this, "Aucun salon actif", "Héberge un salon (Paramètres) ou rejoins-en un avant d'envoyer un message.");
            return;
        }
        input->clear();
    }

    void join() {
        bool ok;
        const auto host = QInputDialog::getText(this, "Rejoindre un salon", "IP de l'hôte :", QLineEdit::Normal, "127.0.0.1", &ok);
        if (!ok || host.isEmpty()) return;
        const int p = QInputDialog::getInt(this, "Rejoindre un salon", "Port :", port, 1, 65535, 1, &ok);
        if (!ok) return;

        if (server.isListening()) {
            const auto choice = QMessageBox::question(this, "Quitter l'hébergement ?",
                "Tu héberges déjà un salon : le rejoindre un autre va fermer ton salon actuel et déconnecter tout le monde. Continuer ?",
                QMessageBox::Yes | QMessageBox::No);
            if (choice != QMessageBox::Yes) return;
            server.close();
            updateStatus();
        }

        if (outgoing) { outgoing->disconnectFromHost(); outgoing->deleteLater(); }
        outgoing = new QTcpSocket(this);

        connect(outgoing, &QTcpSocket::connected, this, [this, host, p] {
            log("SYSTÈME", "Connecté au salon distant " + host + ":" + QString::number(p));
        });
        connect(outgoing, &QTcpSocket::readyRead, this, [this] {
            outgoingBuffer += outgoing->readAll();
            while (outgoingBuffer.contains('\n')) {
                int n = outgoingBuffer.indexOf('\n');
                const auto line = outgoingBuffer.left(n);
                outgoingBuffer.remove(0, n + 1);
                const auto doc = QJsonDocument::fromJson(line);
                if (doc.isObject()) {
                    const auto o = doc.object();
                    log(o["id"].toString() + " - " + o["pseudo"].toString(), o["message"].toString());
                }
            }
        });
        connect(outgoing, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
            log("SYSTÈME", "Erreur : " + outgoing->errorString());
        });

        outgoing->connectToHost(host, static_cast<quint16>(p));
    }

    void configure() {
        QDialog d(this);
        d.setWindowTitle("Paramètres");
        QLineEdit i(id), p(pseudo);
        QLineEdit po(QString::number(port));
        QCheckBox a("Héberger automatiquement un salon au lancement");
        a.setChecked(automatic);

        auto *f = new QFormLayout(&d);
        f->addRow("ID", &i);
        f->addRow("Pseudo", &p);
        f->addRow("Port d'écoute", &po);
        f->addRow(&a);
        auto *ok = new QPushButton("Enregistrer");
        f->addRow(ok);
        connect(ok, &QPushButton::clicked, &d, &QDialog::accept);

        if (d.exec() == QDialog::Accepted) {
            bool valid;
            int newPort = po.text().toInt(&valid);
            if (!valid || newPort < 1 || newPort > 65535) {
                QMessageBox::warning(this, "Port", "Entre un port entre 1 et 65535.");
                return;
            }
            id = i.text().trimmed();
            pseudo = p.text().trimmed();
            port = newPort;
            automatic = a.isChecked();

            QSettings s("OnyxBlanc", "QtChatApp");
            s.setValue("id", id);
            s.setValue("pseudo", pseudo);
            s.setValue("port", port);
            s.setValue("automatic", automatic);

            if (automatic) listen();
            else { server.close(); updateStatus(); refreshUserList(); }
        }
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    Chat window;
    window.show();
    return app.exec();
}
