#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>

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
    QPushButton *loginBtn;
    QPushButton *signUpBtn;
};

#endif
