#include "networkmanager.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>

using namespace std;
namespace asio = boost::asio;
using tcp_ssl_socket = asio::ssl::stream<asio::ip::tcp::socket>;

// ─── Quoted-Printable decoder ─────────────────────────────────────────────────
static string decodeQuotedPrintable(const string& input) {
    string result;
    result.reserve(input.size());
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] != '=') { result += input[i++]; continue; }
        // Soft line break: =\n or =\r\n
        if (i+1 < input.size() && input[i+1] == '\n')              { i += 2; continue; }
        if (i+2 < input.size() && input[i+1]=='\r' && input[i+2]=='\n') { i += 3; continue; }
        // =XX hex byte
        if (i+2 < input.size()
            && isxdigit((unsigned char)input[i+1])
            && isxdigit((unsigned char)input[i+2])) {
            result += static_cast<char>(stoul(string{input[i+1],input[i+2]}, nullptr, 16));
            i += 3; continue;
        }
        result += input[i++]; // lone '='
    }
    return result;
}

// ─── MIME plain-text extractor ────────────────────────────────────────────────
static void splitHeadersBody(const string& block, string& headers, string& body) {
    size_t sep = block.find("\r\n\r\n");
    size_t sepLen = 4;
    if (sep == string::npos) { sep = block.find("\n\n"); sepLen = 2; }
    if (sep == string::npos) { headers = ""; body = block; return; }
    headers = block.substr(0, sep);
    body    = block.substr(sep + sepLen);
}

static string getHeader(const string& headers, const string& key) {
    istringstream ss(headers);
    string line, lkey = key;
    for (auto& c : lkey) c = tolower(c);
    while (getline(ss, line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();
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

static string extractPlainText(const string& rawMessage) {
    string headers, body;
    splitHeadersBody(rawMessage, headers, body);

    string ct  = getHeader(headers, "content-type");
    string enc = getHeader(headers, "content-transfer-encoding");
    for (auto& c : enc) c = tolower(c);

    // ── Non-multipart ──────────────────────────────────────────────────────────
    if (ct.find("multipart") == string::npos) {
        if (ct.find("text/plain") == string::npos && !ct.empty()) return "";
        if (enc.find("quoted-printable") != string::npos) return decodeQuotedPrintable(body);
        if (enc.find("base64")           != string::npos) return "";
        return body;
    }

    // ── Multipart: find boundary ───────────────────────────────────────────────
    size_t bpos = ct.find("boundary=");
    if (bpos == string::npos) return "";
    string boundary = ct.substr(bpos + 9);
    if (!boundary.empty() && boundary.front() == '"')
        boundary = boundary.substr(1, boundary.find('"', 1) - 1);
    while (!boundary.empty() && (isspace((unsigned char)boundary.back()) || boundary.back()==';'))
        boundary.pop_back();

    string delim = "--" + boundary;
    string result;
    size_t pos = 0;

    while ((pos = body.find(delim, pos)) != string::npos) {
        pos += delim.size();
        if (pos < body.size() && body[pos] == '-') break;   // closing --boundary--
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

// ─── Low-level IMAP helpers ───────────────────────────────────────────────────

// Read one CRLF-terminated line from the socket, using buf as a carry buffer.
// This does ONE read_until call per line — acceptable because these are short
// protocol lines (not large data blocks).
static string readLine(tcp_ssl_socket& socket, string& buf) {
    asio::read_until(socket, asio::dynamic_buffer(buf), "\r\n");
    size_t pos = buf.find("\r\n");
    string line = buf.substr(0, pos);
    buf.erase(0, pos + 2);
    return line;
}

// Read exactly n bytes in as few round-trips as possible using
// asio::read() with transfer_exactly.  This is the key fix:
// instead of calling read_until once per line inside the literal,
// we do a single bulk read for the entire block.
static string readExactly(tcp_ssl_socket& socket, string& buf, size_t n) {
    // First drain what's already in buf
    if (buf.size() >= n) {
        string result = buf.substr(0, n);
        buf.erase(0, n);
        return result;
    }
    // Need more bytes — read the remainder in one shot
    size_t already = buf.size();
    size_t need    = n - already;
    buf.resize(n);   // make room
    boost::system::error_code ec;
    asio::read(socket,
               asio::buffer(&buf[already], need),
               asio::transfer_exactly(need),
               ec);
    if (ec && ec != asio::error::eof)
        throw boost::system::system_error(ec);
    string result = buf.substr(0, n);
    buf.erase(0, n);
    return result;
}

// Send a command and read lines until the tagged response is seen.
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

// ─── SyncWorker::doSync ───────────────────────────────────────────────────────

SyncWorker::SyncWorker(QObject *parent) : QObject(parent) {}

void SyncWorker::doSync(const QString& email, const QString& appPassword) {
    try {
        asio::io_context io_context;
        asio::ssl::context ssl_ctx(asio::ssl::context::tlsv12_client);
        ssl_ctx.set_default_verify_paths();

        asio::ip::tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("imap.gmail.com", "993");

        tcp_ssl_socket socket(io_context, ssl_ctx);
        asio::connect(socket.lowest_layer(), endpoints);
        socket.handshake(asio::ssl::stream_base::client);

        string buf;

        // Consume server greeting
        readLine(socket, buf);

        // LOGIN
        string loginCmd = "A01 LOGIN " + email.toStdString()
                        + " " + appPassword.toStdString() + "\r\n";
        auto loginResp = sendAndReceive(socket, buf, loginCmd, "A01");
        if (loginResp.back().find("A01 OK") == string::npos) {
            emit syncError("Login failed. Check your Gmail address and App Password.");
            return;
        }

        // SELECT INBOX
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

        // FETCH last 5 messages — full raw messages, without marking as read
        int startMsg = max(1, totalMessages - 4);
        string fetchCmd = "A03 FETCH " + to_string(startMsg) + ":"
                        + to_string(totalMessages) + " (BODY.PEEK[])\r\n";
        asio::write(socket, asio::buffer(fetchCmd));

        QString allPlainText;

        while (true) {
            string line = readLine(socket, buf);

            // Tagged response → FETCH is complete
            if (line.size() >= 3 && line.substr(0, 3) == "A03") break;

            // Literal block: server announces it as {N} at end of line
            size_t braceOpen  = line.rfind('{');
            size_t braceClose = line.rfind('}');
            if (braceOpen  != string::npos &&
                braceClose != string::npos &&
                braceClose  > braceOpen)
            {
                string sizeStr = line.substr(braceOpen + 1, braceClose - braceOpen - 1);
                try {
                    size_t literalSize = stoul(sizeStr);

                    // ── KEY FIX: read all N bytes in one bulk call ─────────────
                    string rawMessage = readExactly(socket, buf, literalSize);

                    string plainText = extractPlainText(rawMessage);
                    if (!plainText.empty()) {
                        allPlainText += QString::fromStdString(plainText) + "\n";
                    }
                } catch (...) {}
            }
            // Other lines (FETCH metadata, closing parenthesis) are ignored
        }

        sendAndReceive(socket, buf, "A04 LOGOUT\r\n", "A04");
        emit syncSuccess(allPlainText);

    } catch (const exception& e) {
        emit syncError(QString("IMAP error: %1").arg(e.what()));
    }
}

// ─── NetworkManager ──────────────────────────────────────────────────────────

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent), m_isRunning(false)
{
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
