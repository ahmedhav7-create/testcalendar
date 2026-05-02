#include "signupdialog.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include "authmanager.hpp"

SignUpDialog::SignUpDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Create New Account");
    setMinimumWidth(380);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(24, 24, 24, 24);

    QLabel *title = new QLabel("Create New Account", this);
    QFont titleFont = title->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);

    fullNameInput = new QLineEdit(this);
    fullNameInput->setPlaceholderText("e.g. Ahmed Mohamed");
    fullNameInput->setMinimumHeight(34);

    emailInput = new QLineEdit(this);
    emailInput->setPlaceholderText("e.g. ahmed@example.com");
    emailInput->setMinimumHeight(34);

    // Inline error label shown when email format is wrong
    emailErrorLabel = new QLabel("", this);
    emailErrorLabel->setStyleSheet("color: #c0392b; font-size: 11px;");
    emailErrorLabel->setVisible(false);

    passwordInput = new QLineEdit(this);
    passwordInput->setEchoMode(QLineEdit::Password);
    passwordInput->setPlaceholderText("At least 6 characters");
    passwordInput->setMinimumHeight(34);

    confirmPasswordInput = new QLineEdit(this);
    confirmPasswordInput->setEchoMode(QLineEdit::Password);
    confirmPasswordInput->setPlaceholderText("Repeat password");
    confirmPasswordInput->setMinimumHeight(34);

    QPushButton *createBtn = new QPushButton("Create Account", this);
    createBtn->setMinimumHeight(38);

    layout->addWidget(title);
    layout->addSpacing(8);
    layout->addWidget(new QLabel("Full Name", this));
    layout->addWidget(fullNameInput);
    layout->addWidget(new QLabel("Email Address", this));
    layout->addWidget(emailInput);
    layout->addWidget(emailErrorLabel);
    layout->addWidget(new QLabel("Password", this));
    layout->addWidget(passwordInput);
    layout->addWidget(new QLabel("Confirm Password", this));
    layout->addWidget(confirmPasswordInput);
    layout->addSpacing(12);
    layout->addWidget(createBtn);

    // Real-time email validation as the user types
    connect(emailInput, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (text.isEmpty()) {
            emailErrorLabel->setVisible(false);
        } else if (!AuthManager::isValidEmail(text)) {
            emailErrorLabel->setText("Please enter a valid email address.");
            emailErrorLabel->setVisible(true);
        } else {
            emailErrorLabel->setVisible(false);
        }
    });

    connect(createBtn, &QPushButton::clicked, this, &SignUpDialog::onCreateAccount);
}

void SignUpDialog::onCreateAccount() {
    QString fullName = fullNameInput->text().trimmed();
    QString email    = emailInput->text().trimmed();
    QString password = passwordInput->text();
    QString confirm  = confirmPasswordInput->text();

    if (fullName.isEmpty()) {
        QMessageBox::warning(this, "Missing Field", "Please enter your full name.");
        return;
    }

    if (!AuthManager::isValidEmail(email)) {
        QMessageBox::warning(this, "Invalid Email", "Please enter a valid email address.");
        return;
    }

    if (password.length() < 6) {
        QMessageBox::warning(this, "Weak Password", "Password must be at least 6 characters.");
        return;
    }

    if (password != confirm) {
        QMessageBox::warning(this, "Password Mismatch", "Passwords do not match.");
        return;
    }

    if (AuthManager::instance().registerUser(email, password)) {
        QMessageBox::information(this, "Success",
            QString("Account created for %1!\nYou can now log in.").arg(fullName));
        accept();
    } else {
        QMessageBox::warning(this, "Already Registered",
            "An account with this email already exists.");
    }
}
