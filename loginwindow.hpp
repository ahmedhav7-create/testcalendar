#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class LoginWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);

private slots:
    void onSignUpClicked();
    void onLoginClicked();

private:
    QLineEdit   *usernameInput;
    QLineEdit   *passwordInput;
    QLabel      *emailError;
    QPushButton *loginBtn;
    QPushButton *signUpBtn;
};

#endif
