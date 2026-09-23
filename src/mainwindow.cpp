#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    loadSettings();
    buildUi();
    applyStyle();

    connect(&m_connection, &ConnectionManager::messageReceived, this, &MainWindow::onMessageReceived);
    connect(&m_connection, &ConnectionManager::systemMessage, this, &MainWindow::onSystemMessage);
    connect(&m_connection, &ConnectionManager::participantsChanged, this, &MainWindow::onParticipantsChanged);
    connect(&m_connection, &ConnectionManager::hostingStateChanged, this, &MainWindow::onHostingStateChanged);
    connect(&m_connection, &ConnectionManager::errorOccurred, this, &MainWindow::onErrorOccurred);
    connect(&m_connection, &ConnectionManager::remoteConnected, this, &MainWindow::updateStatusLabel);
    connect(&m_connection, &ConnectionManager::remoteDisconnected, this, &MainWindow::updateStatusLabel);

    if (m_settings.automatic) {
        m_connection.startHosting(static_cast<quint16>(m_settings.port));
    }
}

void MainWindow::loadSettings() {
    QSettings s("OnyxBlanc", "QtChatApp");
    m_settings.id = s.value("id", "PC-001").toString();
    m_settings.pseudo = s.value("pseudo", "Utilisateur").toString();
    m_settings.port = s.value("port", 5000).toInt();
    m_settings.automatic = s.value("automatic", true).toBool();
}

void MainWindow::saveSettings() {
    QSettings s("OnyxBlanc", "QtChatApp");
    s.setValue("id", m_settings.id);
    s.setValue("pseudo", m_settings.pseudo);
    s.setValue("port", m_settings.port);
    s.setValue("automatic", m_settings.automatic);
}

void MainWindow::buildUi() {
    setWindowTitle("Qt Chat - Salon multi-utilisateurs");
    resize(1000, 660);

    auto *settingsBtn = new QPushButton("⚙ Paramètres", this);
    connect(settingsBtn, &QPushButton::clicked, this, &MainWindow::openSettings);
    auto *title = new QLabel("SALON DE DISCUSSION LOCAL", this);
    title->setAlignment(Qt::AlignCenter);
    auto *top = new QHBoxLayout();
    top->addWidget(settingsBtn);
    top->addWidget(title, 1);

    m_history = new QTextEdit(this);
    m_history->setReadOnly(true);
    m_history->setPlaceholderText("Les messages du salon apparaîtront ici...");

    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("Écrire un message pour tout le salon...");
    auto *sendBtn = new QPushButton("Envoyer  ➜", this);
    connect(sendBtn, &QPushButton::clicked, this, &MainWindow::sendCurrentMessage);
    connect(m_input, &QLineEdit::returnPressed, this, &MainWindow::sendCurrentMessage);
    auto *writeRow = new QHBoxLayout();
    writeRow->addWidget(m_input);
    writeRow->addWidget(sendBtn);

    m_ipLabel = new QLabel(ConnectionManager::localIPv4(), this);
    m_portLabel = new QLabel(this);
    auto *copyIpBtn = new QPushButton("Copier IP", this);
    auto *copyPortBtn = new QPushButton("Copier port", this);
    connect(copyIpBtn, &QPushButton::clicked, this, [this] { QApplication::clipboard()->setText(m_ipLabel->text()); });
    connect(copyPortBtn, &QPushButton::clicked, this, [this] { QApplication::clipboard()->setText(QString::number(m_settings.port)); });

    auto *networkBox = new QGroupBox("CONNEXION LOCALE", this);
    auto *networkForm = new QHBoxLayout();
    networkForm->addWidget(new QLabel("IP :", this));
    networkForm->addWidget(m_ipLabel);
    networkForm->addWidget(copyIpBtn);
    auto *networkLayout = new QVBoxLayout(networkBox);
    networkLayout->addLayout(networkForm);
    auto *portRow = new QHBoxLayout();
    portRow->addWidget(new QLabel("Port :", this));
    portRow->addWidget(m_portLabel);
    portRow->addWidget(copyPortBtn);
    networkLayout->addLayout(portRow);

    auto *left = new QVBoxLayout();
    left->addLayout(top);
    left->addWidget(m_history, 1);
    left->addLayout(writeRow);
    left->addWidget(networkBox);

    m_userList = new QListWidget(this);
    m_countLabel = new QLabel("0 connecté(s)", this);
    auto *joinBtn = new QPushButton("＋ Rejoindre un autre salon", this);
    connect(joinBtn, &QPushButton::clicked, this, &MainWindow::joinAnotherSalon);

    auto *participantsBox = new QGroupBox("PARTICIPANTS DU SALON", this);
    auto *participantsLayout = new QVBoxLayout(participantsBox);
    participantsLayout->addWidget(m_countLabel);
    participantsLayout->addWidget(m_userList, 1);
    participantsLayout->addWidget(joinBtn);

    auto *gameBtn = new QPushButton("ZONE DE JEUX\nBientôt disponible", this);
    gameBtn->setEnabled(false);
    connect(gameBtn, &QPushButton::clicked, this, &MainWindow::onGameZoneClicked);

    auto *right = new QVBoxLayout();
    right->addWidget(participantsBox, 1);
    right->addWidget(gameBtn);

    auto *central = new QWidget(this);
    auto *mainLayout = new QHBoxLayout(central);
    mainLayout->addLayout(left, 3);
    mainLayout->addLayout(right, 1);
    setCentralWidget(central);

    updateStatusLabel();
    refreshUserList();
}

void MainWindow::applyStyle() {
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

void MainWindow::openSettings() {
    SettingsDialog::Settings current{m_settings.id, m_settings.pseudo, m_settings.port, m_settings.automatic};
    SettingsDialog dialog(current, this);

    if (dialog.exec() == QDialog::Accepted) {
        m_settings = dialog.result();
        saveSettings();

        if (m_settings.automatic) {
            m_connection.startHosting(static_cast<quint16>(m_settings.port));
        } else {
            m_connection.stopHosting();
        }
        updateStatusLabel();
        refreshUserList();
    }
}

void MainWindow::sendCurrentMessage() {
    const auto text = m_input->text().trimmed();
    if (text.isEmpty()) return;

    if (!m_connection.isHosting() && !m_connection.isConnectedToRemote()) {
        QMessageBox::information(this, "Aucun salon actif", "Héberge un salon (Paramètres) ou rejoins-en un avant d'envoyer un message.");
        return;
    }

    m_connection.sendMessage(m_settings.id, m_settings.pseudo, text);
    logMessage(m_settings.id + " - " + m_settings.pseudo, text);
    m_input->clear();
}

void MainWindow::joinAnotherSalon() {
    bool ok = false;
    const auto host = QInputDialog::getText(this, "Rejoindre un salon", "IP de l'hôte :", QLineEdit::Normal, "127.0.0.1", &ok);
    if (!ok || host.isEmpty()) return;

    const int port = QInputDialog::getInt(this, "Rejoindre un salon", "Port :", m_settings.port, 1, 65535, 1, &ok);
    if (!ok) return;

    if (m_connection.isHosting()) {
        const auto choice = QMessageBox::question(this, "Quitter l'hébergement ?",
            "Tu héberges déjà un salon : rejoindre un autre va le fermer et déconnecter tout le monde. Continuer ?",
            QMessageBox::Yes | QMessageBox::No);
        if (choice != QMessageBox::Yes) return;
        m_connection.stopHosting();
    }

    m_settings.port = port;
    m_connection.joinRemote(host, static_cast<quint16>(port));
    updateStatusLabel();
}

void MainWindow::onGameZoneClicked() {
    QMessageBox::information(this, "Zone de jeux", "Fonctionnalité en construction.");
}

void MainWindow::onMessageReceived(const QString &id, const QString &pseudo, const QString &message) {
    logMessage(id + " - " + pseudo, message);
}

void MainWindow::onSystemMessage(const QString &text) {
    logMessage("SYSTÈME", text);
}

void MainWindow::onParticipantsChanged() {
    refreshUserList();
}

void MainWindow::onHostingStateChanged(bool) {
    updateStatusLabel();
    refreshUserList();
}

void MainWindow::onErrorOccurred(const QString &error) {
    logMessage("SYSTÈME", "Erreur : " + error);
}

void MainWindow::logMessage(const QString &who, const QString &text) {
    m_history->append("[" + QDateTime::currentDateTime().toString("HH:mm") + "] " + who + " : " + text.toHtmlEscaped());
}

void MainWindow::refreshUserList() {
    m_userList->clear();
    m_userList->addItem("● " + m_settings.id + " - " + m_settings.pseudo + " (vous, hôte)");

    const auto labels = m_connection.participantLabels();
    for (const auto &label : labels)
        m_userList->addItem("○ " + label);

    m_countLabel->setText(QString::number(labels.size() + 1) + " connecté(s)");
}

void MainWindow::updateStatusLabel() {
    if (m_connection.isHosting())
        m_portLabel->setText(QString::number(m_settings.port) + " • salon ouvert");
    else if (m_connection.isConnectedToRemote())
        m_portLabel->setText(QString::number(m_settings.port) + " • connecté à distance");
    else
        m_portLabel->setText("fermé");
}
