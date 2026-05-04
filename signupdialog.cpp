#include "signupdialog.hpp"
#include "authmanager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QFrame>

static const QString SIGNUP_STYLE = R"(
QDialog {
    background: #F5F7FA;
    font-family: 'Segoe UI', Arial, sans-serif;
}
QLabel#sectionTitle {
    font-size: 16px;
    font-weight: bold;
    color: #1565C0;
}
QLabel#fieldLabel {
    color: #546E7A;
    font-size: 11px;
    font-weight: bold;
    letter-spacing: 0.5px;
}
QLabel#errorLabel {
    color: #C62828;
    font-size: 11px;
}
QLabel#successHint {
    color: #2E7D32;
    font-size: 11px;
}
QLineEdit {
    border: 1px solid #CFD8DC;
    border-radius: 6px;
    padding: 8px 12px;
    font-size: 13px;
    background: #FFFFFF;
    min-height: 18px;
}
QLineEdit:focus { border: 2px solid #1565C0; background: #FFFFFF; }
QLineEdit:hover { border-color: #78909C; }
QPushButton#createBtn {
    background: #1565C0;
    color: white;
    border: none;
    border-radius: 6px;
    font-size: 13px;
    font-weight: bold;
    padding: 10px;
}
QPushButton#createBtn:hover   { background: #1976D2; }
QPushButton#createBtn:pressed { background: #0D47A1; }
QPushButton#cancelBtn {
    background: #FFFFFF;
    color: #546E7A;
    border: 1px solid #CFD8DC;
    border-radius: 6px;
    font-size: 13px;
    padding: 10px;
}
QPushButton#cancelBtn:hover { background: #ECEFF1; }
)";

SignUpDialog::SignUpDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Create New Account");
    setMinimumWidth(420);
    setStyleSheet(SIGNUP_STYLE);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(10);

    // Title
    QLabel *title = new QLabel("Create New Account", this);
    title->setObjectName("sectionTitle");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QFrame *div = new QFrame(this);
    div->setFrameShape(QFrame::HLine);
    div->setStyleSheet("color:#E0E0E0;");
    layout->addWidget(div);
    layout->addSpacing(4);

    // Full name
    QLabel *nameLbl = new QLabel("FULL NAME", this);
    nameLbl->setObjectName("fieldLabel");
    layout->addWidget(nameLbl);
    fullNameInput = new QLineEdit(this);
    fullNameInput->setPlaceholderText("e.g. Ahmed Mohamed");
    layout->addWidget(fullNameInput);

    layout->addSpacing(2);

    // Email
    QLabel *emailLbl = new QLabel("EMAIL ADDRESS", this);
    emailLbl->setObjectName("fieldLabel");
    layout->addWidget(emailLbl);
    emailInput = new QLineEdit(this);
    emailInput->setPlaceholderText("e.g. ahmed@example.com");
    layout->addWidget(emailInput);

    emailErrorLabel = new QLabel("", this);
    emailErrorLabel->setObjectName("errorLabel");
    emailErrorLabel->setVisible(false);
    layout->addWidget(emailErrorLabel);

    layout->addSpacing(2);

    // Password
    QLabel *passLbl = new QLabel("PASSWORD", this);
    passLbl->setObjectName("fieldLabel");
    layout->addWidget(passLbl);
    passwordInput = new QLineEdit(this);
    passwordInput->setEchoMode(QLineEdit::Password);
    passwordInput->setPlaceholderText("At least 6 characters");
    layout->addWidget(passwordInput);

    // Password strength hint
    strengthLabel = new QLabel("", this);
    strengthLabel->setObjectName("successHint");
    strengthLabel->setVisible(false);
    layout->addWidget(strengthLabel);

    layout->addSpacing(2);

    // Confirm password
    QLabel *confLbl = new QLabel("CONFIRM PASSWORD", this);
    confLbl->setObjectName("fieldLabel");
    layout->addWidget(confLbl);
    confirmPasswordInput = new QLineEdit(this);
    confirmPasswordInput->setEchoMode(QLineEdit::Password);
    confirmPasswordInput->setPlaceholderText("Repeat password");
    layout->addWidget(confirmPasswordInput);

    matchLabel = new QLabel("", this);
    matchLabel->setObjectName("errorLabel");
    matchLabel->setVisible(false);
    layout->addWidget(matchLabel);

    layout->addSpacing(12);

    // Buttons
    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setObjectName("cancelBtn");
    QPushButton *createBtn = new QPushButton("Create Account", this);
    createBtn->setObjectName("createBtn");
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(createBtn);
    layout->addLayout(btnRow);

    // ── Real-time validation ──────────────────────────────────────────────────
    connect(emailInput, &QLineEdit::textChanged, this, [this](const QString& t) {
        if (t.isEmpty()) { emailErrorLabel->setVisible(false); return; }
        if (!AuthManager::isValidEmail(t)) {
            emailErrorLabel->setText("Please enter a valid email address.");
            emailErrorLabel->setVisible(true);
        } else {
            emailErrorLabel->setVisible(false);
        }
    });

    connect(passwordInput, &QLineEdit::textChanged, this, [this](const QString& t) {
        if (t.isEmpty()) { strengthLabel->setVisible(false); return; }
        if (t.length() < 6) {
            strengthLabel->setText("Password too short (minimum 6 characters).");
            strengthLabel->setStyleSheet("color:#C62828; font-size:11px;");
        } else {
            strengthLabel->setText("✔  Password length OK.");
            strengthLabel->setStyleSheet("color:#2E7D32; font-size:11px;");
        }
        strengthLabel->setVisible(true);
    });

    connect(confirmPasswordInput, &QLineEdit::textChanged, this, [this](const QString& t) {
        if (t.isEmpty()) { matchLabel->setVisible(false); return; }
        if (t != passwordInput->text()) {
            matchLabel->setText("Passwords do not match.");
            matchLabel->setVisible(true);
        } else {
            matchLabel->setVisible(false);
        }
    });

    connect(createBtn, &QPushButton::clicked, this, &SignUpDialog::onCreateAccount);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void SignUpDialog::onCreateAccount() {
    QString fullName = fullNameInput->text().trimmed();
    QString email    = emailInput->text().trimmed();
    QString password = passwordInput->text();
    QString confirm  = confirmPasswordInput->text();

    if (fullName.isEmpty()) {
        QMessageBox::warning(this, "Missing Field", "Please enter your full name.");
        fullNameInput->setFocus();
        return;
    }
    if (!AuthManager::isValidEmail(email)) {
        QMessageBox::warning(this, "Invalid Email", "Please enter a valid email address.");
        emailInput->setFocus();
        return;
    }
    if (password.length() < 6) {
        QMessageBox::warning(this, "Weak Password", "Password must be at least 6 characters.");
        passwordInput->setFocus();
        return;
    }
    if (password != confirm) {
        QMessageBox::warning(this, "Password Mismatch", "Passwords do not match.");
        confirmPasswordInput->setFocus();
        return;
    }
    if (AuthManager::instance().registerUser(email, password)) {
        QMessageBox::information(this, "Account Created",
            QString("Welcome, %1!\nYour account has been created. You can now log in.")
            .arg(fullName));
        accept();
    } else {
        QMessageBox::warning(this, "Already Registered",
            "An account with this email already exists.");
    }
}
