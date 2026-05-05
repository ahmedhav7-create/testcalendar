#include "networkmanager.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>

using namespace std;
namespace asio = boost::asio;
using tcp_ssl_socket = asio::ssl::stream<asio::ip::tcp::socket>;

// Quoted-Printable is an email encoding (RFC 2045) where special characters
// are written as (hex). Long lines are split with a trailing = followed
// by a newline, we rejoin those here so the text is readable again.
static string decodeQuotedPrintable(const string& input) {
    string result;
    result.reserve(input.size());
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] != '=') { result += input[i++]; continue; }
        // Soft line break: the = at end of line means "continue on next line"
        if (i+1 < input.size() && input[i+1] == '\n')                   { i += 2; continue; }
        if (i+2 < input.size() && input[i+1]=='\r' && input[i+2]=='\n') { i += 3; continue; }
        if (i+2 < input.size()
            && isxdigit((unsigned char)input[i+1])
            && isxdigit((unsigned char)input[i+2])) {
            result += static_cast<char>(stoul(string{input[i+1], input[i+2]}, nullptr, 16));
            i += 3; continue;
        }
        result += input[i++];
    }
    return result;
}

// Email messages have headers at the top, then a blank line, then the body.
// This splits a raw message block at that blank line.
static void splitHeadersBody(const string& block, string& headers, string& body) {
    size_t sep = block.find("\r\n\r\n");
    size_t sepLen = 4;
    if (sep == string::npos) { sep = block.find("\n\n"); sepLen = 2; }
    if (sep == string::npos) { headers = ""; body = block; return; }
    headers = block.substr(0, sep);
    body    = block.substr(sep + sepLen);
}

// Looks for a specific header field (e.g. "content-type") in the headers block.
static string getHeader(const string& headers, const string& key) {
    istringstream ss(headers);
    string line, lkey = key;
    for (auto& c : lkey) c = tolower(c);
    while (getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        string ll = line;
        for (auto& c : ll) c = tolower(c);
        if (ll.substr(0, lkey.size()) == lkey) {
            string val = line.substr(line.find(':') + 1);
            while (!val.empty() && isspace((unsigned char)val.front())) val.erase(val.begin());
            return val;
        }
    }
    return "";
}

// Gmail sends emails as multipart/alternative, meaning the same message
// appears twice in the raw data: once as text/plain and once as text/html.
// We only want the plain-text part so our date regex can scan it cleanly.
static string extractPlainText(const string& rawMessage) {
    string headers, body;
    splitHeadersBody(rawMessage, headers, body);

    string ct  = getHeader(headers, "content-type");
    string enc = getHeader(headers, "content-transfer-encoding");
    for (auto& c : enc) c = tolower(c);

    // Simple (non-multipart) message
    if (ct.find("multipart") == string::npos) {
        if (ct.find("text/plain") == string::npos && !ct.empty()) return "";
        if (enc.find("quoted-printable") != string::npos) return decodeQuotedPrintable(body);
        if (enc.find("base64")           != string::npos) return "";
        return body;
    }

    // Multipart message: parts are separated by a boundary string
    // e.g. Content-Type: multipart/alternative; boundary="abc123"
    size_t bpos = ct.find("boundary=");
    if (bpos == string::npos) return "";
    string boundary = ct.substr(bpos + 9);
    if (!boundary.empty() && boundary.front() == '"')
        boundary = boundary.substr(1, boundary.find('"', 1) - 1);
    while (!boundary.empty() && (isspace((unsigned char)boundary.back()) || boundary.back() == ';'))
        boundary.pop_back();

    string delim = "--" + boundary;
    string result;
    size_t pos = 0;

    while ((pos = body.find(delim, pos)) != string::npos) {
        pos += delim.size();
        if (pos < body.size() && body[pos] == '-') break; // closing boundary
        if (pos < body.size() && body[pos] == '\r') pos++;
        if (pos < body.size() && body[pos] == '\n') pos++;

        size_t nextDelim = body.find(delim, pos);
        if (nextDelim == string::npos) break;

        string part = body.substr(pos, nextDelim - pos);
        string ph, pb;
        splitHeadersBody(part, ph, pb);

        string pct  = getHeader(ph, "content-type");
        string penc = getHeader(ph, "content-transfer-encoding");
        for (auto& c : penc) c = tolower(c);

        if (pct.find("text/plain") != string::npos) {
            if (penc.find("quoted-printable") != string::npos)
                result += decodeQuotedPrintable(pb) + "\n";
            else if (penc.find("base64") == string::npos)
                result += pb + "\n";
        }
        pos = nextDelim;
    }
    return result;
}

// Reads one CRLF-terminated IMAP protocol line from the socket.
// buf holds leftover bytes from previous reads.
static string readLine(tcp_ssl_socket& socket, string& buf) {
    asio::read_until(socket, asio::dynamic_buffer(buf), "\r\n");
    size_t pos = buf.find("\r\n");
    string line = buf.substr(0, pos);
    buf.erase(0, pos + 2);
    return line;
}

// Reads exactly n bytes from the socket in one bulk call (asio::transfer_exactly),
// which is much faster than reading line-by-line for large email bodies.
static string readExactly(tcp_ssl_socket& socket, string& buf, size_t n) {
    if (buf.size() >= n) {
        string result = buf.substr(0, n);
        buf.erase(0, n);
        return result;
    }
    size_t already = buf.size();
    size_t need    = n - already;
    buf.resize(n);
    boost::system::error_code ec;
    asio::read(socket, asio::buffer(&buf[already], need), asio::transfer_exactly(need), ec);
    if (ec && ec != asio::error::eof)
        throw boost::system::system_error(ec);
    string result = buf.substr(0, n);
    buf.erase(0, n);
    return result;
}

// Sends an IMAP command and collects all response lines until the
// tagged line (e.g. "A01 OK" or "A01 NO") is received.
static vector<string> sendAndReceive(tcp_ssl_socket& socket,
                                     string& buf,
                                     const string& command,
                                     const string& tag)
{
    asio::write(socket, asio::buffer(command));
    vector<string> lines;
    while (true) {
        string line = readLine(socket, buf);
        lines.push_back(line);
        if (line.size() >= tag.size() && line.substr(0, tag.size()) == tag) break;
    }
    return lines;
}

SyncWorker::SyncWorker(QObject *parent) : QObject(parent) {}

void SyncWorker::doSync(const QString& email, const QString& appPassword) {
    try {
        // Set up a TLS-encrypted TCP connection to Gmail's IMAP server on port 993
        asio::io_context io_context;
        asio::ssl::context ssl_ctx(asio::ssl::context::tlsv12_client);
        ssl_ctx.set_default_verify_paths();

        asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("imap.gmail.com", "993");

        tcp_ssl_socket socket(io_context, ssl_ctx);
        asio::connect(socket.lowest_layer(), endpoints);
        socket.handshake(asio::ssl::stream_base::client);

        string buf;
        readLine(socket, buf); // discard server greeting

        // Authenticate
        string loginCmd = "A01 LOGIN " + email.toStdString()
                        + " " + appPassword.toStdString() + "\r\n";
        auto loginResp = sendAndReceive(socket, buf, loginCmd, "A01");
        if (loginResp.back().find("A01 OK") == string::npos) {
            emit syncError("Login failed. Check your Gmail address and App Password.");
            return;
        }

        // Open the inbox and read how many messages it contains
        auto selectResp = sendAndReceive(socket, buf, "A02 SELECT INBOX\r\n", "A02");
        int totalMessages = 0;
        for (const auto& line : selectResp) {
            if (line.find("EXISTS") != string::npos) {
                istringstream ss(line);
                string star, kw; int count;
                if (ss >> star >> count >> kw) totalMessages = count;
            }
        }

        if (totalMessages == 0) {
            sendAndReceive(socket, buf, "A04 LOGOUT\r\n", "A04");
            emit syncSuccess("");
            return;
        }

        // Fetch the last 5 messages. BODY.PEEK[] downloads the full raw message
        // without marking emails as read.
        int startMsg = max(1, totalMessages - 4);
        string fetchCmd = "A03 FETCH " + to_string(startMsg) + ":"
                        + to_string(totalMessages) + " (BODY.PEEK[])\r\n";
        asio::write(socket, asio::buffer(fetchCmd));

        QString allPlainText;

        while (true) {
            string line = readLine(socket, buf);
            if (line.size() >= 3 && line.substr(0, 3) == "A03") break;

            // IMAP signals a large data block with {N} at the end of a line,
            // meaning "the next N bytes are the message body".
            size_t braceOpen  = line.rfind('{');
            size_t braceClose = line.rfind('}');
            if (braceOpen != string::npos && braceClose != string::npos && braceClose > braceOpen) {
                string sizeStr = line.substr(braceOpen + 1, braceClose - braceOpen - 1);
                try {
                    size_t literalSize = stoul(sizeStr);
                    string rawMessage  = readExactly(socket, buf, literalSize);
                    string plainText   = extractPlainText(rawMessage);
                    if (!plainText.empty())
                        allPlainText += QString::fromStdString(plainText) + "\n";
                } catch (...) {}
            }
        }

        sendAndReceive(socket, buf, "A04 LOGOUT\r\n", "A04");
        emit syncSuccess(allPlainText);

    } catch (const exception& e) {
        emit syncError(QString("IMAP error: %1").arg(e.what()));
    }
}

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent), m_isRunning(false)
{
    // Run the blocking IMAP code on a separate thread so the GUI stays responsive
    m_thread = new QThread(this);
    m_worker = new SyncWorker();
    m_worker->moveToThread(m_thread);

    connect(m_worker, &SyncWorker::syncSuccess, this, &NetworkManager::syncSuccess);
    connect(m_worker, &SyncWorker::syncError,   this, &NetworkManager::syncError);
    connect(m_worker, &SyncWorker::syncSuccess, this, [this](const QString&) { m_isRunning = false; });
    connect(m_worker, &SyncWorker::syncError,   this, [this](const QString&) { m_isRunning = false; });
    connect(this, &NetworkManager::requestSync, m_worker, &SyncWorker::doSync);

    m_thread->start();
}

NetworkManager::~NetworkManager() {
    m_thread->quit();
    m_thread->wait();
    delete m_worker;
}

void NetworkManager::syncWithGmail(const QString& email, const QString& appPassword) {
    if (m_isRunning) { emit syncError("A sync is already in progress."); return; }
    m_isRunning = true;
    emit requestSync(email, appPassword);
}
