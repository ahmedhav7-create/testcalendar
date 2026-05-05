#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include <QString>
#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QRegularExpression>

class AuthManager {
public:
    // Singleton: one shared instance across the whole app
    static AuthManager& instance() {
        static AuthManager inst;
        return inst;
    }

    static bool isValidEmail(const QString& email) {
        static QRegularExpression emailRegex(
            R"(^[A-Za-z0-9._%+\-]+@[A-Za-z0-9.\-]+\.[A-Za-z]{2,}$)"
        );
        return emailRegex.match(email).hasMatch();
    }

    bool registerUser(const QString& email, const QString& password) {
        if (users.contains(email))
            return false;
        users[email] = password;
        saveUsersToFile();
        return true;
    }

    bool validateUser(const QString& email, const QString& password) {
        return users.contains(email) && users[email] == password;
    }

private:
    AuthManager() {
        loadUsersFromFile();
    }

    QMap<QString, QString> users;
    const QString databasePath = "users_database.txt";

    void saveUsersToFile() {
        QFile file(databasePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            for (auto it = users.constBegin(); it != users.constEnd(); ++it)
                out << it.key() << "," << it.value() << "\n";
            file.close();
        }
    }

    void loadUsersFromFile() {
        QFile file(databasePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QStringList parts = in.readLine().split(",");
                if (parts.size() == 2)
                    users[parts[0]] = parts[1];
            }
            file.close();
        }
    }
};

#endif
