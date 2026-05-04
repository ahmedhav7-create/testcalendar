#include "loginwindow.hpp"
#include "signupdialog.hpp"
#include "mainwindow.hpp"
#include "authmanager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>

static const QString LOGIN_STYLE = R"(
QMainWindow, QWidget#bg {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #1565C0, stop:1 #0D47A1);
}
QWidget#card {
    background: #FFFFFF;
    border-radius: 14px;
}
QLabel#appName {
    color: #FFFFFF;
    font-size: 26px;
    font-weight: bold;
    font-family: 'Segoe UI', Arial, sans-serif;
}
QLabel#appSub {
    color: #BBDEFB;
    font-size: 13px;
    font-family: 'Segoe UI', Arial, sans-serif;
}
QLabel#fieldLabel {
    color: #546E7A;
    font-size: 11px;
    font-weight: bold;
    font-family: 'Segoe UI', Arial, sans-serif;
    letter-spacing: 0.5px;
}
QLabel#errorLabel {
    color: #C62828;
    font-size: 11px;
    font-family: 'Segoe UI', Arial, sans-serif;
}
QLineEdit {
    border: 1px solid #CFD8DC;
    border-radius: 6px;
    padding: 9px 12px;
    font-size: 13px;
    font-family: 'Segoe UI', Arial, sans-serif;
    background: #F8FAFC;
    color: #212121;
    min-height: 20px;
}
QLineEdit:focus {
    border: 2px solid #1565C0;
    background: #FFFFFF;
}
QLineEdit:hover {
    border-color: #78909C;
}
QPushButton#loginBtn {
    background: #1565C0;
    color: white;
    border: none;
    border-radius: 6px;
    font-size: 14px;
    font-weight: bold;
    font-family: 'Segoe UI', Arial, sans-serif;
    padding: 11px;
    min-height: 20px;
}
QPushButton#loginBtn:hover   { background: #1976D2; }
QPushButton#loginBtn:pressed { background: #0D47A1; }
QPushButton#signUpBtn {
    background: transparent;
    color: #1565C0;
    border: none;
    font-size: 12px;
    font-weight: bold;
    font-family: 'Segoe UI', Arial, sans-serif;
    text-decoration: underline;
    padding: 0;
}
QPushButton#signUpBtn:hover { color: #0D47A1; }
)";

LoginWindow::LoginWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Smart Prioritized Calendar");
    setFixedSize(460, 560);
    setStyleSheet(LOGIN_STYLE);

    // ── Background widget ─────────────────────────────────────────────────────
    QWidget *bg = new QWidget(this);
    bg->setObjectName("bg");
    setCentralWidget(bg);

    QVBoxLayout *bgLayout = new QVBoxLayout(bg);
    bgLayout->setContentsMargins(0, 0, 0, 0);
    bgLayout->setSpacing(0);

    // ── Top branding area ─────────────────────────────────────────────────────
    QWidget *brandArea = new QWidget(bg);
    brandArea->setStyleSheet("background: transparent;");
    QVBoxLayout *brandLayout = new QVBoxLayout(brandArea);
    brandLayout->setContentsMargins(40, 44, 40, 28);
    brandLayout->setSpacing(6);
    brandLayout->setAlignment(Qt::AlignCenter);

    QLabel *icon = new QLabel("📅", brandArea);
    icon->setAlignment(Qt::AlignCenter);
    QFont iconFont = icon->font();
    iconFont.setPointSize(36);
    icon->setFont(iconFont);
    icon->setStyleSheet("font-size: 48px; background: transparent;");

    QLabel *appName = new QLabel("Smart Calendar", brandArea);
    appName->setObjectName("appName");
    appName->setAlignment(Qt::AlignCenter);

    QLabel *appSub = new QLabel("Prioritize your time, simplify your life.", brandArea);
    appSub->setObjectName("appSub");
    appSub->setAlignment(Qt::AlignCenter);

    brandLayout->addWidget(icon);
    brandLayout->addWidget(appName);
    brandLayout->addWidget(appSub);
    bgLayout->addWidget(brandArea);

    // ── White card ────────────────────────────────────────────────────────────
    QWidget *card = new QWidget(bg);
    card->setObjectName("card");

    // Drop shadow on the card
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(24);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 60));
    card->setGraphicsEffect(shadow);

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(32, 28, 32, 28);
    cardLayout->setSpacing(10);

    // Email
    QLabel *emailLbl = new QLabel("EMAIL ADDRESS", card);
    emailLbl->setObjectName("fieldLabel");
    usernameInput = new QLineEdit(card);
    usernameInput->setPlaceholderText("you@example.com");

    // Inline email error
    emailError = new QLabel("", card);
    emailError->setObjectName("errorLabel");
    emailError->setVisible(false);

    // Password
    QLabel *passLbl = new QLabel("PASSWORD", card);
    passLbl->setObjectName("fieldLabel");
    passwordInput = new QLineEdit(card);
    passwordInput->setPlaceholderText("Enter your password");
    passwordInput->setEchoMode(QLineEdit::Password);

    // Login button
    loginBtn = new QPushButton("Log In", card);
    loginBtn->setObjectName("loginBtn");
    cardLayout->addSpacing(4);

    // Sign-up row
    QHBoxLayout *signUpRow = new QHBoxLayout();
    QLabel *noAcc = new QLabel("Don't have an account?", card);
    noAcc->setStyleSheet("color:#78909C; font-size:12px; background:transparent;");
    signUpBtn = new QPushButton("Sign Up", card);
    signUpBtn->setObjectName("signUpBtn");
    signUpRow->addStretch();
    signUpRow->addWidget(noAcc);
    signUpRow->addWidget(signUpBtn);
    signUpRow->addStretch();

    cardLayout->addWidget(emailLbl);
    cardLayout->addWidget(usernameInput);
    cardLayout->addWidget(emailError);
    cardLayout->addSpacing(4);
    cardLayout->addWidget(passLbl);
    cardLayout->addWidget(passwordInput);
    cardLayout->addSpacing(10);
    cardLayout->addWidget(loginBtn);
    cardLayout->addSpacing(4);
    cardLayout->addLayout(signUpRow);

    // Wrap card in a horizontal centering layout with margins
    QHBoxLayout *cardRow = new QHBoxLayout();
    cardRow->setContentsMargins(24, 0, 24, 30);
    cardRow->addWidget(card);
    bgLayout->addLayout(cardRow);

    // ── Connections ───────────────────────────────────────────────────────────
    connect(usernameInput, &QLineEdit::textChanged, this, [this](const QString& t) {
        if (t.isEmpty() || AuthManager::isValidEmail(t)) {
            emailError->setVisible(false);
            usernameInput->setStyleSheet("");
        } else {
            emailError->setText("Please enter a valid email address.");
            emailError->setVisible(true);
            usernameInput->setStyleSheet(
                "border:2px solid #C62828; border-radius:6px; padding:9px 12px;"
                "background:#FFF8F8;");
        }
    });

    connect(passwordInput, &QLineEdit::returnPressed, this, &LoginWindow::onLoginClicked);
    connect(loginBtn,  &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(signUpBtn, &QPushButton::clicked, this, &LoginWindow::onSignUpClicked);
}

void LoginWindow::onSignUpClicked() {
    SignUpDialog dialog(this);
    dialog.exec();
}

void LoginWindow::onLoginClicked() {
    QString email    = usernameInput->text().trimmed();
    QString password = passwordInput->text();

    if (!AuthManager::isValidEmail(email)) {
        emailError->setText("Please enter a valid email address.");
        emailError->setVisible(true);
        usernameInput->setFocus();
        return;
    }

    if (AuthManager::instance().validateUser(email, password)) {
        MainWindow *mainWindow = new MainWindow();
        mainWindow->show();
        this->close();
    } else {
        passwordInput->clear();
        passwordInput->setStyleSheet(
            "border:2px solid #C62828; border-radius:6px; padding:9px 12px;"
            "background:#FFF8F8;");
        QMessageBox::warning(this, "Login Failed",
            "Incorrect email or password.\nPlease try again or sign up.");
        passwordInput->setStyleSheet("");
    }
}
