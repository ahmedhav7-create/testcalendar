#include "loginwindow.hpp"
#include "signupdialog.hpp"
#include "mainwindow.hpp"
#include "authmanager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>

LoginWindow::LoginWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Smart Prioritized Calendar — Login");
    setFixedSize(420, 340);

    QWidget *central = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(central);
    layout->setContentsMargins(50, 40, 50, 40);
    layout->setSpacing(12);

    QLabel *appLabel = new QLabel("📅 Smart Calendar", this);
    QFont big = appLabel->font();
    big.setPointSize(16);
    big.setBold(true);
    appLabel->setFont(big);
    appLabel->setAlignment(Qt::AlignCenter);

    QLabel *subLabel = new QLabel("Sign in to continue", this);
    subLabel->setAlignment(Qt::AlignCenter);
    subLabel->setStyleSheet("color: #666;");

    usernameInput = new QLineEdit(this);
    usernameInput->setPlaceholderText("Email address");
    usernameInput->setMinimumHeight(34);

    passwordInput = new QLineEdit(this);
    passwordInput->setPlaceholderText("Password");
    passwordInput->setEchoMode(QLineEdit::Password);
    passwordInput->setMinimumHeight(34);

    loginBtn = new QPushButton("Log In", this);
    loginBtn->setMinimumHeight(36);

    QHBoxLayout *signUpRow = new QHBoxLayout();
    QLabel *noAccountLabel = new QLabel("Don't have an account?", this);
    signUpBtn = new QPushButton("Sign Up", this);
    signUpBtn->setFlat(true);
    signUpBtn->setStyleSheet("color: #0078d7; text-decoration: underline;");
    signUpRow->addStretch();
    signUpRow->addWidget(noAccountLabel);
    signUpRow->addWidget(signUpBtn);
    signUpRow->addStretch();

    layout->addWidget(appLabel);
    layout->addWidget(subLabel);
    layout->addSpacing(8);
    layout->addWidget(new QLabel("Email", this));
    layout->addWidget(usernameInput);
    layout->addWidget(new QLabel("Password", this));
    layout->addWidget(passwordInput);
    layout->addSpacing(4);
    layout->addWidget(loginBtn);
    layout->addLayout(signUpRow);

    setCentralWidget(central);

    // Allow pressing Enter to log in
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
        QMessageBox::warning(this, "Invalid Email",
            "Please enter a valid email address.");
        return;
    }

    if (AuthManager::instance().validateUser(email, password)) {
        MainWindow *mainWindow = new MainWindow();
        mainWindow->show();
        this->close();
    } else {
        QMessageBox::critical(this, "Login Failed",
            "Invalid email or password.\nPlease try again or sign up.");
        passwordInput->clear();
    }
}
