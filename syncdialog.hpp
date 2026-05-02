#ifndef SYNCDIALOG_H
#define SYNCDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>

class SyncDialog : public QDialog {
    Q_OBJECT

public:
    explicit SyncDialog(QWidget *parent = nullptr);

    QString email() const;
    QString appPassword() const;

private slots:
    void onConnect();

private:
    QLineEdit *emailInput;
    QLineEdit *appPasswordInput;
    QLabel    *emailErrorLabel;
};

#endif
