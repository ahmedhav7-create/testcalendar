#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QString>
#include <QThread>
#include <string>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

// ─── Worker ──────────────────────────────────────────────────────────────────
// Runs the blocking IMAP sync on a dedicated thread so the Qt GUI
// event loop is never blocked.  This is the standard Qt pattern for
// offloading long network operations:
//   1. Create a QThread.
//   2. Create a Worker and move it to that thread.
//   3. Connect signals/slots across the thread boundary.
//
// NOTE: This goes slightly beyond what the course slides cover (which
// show Boost.Asio's own async model).  Here we use QThread because Qt
// requires GUI updates to happen on the main thread, and the simplest
// safe approach is to run the blocking Boost.Asio code on a worker
// thread and emit Qt signals back to the main thread when done.
// ─────────────────────────────────────────────────────────────────────────────

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

// ─── NetworkManager ──────────────────────────────────────────────────────────
class NetworkManager : public QObject {
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    // Call this from the main thread; it posts work to the worker thread.
    void syncWithGmail(const QString& email, const QString& appPassword);

signals:
    void syncSuccess(const QString& data);
    void syncError(const QString& errorMessage);

    // Internal: tells the worker to start
    void requestSync(const QString& email, const QString& appPassword);

private:
    QThread    *m_thread;
    SyncWorker *m_worker;
    bool        m_isRunning;
};

#endif
