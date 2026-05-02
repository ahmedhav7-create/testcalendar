#include "syncdialog.hpp"
#include "authmanager.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>

SyncDialog::SyncDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Sync with Gmail");
    setMinimumWidth(400);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(24, 24, 24, 24);

    QLabel *title = new QLabel("Connect to Gmail", this);
    QFont titleFont = title->font();
    titleFont.setPointSize(13);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);

    QLabel *infoLabel = new QLabel(
        "Enter your Gmail address and a Google App Password.\n"
        "App Passwords can be created in your Google Account security settings.",
        this
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #555; font-size: 11px;");

    emailInput = new QLineEdit(this);
    emailInput->setPlaceholderText("your.address@gmail.com");
    emailInput->setMinimumHeight(34);

    emailErrorLabel = new QLabel("", this);
    emailErrorLabel->setStyleSheet("color: #c0392b; font-size: 11px;");
    emailErrorLabel->setVisible(false);

    appPasswordInput = new QLineEdit(this);
    appPasswordInput->setEchoMode(QLineEdit::Password);
    appPasswordInput->setPlaceholderText("16-character App Password");
    appPasswordInput->setMinimumHeight(34);

    QPushButton *connectBtn = new QPushButton("Sync", this);
    connectBtn->setMinimumHeight(38);

    layout->addWidget(title);
    layout->addSpacing(4);
    layout->addWidget(infoLabel);
    layout->addSpacing(8);
    layout->addWidget(new QLabel("Gmail Address", this));
    layout->addWidget(emailInput);
    layout->addWidget(emailErrorLabel);
    layout->addWidget(new QLabel("App Password", this));
    layout->addWidget(appPasswordInput);
    layout->addSpacing(12);
    layout->addWidget(connectBtn);

    connect(emailInput, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (!text.isEmpty() && !AuthManager::isValidEmail(text)) {
            emailErrorLabel->setText("Please enter a valid Gmail address.");
            emailErrorLabel->setVisible(true);
        } else {
            emailErrorLabel->setVisible(false);
        }
    });

    connect(connectBtn, &QPushButton::clicked, this, &SyncDialog::onConnect);
}

void SyncDialog::onConnect() {
    if (!AuthManager::isValidEmail(emailInput->text().trimmed())) {
        QMessageBox::warning(this, "Invalid Email", "Please enter a valid Gmail address.");
        return;
    }
    if (appPasswordInput->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Missing Password", "Please enter your App Password.");
        return;
    }
    accept();
}

QString SyncDialog::email() const {
    return emailInput->text().trimmed();
}

QString SyncDialog::appPassword() const {
    return appPasswordInput->text().trimmed();
}
