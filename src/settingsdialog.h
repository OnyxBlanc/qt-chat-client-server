#pragma once

#include <QDialog>
#include <QString>

class QLineEdit;
class QCheckBox;

// Dialogue de parametres : ID, Pseudo, port d'ecoute et hebergement
// automatique au demarrage. Ces valeurs sont ensuite sauvegardees par
// MainWindow via QSettings.
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    struct Settings {
        QString id;
        QString pseudo;
        int port = 5000;
        bool automatic = true;
    };

    explicit SettingsDialog(const Settings &current, QWidget *parent = nullptr);
    Settings result() const;

private slots:
    void validateAndAccept();

private:
    QLineEdit *m_idEdit;
    QLineEdit *m_pseudoEdit;
    QLineEdit *m_portEdit;
    QCheckBox *m_automaticCheck;
};
