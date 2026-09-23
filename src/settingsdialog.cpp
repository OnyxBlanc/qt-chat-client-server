#include "settingsdialog.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QMessageBox>

SettingsDialog::SettingsDialog(const Settings &current, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Parametres");

    m_idEdit = new QLineEdit(current.id, this);
    m_pseudoEdit = new QLineEdit(current.pseudo, this);
    m_portEdit = new QLineEdit(QString::number(current.port), this);
    m_automaticCheck = new QCheckBox("Heberger automatiquement un salon au lancement", this);
    m_automaticCheck->setChecked(current.automatic);

    auto *form = new QFormLayout();
    form->addRow("ID :", m_idEdit);
    form->addRow("Pseudo :", m_pseudoEdit);
    form->addRow("Port d'ecoute :", m_portEdit);
    form->addRow(m_automaticCheck);

    auto *okBtn = new QPushButton("Enregistrer", this);
    auto *cancelBtn = new QPushButton("Annuler", this);
    connect(okBtn, &QPushButton::clicked, this, &SettingsDialog::validateAndAccept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    auto *buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(okBtn);
    buttons->addWidget(cancelBtn);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addLayout(buttons);
}

void SettingsDialog::validateAndAccept() {
    if (m_idEdit->text().trimmed().isEmpty() || m_pseudoEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Champs manquants", "L'ID et le Pseudo sont obligatoires.");
        return;
    }

    bool ok = false;
    const int port = m_portEdit->text().toInt(&ok);
    if (!ok || port < 1 || port > 65535) {
        QMessageBox::warning(this, "Port invalide", "Entre un port entre 1 et 65535.");
        return;
    }

    accept();
}

SettingsDialog::Settings SettingsDialog::result() const {
    Settings s;
    s.id = m_idEdit->text().trimmed();
    s.pseudo = m_pseudoEdit->text().trimmed();
    s.port = m_portEdit->text().toInt();
    s.automatic = m_automaticCheck->isChecked();
    return s;
}
