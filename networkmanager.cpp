#include "networkmanager.hpp"
#include <iostream>

using namespace std;

// ─── SyncWorker ──────────────────────────────────────────────────────────────

SyncWorker::SyncWorker(QObject *parent) : QObject(parent) {}

void SyncWorker::doSync(const QString& email, const QString& appPassword) {
    try {
        boost::asio::io_context io_context;
        boost::asio::ssl::context ssl_context(boost::asio::ssl::context::tlsv12_client);
        ssl_context.set_default_verify_paths();

        boost::asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("imap.gmail.com", "993");

        boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket(io_context, ssl_context);

        boost::asio::connect(socket.lowest_layer(), endpoints);
        socket.handshake(boost::asio::ssl::stream_base::client);

        string response;

        // Read server greeting
        boost::asio::read_until(socket, boost::asio::dynamic_buffer(response), "\r\n");
        response.clear();

        // LOGIN
        string loginCmd = "A01 LOGIN " + email.toStdString()
                        + " " + appPassword.toStdString() + "\r\n";
        boost::asio::write(socket, boost::asio::buffer(loginCmd));
        boost::asio::read_until(socket, boost::asio::dynamic_buffer(response), "\r\n");

        if (response.find("OK") == string::npos && response.find("Success") == string::npos) {
            emit syncError("Login failed. Please check your App Password.");
            return;
        }
        response.clear();

        // SELECT INBOX
        boost::asio::write(socket, boost::asio::buffer("A02 SELECT INBOX\r\n"));
        int totalMessages = 0;

        do {
            response.clear();
            boost::asio::read_until(socket, boost::asio::dynamic_buffer(response), "\r\n");
            if (response.find("EXISTS") != string::npos) {
                size_t a = response.find(" ");
                size_t b = response.find(" ", a + 1);
                if (a != string::npos && b != string::npos) {
                    try { totalMessages = stoi(response.substr(a + 1, b - a - 1)); }
                    catch (...) {}
                }
            }
        } while (response.find("A02 OK") == string::npos
              && response.find("A02 BAD") == string::npos);

        if (totalMessages == 0) {
            boost::asio::write(socket, boost::asio::buffer("A04 LOGOUT\r\n"));
            emit syncSuccess("");   // nothing to import
            return;
        }

        int startMsg = max(1, totalMessages - 2);
        string fetchCmd = "A03 FETCH " + to_string(startMsg) + ":"
                        + to_string(totalMessages) + " BODY[TEXT]\r\n";
        boost::asio::write(socket, boost::asio::buffer(fetchCmd));

        string fullBody;
        do {
            response.clear();
            boost::asio::read_until(socket, boost::asio::dynamic_buffer(response), "\r\n");
            fullBody += response;
        } while (response.find("A03 OK") == string::npos
              && response.find("A03 BAD") == string::npos);

        boost::asio::write(socket, boost::asio::buffer("A04 LOGOUT\r\n"));

        emit syncSuccess(QString::fromStdString(fullBody));

    } catch (const exception& e) {
        emit syncError(QString("IMAP Error: %1").arg(e.what()));
    }
}

// ─── NetworkManager ──────────────────────────────────────────────────────────

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent), m_isRunning(false)
{
    m_thread = new QThread(this);
    m_worker = new SyncWorker();
    m_worker->moveToThread(m_thread);

    // Worker results → NetworkManager signals (crosses thread boundary safely)
    connect(m_worker, &SyncWorker::syncSuccess, this, &NetworkManager::syncSuccess);
    connect(m_worker, &SyncWorker::syncError,   this, &NetworkManager::syncError);

    // When a result arrives (either success or error), mark as not running
    connect(m_worker, &SyncWorker::syncSuccess, this, [this](const QString&) {
        m_isRunning = false;
    });
    connect(m_worker, &SyncWorker::syncError, this, [this](const QString&) {
        m_isRunning = false;
    });

    // Main thread → worker (queued connection, executes on worker thread)
    connect(this, &NetworkManager::requestSync, m_worker, &SyncWorker::doSync);

    m_thread->start();
}

NetworkManager::~NetworkManager() {
    m_thread->quit();
    m_thread->wait();
    delete m_worker;
}

void NetworkManager::syncWithGmail(const QString& email, const QString& appPassword) {
    if (m_isRunning) {
        emit syncError("A sync is already in progress.");
        return;
    }
    m_isRunning = true;
    emit requestSync(email, appPassword);   // non-blocking: worker picks it up
}
