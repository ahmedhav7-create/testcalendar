#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QString>
#include <QThread>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

// SyncWorker does the actual IMAP work and lives on a background thread.
// It communicates results back to the main thread via Qt signals.
class SyncWorker : public QObject {
    Q_OBJECT
public:
    explicit SyncWorker(QObject *parent = nullptr);

public slots:
    void doSync(const QString& email, const QString& appPassword);

signals:
    void syncSuccess(const QString& data);
    void syncError(const QString& errorMessage);
};

class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    void syncWithGmail(const QString& email, const QString& appPassword);

signals:
    void syncSuccess(const QString& data);
    void syncError(const QString& errorMessage);
    void requestSync(const QString& email, const QString& appPassword);

private:
    QThread    *m_thread;
    SyncWorker *m_worker;
    bool        m_isRunning;
};

#endif
