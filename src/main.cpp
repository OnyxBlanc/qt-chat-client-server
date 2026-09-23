#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QColorDialog>
#include <QDateTime>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QPushButton>
#include <QRandomGenerator>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

class ChatWindow final : public QWidget {
public:
    ChatWindow() {
        setWindowTitle("Qt Chat - Client / Serveur");
        resize(920, 620);
        m_id = "PC-" + QString::number(QRandomGenerator::global()->bounded(100, 1000));
        buildUi();
        applyTheme();
    }

private:
    QString m_id;
    QString m_pseudo = "Utilisateur";
    QString m_darkColor = "#424C8F";
    QString m_lightColor = "#A1ADED";
    bool m_dark = true;
    QTcpServer *m_server = nullptr;
    QTcpSocket *m_socket = nullptr;
    QList<QTcpSocket *> m_clients;
    QHash<QTcpSocket *, QByteArray> m_buffers;
    QByteArray m_clientBuffer;
    QTextEdit *m_history = nullptr;
    QLineEdit *m_message = nullptr;
    QListWidget *m_servers = nullptr;
    QLabel *m_ip = nullptr;
    QLabel *m_port = nullptr;
    QCheckBox *m_auto = nullptr;

    static QString localIp() {
        for (const auto &address : QNetworkInterface::allAddresses()) {
            if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback()) return address.toString();
        }
        return "127.0.0.1";
    }

    void buildUi() {
        auto *settings = new QPushButton("⚙ Paramètres");
        connect(settings, &QPushButton::clicked, this, [this] { openSettings(); });
        auto *title = new QLabel("Chat client / serveur local");
        title->setAlignment(Qt::AlignCenter);
        auto *top = new QHBoxLayout;
        top->addWidget(settings);
        top->addWidget(title, 1);

        m_history = new QTextEdit;
        m_history->setReadOnly(true);
        m_history->setPlaceholderText("Historique : ID - Pseudo : message");
        m_message = new QLineEdit;
        m_message->setPlaceholderText("Zone d'écriture...");
        auto *send = new QPushButton("Envoyer ➜");
        connect(send, &QPushButton::clicked, this, [this] { sendMessage(); });
        connect(m_message, &QLineEdit::returnPressed, this, [this] { sendMessage(); });
        auto *write = new QHBoxLayout;
        write->addWidget(m_message);
        write->addWidget(send);

        m_ip = new QLabel(localIp());
        m_port = new QLabel("Aucun serveur actif");
        auto *copyIp = new QPushButton("Copier IP");
        auto *copyPort = new QPushButton("Copier port");
        connect(copyIp, &QPushButton::clicked, this, [this] { QApplication::clipboard()->setText(m_ip->text()); });
        connect(copyPort, &QPushButton::clicked, this, [this] { QApplication::clipboard()->setText(m_port->text()); });
        auto *network = new QGroupBox("Vos coordonnées réseau");
        auto *networkLayout = new QFormLayout(network);
        auto *ipRow = new QHBoxLayout; ipRow->addWidget(m_ip); ipRow->addWidget(copyIp);
        auto *portRow = new QHBoxLayout; portRow->addWidget(m_port); portRow->addWidget(copyPort);
        networkLayout->addRow("Votre IP :", ipRow);
        networkLayout->addRow("Votre port :", portRow);

        auto *left = new QVBoxLayout;
        left->addLayout(top);
        left->addWidget(m_history, 1);
        left->addLayout(write);
        left->addWidget(network);

        m_servers = new QListWidget;
        connect(m_servers, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) { connectEntry(item); });
        auto *add = new QPushButton("+");
        auto *remove = new QPushButton("Retirer");
        connect(add, &QPushButton::clicked, this, [this] { addConnection(); });
        connect(remove, &QPushButton::clicked, this, [this] { delete m_servers->takeItem(m_servers->currentRow()); });
        m_auto = new QCheckBox("Mode automatique");
        connect(m_auto, &QCheckBox::toggled, this, [this](bool enabled) { if (enabled && m_servers->count()) connectEntry(m_servers->item(0)); });
        auto *connections = new QGroupBox("Liste des serveurs");
        auto *connectionsLayout = new QVBoxLayout(connections);
        connectionsLayout->addWidget(m_servers, 1);
        auto *buttons = new QHBoxLayout; buttons->addWidget(add); buttons->addWidget(remove); buttons->addStretch();
        connectionsLayout->addLayout(buttons);
        connectionsLayout->addWidget(m_auto);

        auto *games = new QPushButton("Zone de jeux\n(en construction)");
        games->setEnabled(false);
        games->setMinimumHeight(95);
        auto *right = new QVBoxLayout;
        right->addWidget(connections, 1);
        right->addWidget(games);

        auto *layout = new QHBoxLayout(this);
        layout->addLayout(left, 3);
        layout->addLayout(right, 1);
    }

    void applyTheme() {
        const QString background = m_dark ? m_darkColor : m_lightColor;
        const QString foreground = m_dark ? "#F4F5FF" : "#111426";
        setStyleSheet(QString("QWidget { background:%1; color:%2; } QGroupBox, QTextEdit, QLineEdit, QListWidget { border:1px solid %2; border-radius:5px; } QTextEdit, QLineEdit, QListWidget { background:rgba(255,255,255,30); } QPushButton { border:1px solid %2; border-radius:5px; padding:6px; background:rgba(255,255,255,35); } QPushButton:disabled { color:#777; border-color:#777; }").arg(background, foreground));
    }

    void openSettings() {
        QDialog dialog(this); dialog.setWindowTitle("Paramètres");
        QLineEdit id(m_id), pseudo(m_pseudo), dark(m_darkColor), light(m_lightColor);
        QCheckBox mode("Utiliser le mode foncé"); mode.setChecked(m_dark);
        auto *form = new QFormLayout(&dialog);
        form->addRow("ID :", &id); form->addRow("Pseudo :", &pseudo); form->addRow("Couleur foncée (hex) :", &dark); form->addRow("Couleur claire (hex) :", &light); form->addRow(&mode);
        auto *ok = new QPushButton("Valider"); form->addRow(ok);
        connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);
        if (dialog.exec() == QDialog::Accepted && !id.text().trimmed().isEmpty() && !pseudo.text().trimmed().isEmpty()) {
            m_id = id.text().trimmed(); m_pseudo = pseudo.text().trimmed();
            if (QColor(dark.text()).isValid()) m_darkColor = QColor(dark.text()).name();
            if (QColor(light.text()).isValid()) m_lightColor = QColor(light.text()).name();
            m_dark = mode.isChecked(); applyTheme();
        }
    }

    void log(const QString &id, const QString &pseudo, const QString &message) {
        m_history->append(QString("[%1] %2 - %3 : %4").arg(QDateTime::currentDateTime().toString("HH:mm"), id, pseudo, message.toHtmlEscaped()));
    }

    void handleLine(const QByteArray &line) {
        const QJsonDocument document = QJsonDocument::fromJson(line);
        if (!document.isObject()) return;
        const QJsonObject object = document.object();
        log(object["id"].toString(), object["pseudo"].toString(), object["message"].toString());
    }

    void sendMessage() {
        const QString text = m_message->text().trimmed();
        if (text.isEmpty()) return;
        if (!m_server && !m_socket) { QMessageBox::information(this, "Connexion", "Héberge ou rejoins un serveur avant d'envoyer un message."); return; }
        QJsonObject object{{"id", m_id}, {"pseudo", m_pseudo}, {"message", text}};
        const QByteArray line = QJsonDocument(object).toJson(QJsonDocument::Compact) + '\n';
        if (m_server) { for (auto *client : m_clients) client->write(line); log(m_id, m_pseudo, text); }
        else if (m_socket) m_socket->write(line);
        m_message->clear();
    }

    void startServer(quint16 port) {
        closeConnections();
        m_server = new QTcpServer(this);
        if (!m_server->listen(QHostAddress::Any, port)) { QMessageBox::warning(this, "Serveur", m_server->errorString()); m_server->deleteLater(); m_server = nullptr; return; }
        connect(m_server, &QTcpServer::newConnection, this, [this] {
            while (m_server->hasPendingConnections()) {
                QTcpSocket *client = m_server->nextPendingConnection(); m_clients << client;
                connect(client, &QTcpSocket::readyRead, this, [this, client] {
                    QByteArray &buffer = m_buffers[client]; buffer += client->readAll();
                    while (buffer.contains('\n')) { const int end = buffer.indexOf('\n'); const QByteArray line = buffer.left(end); buffer.remove(0, end + 1); handleLine(line); for (auto *other : m_clients) if (other != client) other->write(line + '\n'); }
                });
                connect(client, &QTcpSocket::disconnected, this, [this, client] { m_clients.removeAll(client); m_buffers.remove(client); client->deleteLater(); });
            }
        });
        m_port->setText(QString::number(port)); log("SYSTÈME", "Serveur", "Serveur local démarré sur le port " + QString::number(port));
    }

    void joinServer(const QString &host, quint16 port) {
        closeConnections(); m_socket = new QTcpSocket(this);
        connect(m_socket, &QTcpSocket::connected, this, [this, host, port] { log("SYSTÈME", "Client", "Connecté à " + host + ":" + QString::number(port)); });
        connect(m_socket, &QTcpSocket::readyRead, this, [this] { m_clientBuffer += m_socket->readAll(); while (m_clientBuffer.contains('\n')) { const int end = m_clientBuffer.indexOf('\n'); const QByteArray line = m_clientBuffer.left(end); m_clientBuffer.remove(0, end + 1); handleLine(line); } });
        connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) { log("SYSTÈME", "Erreur", m_socket->errorString()); });
        m_socket->connectToHost(host, port); m_port->setText(QString::number(port));
    }

    void closeConnections() {
        if (m_server) { m_server->close(); m_server->deleteLater(); m_server = nullptr; }
        for (auto *client : m_clients) { client->disconnectFromHost(); client->deleteLater(); }
        m_clients.clear(); m_buffers.clear();
        if (m_socket) { m_socket->disconnectFromHost(); m_socket->deleteLater(); m_socket = nullptr; }
    }

    void addConnection() {
        bool ok = false;
        const QString role = QInputDialog::getItem(this, "Connexion", "Action :", {"Héberger un serveur", "Rejoindre un serveur"}, 0, false, &ok);
        if (!ok) return;
        const int port = QInputDialog::getInt(this, "Port", "Port TCP :", 5000, 1, 65535, 1, &ok);
        if (!ok) return;
        if (role.startsWith("Héberger")) { startServer(quint16(port)); m_servers->addItem("LOCAL:" + QString::number(port)); }
        else { const QString host = QInputDialog::getText(this, "Serveur distant", "Adresse IP :", QLineEdit::Normal, "127.0.0.1", &ok); if (ok && !host.isEmpty()) { m_servers->addItem(host + ":" + QString::number(port)); joinServer(host, quint16(port)); } }
    }

    void connectEntry(QListWidgetItem *item) {
        const QString entry = item->text(); if (entry.startsWith("LOCAL:")) { startServer(quint16(entry.section(':', 1).toUShort())); return; }
        const int separator = entry.lastIndexOf(':'); if (separator > 0) joinServer(entry.left(separator), quint16(entry.mid(separator + 1).toUShort()));
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ChatWindow window;
    window.show();
    return app.exec();
}
