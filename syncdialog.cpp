#include "syncdialog.hpp"
#include "authmanager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QFrame>

static const QString SYNC_STYLE = R"(
QDialog {
    background: #F5F7FA;
    font-family: 'Segoe UI', Arial, sans-serif;
}
QLabel#dlgTitle  { font-size:15px; font-weight:bold; color:#1565C0; }
QLabel#dlgInfo   { color:#546E7A; font-size:12px; }
QLabel#fieldLabel{ color:#546E7A; font-size:11px; font-weight:bold; letter-spacing:0.5px; }
QLabel#errorLabel{ color:#C62828; font-size:11px; }
QLineEdit {
    border:1px solid #CFD8DC; border-radius:6px;
    padding:8px 12px; font-size:13px;
    background:#FFFFFF; min-height:18px;
}
QLineEdit:focus { border:2px solid #1565C0; }
QLineEdit:hover { border-color:#78909C; }
QPushButton#syncBtn {
    background:#1565C0; color:white; border:none;
    border-radius:6px; font-size:13px; font-weight:bold; padding:10px;
}
QPushButton#syncBtn:hover   { background:#1976D2; }
QPushButton#syncBtn:pressed { background:#0D47A1; }
QPushButton#cancelBtn {
    background:#FFFFFF; color:#546E7A;
    border:1px solid #CFD8DC; border-radius:6px;
    font-size:13px; padding:10px;
}
QPushButton#cancelBtn:hover { background:#ECEFF1; }
)";

SyncDialog::SyncDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Sync with Gmail");
    setMinimumWidth(420);
    setStyleSheet(SYNC_STYLE);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(10);

    QLabel *title = new QLabel("Connect to Gmail", this);
    title->setObjectName("dlgTitle");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QFrame *div = new QFrame(this);
    div->setFrameShape(QFrame::HLine);
    div->setStyleSheet("color:#E0E0E0;");
    layout->addWidget(div);

    QLabel *info = new QLabel(
        "Enter your Gmail address and a Google App Password.\n"
        "App Passwords are 16-character codes generated in your\n"
        "Google Account → Security → 2-Step Verification settings.",
        this);
    info->setObjectName("dlgInfo");
    info->setWordWrap(true);
    info->setAlignment(Qt::AlignCenter);
    layout->addWidget(info);

    layout->addSpacing(4);

    QLabel *emailLbl = new QLabel("GMAIL ADDRESS", this);
    emailLbl->setObjectName("fieldLabel");
    layout->addWidget(emailLbl);

    emailInput = new QLineEdit(this);
    emailInput->setPlaceholderText("your.address@gmail.com");
    layout->addWidget(emailInput);

    emailErrorLabel = new QLabel("", this);
    emailErrorLabel->setObjectName("errorLabel");
    emailErrorLabel->setVisible(false);
    layout->addWidget(emailErrorLabel);

    layout->addSpacing(4);

    QLabel *passLbl = new QLabel("APP PASSWORD", this);
    passLbl->setObjectName("fieldLabel");
    layout->addWidget(passLbl);

    appPasswordInput = new QLineEdit(this);
    appPasswordInput->setEchoMode(QLineEdit::Password);
    appPasswordInput->setPlaceholderText("xxxx xxxx xxxx xxxx");
    layout->addWidget(appPasswordInput);

    layout->addSpacing(14);

    QHBoxLayout *btnRow = new QHBoxLayout();
    QPushButton *cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setObjectName("cancelBtn");
    QPushButton *syncBtn = new QPushButton("⟳  Sync", this);
    syncBtn->setObjectName("syncBtn");
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(syncBtn);
    layout->addLayout(btnRow);

    // Real-time email validation
    connect(emailInput, &QLineEdit::textChanged, this, [this](const QString& t) {
        if (!t.isEmpty() && !AuthManager::isValidEmail(t)) {
            emailErrorLabel->setText("Please enter a valid Gmail address.");
            emailErrorLabel->setVisible(true);
        } else {
            emailErrorLabel->setVisible(false);
        }
    });

    connect(syncBtn,   &QPushButton::clicked, this, &SyncDialog::onConnect);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void SyncDialog::onConnect() {
    if (!AuthManager::isValidEmail(emailInput->text().trimmed())) {
        emailErrorLabel->setText("Please enter a valid Gmail address.");
        emailErrorLabel->setVisible(true);
        emailInput->setFocus();
        return;
    }
    if (appPasswordInput->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "Missing Password", "Please enter your App Password.");
        appPasswordInput->setFocus();
        return;
    }
    accept();
}

QString SyncDialog::email()       const { return emailInput->text().trimmed(); }
QString SyncDialog::appPassword() const { return appPasswordInput->text().trimmed(); }
