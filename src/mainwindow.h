#pragma once

#include <QMainWindow>
#include "connectionmanager.h"
#include "settingsdialog.h"

class QTextEdit;
class QLineEdit;
class QListWidget;
class QLabel;
class QPushButton;

// Fenetre principale du salon de discussion.
// Assemble l'UI et pilote ConnectionManager (reseau) + SettingsDialog (parametres).
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void openSettings();
    void sendCurrentMessage();
    void joinAnotherSalon();
    void onGameZoneClicked();

    void onMessageReceived(const QString &id, const QString &pseudo, const QString &message);
    void onSystemMessage(const QString &text);
    void onParticipantsChanged();
    void onHostingStateChanged(bool hosting);
    void onErrorOccurred(const QString &error);

private:
    void buildUi();
    void applyStyle();
    void loadSettings();
    void saveSettings();
    void logMessage(const QString &who, const QString &text);
    void refreshUserList();
    void updateStatusLabel();

    ConnectionManager m_connection;
    SettingsDialog::Settings m_settings;

    QTextEdit *m_history;
    QLineEdit *m_input;
    QLabel *m_ipLabel;
    QLabel *m_portLabel;
    QListWidget *m_userList;
    QLabel *m_countLabel;
};
