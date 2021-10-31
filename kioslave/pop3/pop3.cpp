/*
 * SPDX-FileCopyrightText: 1999-2001 Alex Zepeda <zipzippy@sonic.net>
 * SPDX-FileCopyrightText: 2001-2002 Michael Haeckel <haeckel@kde.org>
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */
#include "pop3.h"

extern "C" {
#include <sasl/sasl.h>
}

#include "pop3_debug.h"
#include <QByteArray>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QNetworkProxy>
#include <QRegularExpression>

#include <KLocalizedString>

#include <KIO/SlaveInterface>

#include <string.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

#ifdef Q_OS_WIN
// See kio/src/core/kioglobal_p.h
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#endif

#define GREETING_BUF_LEN 1024
#define MAX_RESPONSE_LEN 512
#define MAX_COMMANDS 10

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.pop3" FILE "pop3.json")
};

extern "C" {
int Q_DECL_EXPORT kdemain(int argc, char **argv);
}

using namespace KIO;

static const sasl_callback_t callbacks[] = {
    {SASL_CB_ECHOPROMPT, nullptr, nullptr},
    {SASL_CB_NOECHOPROMPT, nullptr, nullptr},
    {SASL_CB_GETREALM, nullptr, nullptr},
    {SASL_CB_USER, nullptr, nullptr},
    {SASL_CB_AUTHNAME, nullptr, nullptr},
    {SASL_CB_PASS, nullptr, nullptr},
    {SASL_CB_CANON_USER, nullptr, nullptr},
    {SASL_CB_LIST_END, nullptr, nullptr},
};

static bool initSASL()
{
#ifdef Q_OS_WIN32 // krazy:exclude=cpp
#if 0
    QByteArray libInstallPath(QFile::encodeName(QDir::toNativeSeparators(KGlobal::dirs()->installPath("lib") + QLatin1String("sasl2"))));
    QByteArray configPath(QFile::encodeName(QDir::toNativeSeparators(KGlobal::dirs()->installPath("config") + QLatin1String("sasl2"))));
    if (sasl_set_path(SASL_PATH_TYPE_PLUGIN, libInstallPath.data()) != SASL_OK
        || sasl_set_path(SASL_PATH_TYPE_CONFIG, configPath.data()) != SASL_OK) {
        fprintf(stderr, "SASL path initialization failed!\n");
        return false;
    }
#endif
#endif

    if (sasl_client_init(nullptr) != SASL_OK) {
        fprintf(stderr, "SASL library initialization failed!\n");
        return false;
    }
    return true;
}

int kdemain(int argc, char **argv)
{
    if (argc != 4) {
        qCDebug(POP3_LOG) << "Usage: kio_pop3 protocol domain-socket1 domain-socket2";
        return -1;
    }
    QCoreApplication app(argc, argv); // needed for QSocketNotifier
    app.setApplicationName(QStringLiteral("kio_pop3"));

    if (!initSASL()) {
        return -1;
    }

    // Are we looking to use SSL?
    POP3Protocol *slave;
    if (strcasecmp(argv[1], "pop3s") == 0) {
        slave = new POP3Protocol(argv[2], argv[3], true);
    } else {
        slave = new POP3Protocol(argv[2], argv[3], false);
    }

    slave->dispatchLoop();
    delete slave;

    sasl_done();

    return 0;
}

POP3Protocol::POP3Protocol(const QByteArray &pool, const QByteArray &app, bool isSSL)
    : TCPSlaveBase((isSSL ? "pop3s" : "pop3"), pool, app, isSSL)
{
    qCDebug(POP3_LOG);
    // m_cmd = CMD_NONE;
    m_iOldPort = 0;
    supports_apop = false;
    m_try_apop = true;
    m_try_sasl = true;
    opened = false;
    readBufferLen = 0;
}

POP3Protocol::~POP3Protocol()
{
    qCDebug(POP3_LOG);
    closeConnection();
}

void POP3Protocol::setHost(const QString &_host, quint16 _port, const QString &_user, const QString &_pass)
{
    m_sServer = _host;
    m_iPort = _port;
    m_sUser = _user;
    m_sPass = _pass;
}

ssize_t POP3Protocol::myRead(void *data, ssize_t len)
{
    if (readBufferLen) {
        ssize_t copyLen = (len < readBufferLen) ? len : readBufferLen;
        memcpy(data, readBuffer, copyLen);
        readBufferLen -= copyLen;
        if (readBufferLen) {
            memmove(readBuffer, &readBuffer[copyLen], readBufferLen);
        }
        return copyLen;
    }
    waitForResponse(600);
    return read((char *)data, len);
}

ssize_t POP3Protocol::myReadLine(char *data, ssize_t len)
{
    ssize_t copyLen = 0, readLen = 0;
    while (true) {
        while (copyLen < readBufferLen && readBuffer[copyLen] != '\n') {
            copyLen++;
        }
        if (copyLen < readBufferLen || copyLen == len) {
            copyLen++;
            memcpy(data, readBuffer, copyLen);
            data[copyLen] = '\0';
            readBufferLen -= copyLen;
            if (readBufferLen) {
                memmove(readBuffer, &readBuffer[copyLen], readBufferLen);
            }
            return copyLen;
        }
        waitForResponse(600);
        readLen = read(&readBuffer[readBufferLen], len - readBufferLen);
        readBufferLen += readLen;
        if (readLen <= 0) {
            data[0] = '\0';
            return 0;
        }
    }
}

POP3Protocol::Resp POP3Protocol::getResponse(char *r_buf, unsigned int r_len)
{
    char *buf = nullptr;
    unsigned int recv_len = 0;
    // fd_set FDs;

    // Give the buffer the appropriate size
    r_len = r_len ? r_len : MAX_RESPONSE_LEN;

    buf = new char[r_len];

    // Clear out the buffer
    memset(buf, 0, r_len);
    myReadLine(buf, r_len - 1);
    // qCDebug(POP3_LOG) << "S:" << buf;

    // This is really a funky crash waiting to happen if something isn't
    // null terminated.
    recv_len = strlen(buf);

    /*
     *   From rfc1939:
     *
     *   Responses in the POP3 consist of a status indicator and a keyword
     *   possibly followed by additional information.  All responses are
     *   terminated by a CRLF pair.  Responses may be up to 512 characters
     *   long, including the terminating CRLF.  There are currently two status
     *   indicators: positive ("+OK") and negative ("-ERR").  Servers MUST
     *   send the "+OK" and "-ERR" in upper case.
     */

    if (strncmp(buf, "+OK", 3) == 0) {
        if (r_buf && r_len) {
            memcpy(r_buf, (buf[3] == ' ' ? buf + 4 : buf + 3), qMin(r_len, (buf[3] == ' ' ? recv_len - 4 : recv_len - 3)));
        }

        delete[] buf;

        return Ok;
    } else if (strncmp(buf, "-ERR", 4) == 0) {
        if (r_buf && r_len) {
            memcpy(r_buf, (buf[4] == ' ' ? buf + 5 : buf + 4), qMin(r_len, (buf[4] == ' ' ? recv_len - 5 : recv_len - 4)));
        }

        QString serverMsg = QString::fromLatin1(buf).mid(5).trimmed();

        m_sError = i18n("The server said: \"%1\"", serverMsg);

        delete[] buf;

        return Err;
    } else if (strncmp(buf, "+ ", 2) == 0) {
        if (r_buf && r_len) {
            memcpy(r_buf, buf + 2, qMin(r_len, recv_len - 4));
            r_buf[qMin(r_len - 1, recv_len - 4)] = '\0';
        }

        delete[] buf;

        return Cont;
    } else {
        qCDebug(POP3_LOG) << "Invalid POP3 response received!";

        if (r_buf && r_len) {
            memcpy(r_buf, buf, qMin(r_len, recv_len));
        }

        if (!*buf) {
            m_sError = i18n("The server terminated the connection.");
        } else {
            m_sError = i18n("Invalid response from server:\n\"%1\"", QLatin1String(buf));
        }

        delete[] buf;

        return Invalid;
    }
}

bool POP3Protocol::sendCommand(const QByteArray &cmd)
{
    /*
     *   From rfc1939:
     *
     *   Commands in the POP3 consist of a case-insensitive keyword, possibly
     *   followed by one or more arguments.  All commands are terminated by a
     *   CRLF pair.  Keywords and arguments consist of printable ASCII
     *   characters.  Keywords and arguments are each separated by a single
     *   SPACE character.  Keywords are three or four characters long. Each
     *   argument may be up to 40 characters long.
     */

    if (!isConnected()) {
        return false;
    }

    QByteArray cmdrn = cmd + "\r\n";

    // Show the command line the client sends, but make sure the password
    // doesn't show up in the debug output
    QByteArray debugCommand = cmd;
    if (!m_sPass.isEmpty()) {
        debugCommand.replace(m_sPass.toLatin1(), "<password>");
    }
    // qCDebug(POP3_LOG) << "C:" << debugCommand;

    // Now actually write the command to the socket
    if (write(cmdrn.data(), cmdrn.size()) != static_cast<ssize_t>(cmdrn.size())) {
        m_sError = i18n("Could not send to server.\n");
        return false;
    }

    return true;
}

POP3Protocol::Resp POP3Protocol::command(const QByteArray &cmd, char *recv_buf, unsigned int len)
{
    sendCommand(cmd);
    return getResponse(recv_buf, len);
}

void POP3Protocol::openConnection()
{
    m_try_apop = !hasMetaData(QStringLiteral("auth")) || metaData(QStringLiteral("auth")) == QLatin1String("APOP");
    m_try_sasl = !hasMetaData(QStringLiteral("auth")) || metaData(QStringLiteral("auth")) == QLatin1String("SASL");

    if (!pop3_open()) {
        qCDebug(POP3_LOG) << "pop3_open failed";
    } else {
        connected();
    }
}

void POP3Protocol::closeConnection()
{
    // If the file pointer exists, we can assume the socket is valid,
    // and to make sure that the server doesn't magically undo any of
    // our deletions and so-on, we should send a QUIT and wait for a
    // response.  We don't care if it's positive or negative.  Also
    // flush out any semblance of a persistent connection, i.e.: the
    // old username and password are now invalid.
    if (!opened) {
        return;
    }

    command("QUIT");
    disconnectFromHost();
    readBufferLen = 0;
    m_sOldUser = m_sOldPass = m_sOldServer = QLatin1String("");
    opened = false;
}

int POP3Protocol::loginAPOP(const char *challenge, KIO::AuthInfo &ai)
{
    char buf[512];

    QString apop_string = QStringLiteral("APOP ");
    if (m_sUser.isEmpty() || m_sPass.isEmpty()) {
        // Prompt for usernames
        const int errorCode = openPasswordDialogV2(ai);
        if (errorCode) {
            error(ERR_ABORTED, i18n("No authentication details supplied."));
            closeConnection();
            return -1;
        } else {
            m_sUser = ai.username;
            m_sPass = ai.password;
        }
    }
    m_sOldUser = m_sUser;
    m_sOldPass = m_sPass;

    apop_string.append(m_sUser);

    memset(buf, 0, sizeof(buf));

    QCryptographicHash ctx(QCryptographicHash::Md5);

    qCDebug(POP3_LOG) << "APOP challenge: " << challenge;

    // Generate digest
    ctx.addData(challenge, strlen(challenge));
    ctx.addData(m_sPass.toLatin1());

    // Genenerate APOP command
    apop_string.append(QLatin1Char(' '));
    apop_string.append(QLatin1String(ctx.result().toHex()));

    if (command(apop_string.toLocal8Bit(), buf, sizeof(buf)) == Ok) {
        return 0;
    }

    qCDebug(POP3_LOG) << "Could not login via APOP. Falling back to USER/PASS";
    closeConnection();
    if (metaData(QStringLiteral("auth")) == QLatin1String("APOP")) {
        error(ERR_CANNOT_LOGIN,
              i18n("Login via APOP failed. The server %1 may not support APOP, although it claims to support it, or the password may be wrong.\n\n%2",
                   m_sServer,
                   m_sError));
        return -1;
    }
    return 1;
}

bool POP3Protocol::saslInteract(void *in, AuthInfo &ai)
{
    qCDebug(POP3_LOG);
    auto interact = (sasl_interact_t *)in;

    // some mechanisms do not require username && pass, so don't need a popup
    // window for getting this info
    for (; interact->id != SASL_CB_LIST_END; interact++) {
        if (interact->id == SASL_CB_AUTHNAME || interact->id == SASL_CB_PASS) {
            if (m_sUser.isEmpty() || m_sPass.isEmpty()) {
                const int errorCode = openPasswordDialogV2(ai);
                if (errorCode) {
                    error(ERR_ABORTED, i18n("No authentication details supplied."));
                    return false;
                }
                m_sUser = ai.username;
                m_sPass = ai.password;
            }
            break;
        }
    }

    interact = (sasl_interact_t *)in;
    while (interact->id != SASL_CB_LIST_END) {
        qCDebug(POP3_LOG) << "SASL_INTERACT id: " << interact->id;
        switch (interact->id) {
        case SASL_CB_USER:
        case SASL_CB_AUTHNAME:
            qCDebug(POP3_LOG) << "SASL_CB_[USER|AUTHNAME]: " << m_sUser;
            interact->result = strdup(m_sUser.toUtf8().constData());
            interact->len = strlen((const char *)interact->result);
            break;
        case SASL_CB_PASS:
            qCDebug(POP3_LOG) << "SASL_CB_PASS: [hidden] ";
            interact->result = strdup(m_sPass.toUtf8().constData());
            interact->len = strlen((const char *)interact->result);
            break;
        default:
            interact->result = nullptr;
            interact->len = 0;
            break;
        }
        interact++;
    }
    return true;
}

#define SASLERROR                                                                                                                                              \
    closeConnection();                                                                                                                                         \
    error(ERR_CANNOT_AUTHENTICATE, i18n("An error occurred during authentication: %1", QString::fromUtf8(sasl_errdetail(conn))));

int POP3Protocol::loginSASL(KIO::AuthInfo &ai)
{
    char buf[512];
    QString sasl_buffer = QStringLiteral("AUTH");

    int result;
    sasl_conn_t *conn = nullptr;
    sasl_interact_t *client_interact = nullptr;
    const char *out = nullptr;
    uint outlen;
    const char *mechusing = nullptr;
    Resp resp;

    result = sasl_client_new("pop", m_sServer.toLatin1().constData(), nullptr, nullptr, callbacks, 0, &conn);

    if (result != SASL_OK) {
        qCDebug(POP3_LOG) << "sasl_client_new failed with: " << result;
        SASLERROR
        return false;
    }

    // We need to check what methods the server supports...
    // This is based on RFC 1734's wisdom
    if (hasMetaData(QStringLiteral("sasl")) || command(sasl_buffer.toLocal8Bit()) == Ok) {
        QStringList sasl_list;
        if (hasMetaData(QStringLiteral("sasl"))) {
            sasl_list.append(metaData(QStringLiteral("sasl")));
        } else {
            while (true /* !AtEOF() */) {
                memset(buf, 0, sizeof(buf));
                myReadLine(buf, sizeof(buf) - 1);

                // HACK: This assumes fread stops at the first \n and not \r
                if ((buf[0] == 0) || (strcmp(buf, ".\r\n") == 0)) {
                    break; // End of data
                }
                // sanders, changed -2 to -1 below
                buf[strlen(buf) - 2] = '\0';

                sasl_list.append(QLatin1String(buf));
            }
        }

        do {
            result = sasl_client_start(conn, sasl_list.join(QLatin1Char(' ')).toLatin1().constData(), &client_interact, &out, &outlen, &mechusing);

            if (result == SASL_INTERACT) {
                if (!saslInteract(client_interact, ai)) {
                    closeConnection();
                    sasl_dispose(&conn);
                    return -1;
                }
            }
        } while (result == SASL_INTERACT);
        if (result != SASL_CONTINUE && result != SASL_OK) {
            qCDebug(POP3_LOG) << "sasl_client_start failed with: " << result;
            SASLERROR
            sasl_dispose(&conn);
            return -1;
        }

        qCDebug(POP3_LOG) << "Preferred authentication method is " << mechusing << ".";

        QByteArray msg, tmp;

        QString firstCommand = QLatin1String("AUTH ") + QString::fromLatin1(mechusing);
        msg = QByteArray::fromRawData(out, outlen).toBase64();
        if (!msg.isEmpty()) {
            firstCommand += QLatin1Char(' ');
            firstCommand += QString::fromLatin1(msg.data(), msg.size());
        }

        tmp.resize(2049);
        resp = command(firstCommand.toLatin1(), tmp.data(), 2049);
        while (resp == Cont) {
            tmp.resize(tmp.indexOf((char)0));
            msg = QByteArray::fromBase64(tmp);
            do {
                result = sasl_client_step(conn, msg.isEmpty() ? nullptr : msg.data(), msg.size(), &client_interact, &out, &outlen);

                if (result == SASL_INTERACT) {
                    if (!saslInteract(client_interact, ai)) {
                        closeConnection();
                        sasl_dispose(&conn);
                        return -1;
                    }
                }
            } while (result == SASL_INTERACT);
            if (result != SASL_CONTINUE && result != SASL_OK) {
                qCDebug(POP3_LOG) << "sasl_client_step failed with: " << result;
                SASLERROR
                sasl_dispose(&conn);
                return -1;
            }

            msg = QByteArray::fromRawData(out, outlen).toBase64();
            tmp.resize(2049);
            resp = command(msg, tmp.data(), 2049);
        }

        sasl_dispose(&conn);
        if (resp == Ok) {
            qCDebug(POP3_LOG) << "SASL authenticated";
            m_sOldUser = m_sUser;
            m_sOldPass = m_sPass;
            return 0;
        }

        if (metaData(QStringLiteral("auth")) == QLatin1String("SASL")) {
            closeConnection();
            error(ERR_CANNOT_LOGIN,
                  i18n("Login via SASL (%1) failed. The server may not support %2, or the password may be wrong.\n\n%3",
                       QLatin1String(mechusing),
                       QLatin1String(mechusing),
                       m_sError));
            return -1;
        }
    }

    if (metaData(QStringLiteral("auth")) == QLatin1String("SASL")) {
        closeConnection();
        error(ERR_CANNOT_LOGIN,
              i18n("Your POP3 server (%1) does not support SASL.\n"
                   "Choose a different authentication method.",
                   m_sServer));
        return -1;
    }
    return 1;
}

bool POP3Protocol::loginPASS(KIO::AuthInfo &ai)
{
    char buf[512];

    if (m_sUser.isEmpty() || m_sPass.isEmpty()) {
        // Prompt for usernames
        const int errorCode = openPasswordDialogV2(ai);
        if (errorCode) {
            error(ERR_ABORTED, i18n("No authentication details supplied."));
            closeConnection();
            return false;
        } else {
            m_sUser = ai.username;
            m_sPass = ai.password;
        }
    }
    m_sOldUser = m_sUser;
    m_sOldPass = m_sPass;

    QString one_string = QStringLiteral("USER ");
    one_string.append(m_sUser);

    if (command(one_string.toLocal8Bit(), buf, sizeof(buf)) != Ok) {
        qCDebug(POP3_LOG) << "Could not login. Bad username Sorry";

        m_sError = i18n("Could not login to %1.\n\n", m_sServer) + m_sError;
        error(ERR_CANNOT_LOGIN, m_sError);
        closeConnection();

        return false;
    }

    one_string = QStringLiteral("PASS ");
    one_string.append(m_sPass);

    if (command(one_string.toLocal8Bit(), buf, sizeof(buf)) != Ok) {
        qCDebug(POP3_LOG) << "Could not login. Bad password Sorry.";
        m_sError = i18n("Could not login to %1. The password may be wrong.\n\n%2", m_sServer, m_sError);
        error(ERR_CANNOT_LOGIN, m_sError);
        closeConnection();
        return false;
    }
    qCDebug(POP3_LOG) << "USER/PASS login succeeded";
    return true;
}

bool POP3Protocol::pop3_open()
{
    qCDebug(POP3_LOG);
    char *greeting_buf;
    if ((m_iOldPort == m_iPort) && (m_sOldServer == m_sServer) && (m_sOldUser == m_sUser) && (m_sOldPass == m_sPass)) {
        qCDebug(POP3_LOG) << "Reusing old connection";
        return true;
    }

    if (!hasMetaData(QStringLiteral("useProxy")) || metaData(QStringLiteral("useProxy")) != QLatin1String("on")) {
        qCDebug(POP3_LOG) << "requested to use no proxy";

        QNetworkProxy proxy;
        proxy.setType(QNetworkProxy::NoProxy);
        if (auto sock = tcpSocket()) {
            sock->setProxy(proxy);
        } else {
            qCWarning(POP3_LOG) << "no socket, cannot set no proxy";
        }
    }

    do {
        closeConnection();

        if (!connectToHost((isAutoSsl() ? QStringLiteral("pop3s") : QStringLiteral("pop3")), m_sServer, m_iPort)) {
            // error(ERR_CANNOT_CONNECT, m_sServer);
            // ConnectToHost has already send an error message.
            return false;
        }
        opened = true;

        greeting_buf = new char[GREETING_BUF_LEN];
        memset(greeting_buf, 0, GREETING_BUF_LEN);

        // If the server doesn't respond with a greeting
        if (getResponse(greeting_buf, GREETING_BUF_LEN) != Ok) {
            m_sError = i18n("Could not login to %1.\n\n", m_sServer)
                + ((!greeting_buf || !*greeting_buf) ? i18n("The server terminated the connection immediately.")
                                                     : i18n("Server does not respond properly:\n%1\n", QLatin1String(greeting_buf)));
            error(ERR_CANNOT_LOGIN, m_sError);
            delete[] greeting_buf;
            closeConnection();
            return false; // we've got major problems, and possibly the
            // wrong port
        }
        QString greeting = QLatin1String(greeting_buf);
        delete[] greeting_buf;

        if (!greeting.isEmpty()) {
            greeting.chop(2);
        }

        // Does the server support APOP?
        // QString apop_cmd;
        const QRegularExpression re(QStringLiteral("<[A-Za-z0-9\\.\\-_]+@[A-Za-z0-9\\.\\-_]+>$"), QRegularExpression::CaseInsensitiveOption);

        qCDebug(POP3_LOG) << "greeting: " << greeting;
        int apop_pos = greeting.indexOf(re);
        supports_apop = (bool)(apop_pos != -1);

        if (metaData(QStringLiteral("nologin")) == QLatin1String("on")) {
            return true;
        }

        if (metaData(QStringLiteral("auth")) == QLatin1String("APOP") && !supports_apop) {
            error(ERR_CANNOT_LOGIN,
                  i18n("Your POP3 server (%1) does not support APOP.\n"
                       "Choose a different authentication method.",
                       m_sServer));
            closeConnection();
            return false;
        }

        m_iOldPort = m_iPort;
        m_sOldServer = m_sServer;

        // Try to go into TLS mode
        if ((metaData(QStringLiteral("tls")) == QLatin1String("on") /*### || (canUseTLS() &&
                                     metaData("tls") != "off")*/)
            && command("STLS") == Ok) {
            if (startSsl()) {
                qCDebug(POP3_LOG) << "TLS mode has been enabled.";
            } else {
                qCDebug(POP3_LOG) << "TLS mode setup has failed. Aborting.";
                error(ERR_SLAVE_DEFINED,
                      i18n("Your POP3 server claims to "
                           "support TLS but negotiation "
                           "was unsuccessful.\nYou can "
                           "disable TLS in the POP account settings dialog."));
                closeConnection();
                return false;
            }
        } else if (metaData(QStringLiteral("tls")) == QLatin1String("on")) {
            error(ERR_SLAVE_DEFINED,
                  i18n("Your POP3 server (%1) does not support TLS. Disable "
                       "TLS, if you want to connect without encryption.",
                       m_sServer));
            closeConnection();
            return false;
        }

        KIO::AuthInfo authInfo;
        authInfo.username = m_sUser;
        authInfo.password = m_sPass;
        authInfo.prompt = i18n("Username and password for your POP3 account:");

        if (supports_apop && m_try_apop) {
            qCDebug(POP3_LOG) << "Trying APOP";
            int retval = loginAPOP(greeting.toLatin1().data() + apop_pos, authInfo);
            switch (retval) {
            case 0:
                return true;
            case -1:
                return false;
            default:
                m_try_apop = false;
            }
        } else if (m_try_sasl) {
            qCDebug(POP3_LOG) << "Trying SASL";
            int retval = loginSASL(authInfo);
            switch (retval) {
            case 0:
                return true;
            case -1:
                return false;
            default:
                m_try_sasl = false;
            }
        } else {
            // Fall back to conventional USER/PASS scheme
            qCDebug(POP3_LOG) << "Trying USER/PASS";
            return loginPASS(authInfo);
        }
    } while (true);
}

size_t POP3Protocol::realGetSize(unsigned int msg_num)
{
    char *buf;
    QByteArray cmd;
    size_t ret = 0;

    buf = new char[MAX_RESPONSE_LEN];
    memset(buf, 0, MAX_RESPONSE_LEN);
    cmd = "LIST " + QByteArray::number(msg_num);
    if (command(cmd, buf, MAX_RESPONSE_LEN) != Ok) {
        delete[] buf;
        return 0;
    } else {
        cmd = buf;
        cmd.remove(0, cmd.indexOf(" "));
        ret = cmd.toLong();
    }
    delete[] buf;
    return ret;
}

void POP3Protocol::get(const QUrl &url)
{
    // List of supported commands
    //
    // URI                                 Command   Result
    // pop3://user:pass@domain/index       LIST      List message sizes
    // pop3://user:pass@domain/uidl        UIDL      List message UIDs
    // pop3://user:pass@domain/remove/#1   DELE #1   Mark a message for deletion
    // pop3://user:pass@domain/download/#1 RETR #1   Get message header and body
    // pop3://user:pass@domain/list/#1     LIST #1   Get size of a message
    // pop3://user:pass@domain/uid/#1      UIDL #1   Get UID of a message
    // pop3://user:pass@domain/commit      QUIT      Delete marked messages
    // pop3://user:pass@domain/headers/#1  TOP #1    Get header of message
    //
    // Notes:
    // Sizes are in bytes.
    // No support for the STAT command has been implemented.
    // commit closes the connection to the server after issuing the QUIT command.

    bool ok = true;
    char buf[MAX_PACKET_LEN];
    char destbuf[MAX_PACKET_LEN];
    QString cmd, path = url.path();
    int maxCommands = (metaData(QStringLiteral("pipelining")) == QLatin1String("on")) ? MAX_COMMANDS : 1;

    if (path.at(0) == QLatin1Char('/')) {
        path.remove(0, 1);
    }
    if (path.isEmpty()) {
        qCDebug(POP3_LOG) << "We should be a dir!!";
        error(ERR_IS_DIRECTORY, url.url());
        // m_cmd = CMD_NONE;
        return;
    }

    if (((path.indexOf(QLatin1Char('/')) == -1) && (path != QLatin1String("index")) && (path != QLatin1String("uidl")) && (path != QLatin1String("commit")))) {
        error(ERR_MALFORMED_URL, url.url());
        // m_cmd = CMD_NONE;
        return;
    }

    cmd = path.left(path.indexOf(QLatin1Char('/')));
    path.remove(0, path.indexOf(QLatin1Char('/')) + 1);

    if (!pop3_open()) {
        qCDebug(POP3_LOG) << "pop3_open failed";
        error(ERR_CANNOT_CONNECT, m_sServer);
        return;
    }

    if ((cmd == QLatin1String("index")) || (cmd == QLatin1String("uidl"))) {
        unsigned long size = 0;
        bool result;

        if (cmd == QLatin1String("index")) {
            result = (command("LIST") == Ok);
        } else {
            result = (command("UIDL") == Ok);
        }

        /*
           LIST
           +OK Mailbox scan listing follows
           1 2979
           2 1348
           .
         */
        if (result) {
            mimeType(QStringLiteral("text/plain"));
            while (true /* !AtEOF() */) {
                memset(buf, 0, sizeof(buf));
                myReadLine(buf, sizeof(buf) - 1);

                // HACK: This assumes fread stops at the first \n and not \r
                if ((buf[0] == 0) || (strcmp(buf, ".\r\n") == 0)) {
                    break; // End of data
                }
                // sanders, changed -2 to -1 below
                int bufStrLen = strlen(buf);
                buf[bufStrLen - 2] = '\0';
                size += bufStrLen;
                data(QByteArray::fromRawData(buf, bufStrLen));
                totalSize(size);
            }
        }
        qCDebug(POP3_LOG) << "Finishing up list";
        data(QByteArray());
        finished();
    } else if (cmd == QLatin1String("remove")) {
        const QStringList waitingCommands = path.split(QLatin1Char(','));
        int activeCommands = 0;
        QStringList::ConstIterator it = waitingCommands.begin();
        while (it != waitingCommands.end() || activeCommands > 0) {
            while (activeCommands < maxCommands && it != waitingCommands.end()) {
                sendCommand((QLatin1String("DELE ") + *it).toLatin1());
                activeCommands++;
                it++;
            }
            getResponse(buf, sizeof(buf) - 1);
            activeCommands--;
        }
        finished();
        // m_cmd = CMD_NONE;
    } else if (cmd == QLatin1String("download") || cmd == QLatin1String("headers")) {
        const QStringList waitingCommands = path.split(QLatin1Char(','), Qt::SkipEmptyParts);
        if (waitingCommands.isEmpty()) {
            qCDebug(POP3_LOG) << "tried to request" << cmd << "for" << path << "with no specific item to get";
            closeConnection();
            error(ERR_INTERNAL, m_sServer);
            return;
        }
        bool noProgress = (metaData(QStringLiteral("progress")) == QLatin1String("off") || waitingCommands.count() > 1);
        int p_size = 0;
        unsigned int msg_len = 0;
        QString list_cmd(QStringLiteral("LIST "));
        list_cmd += path;
        memset(buf, 0, sizeof(buf));
        if (!noProgress) {
            if (command(list_cmd.toLatin1(), buf, sizeof(buf) - 1) == Ok) {
                list_cmd = QLatin1String(buf);
                // We need a space, otherwise we got an invalid reply
                if (!list_cmd.indexOf(QLatin1Char(' '))) {
                    qCDebug(POP3_LOG) << "List command needs a space? " << list_cmd;
                    closeConnection();
                    error(ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
                    return;
                }
                list_cmd.remove(0, list_cmd.indexOf(QLatin1Char(' ')) + 1);
                msg_len = list_cmd.toUInt(&ok);
                if (!ok) {
                    qCDebug(POP3_LOG) << "LIST command needs to return a number? :" << list_cmd << QLatin1String(":");
                    closeConnection();
                    error(ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
                    return;
                }
            } else {
                closeConnection();
                error(ERR_SLAVE_DEFINED,
                      i18n("Error during communication with the POP3 server while "
                           "trying to list mail: %1",
                           m_sError));
                return;
            }
        }

        int activeCommands = 0;
        QStringList::ConstIterator it = waitingCommands.begin();
        bool firstCommand = true;
        while (it != waitingCommands.end() || activeCommands > 0) {
            while (activeCommands < maxCommands && it != waitingCommands.end()) {
                sendCommand(QString((cmd == QLatin1String("headers")) ? QString(QLatin1String("TOP ") + *it + QLatin1String(" 0"))
                                                                      : QString(QLatin1String("RETR ") + *it))
                                .toLatin1());
                activeCommands++;
                it++;
            }
            if (getResponse(buf, sizeof(buf) - 1) == Ok) {
                activeCommands--;
                if (firstCommand) {
                    firstCommand = false;
                    mimeType(QStringLiteral("message/rfc822"));
                }
                totalSize(msg_len);
                memset(buf, 0, sizeof(buf));
                char ending = '\n';
                bool endOfMail = false;
                bool eat = false;
                while (true /* !AtEOF() */) {
                    ssize_t readlen = myRead(buf, sizeof(buf) - 1);
                    if (readlen <= 0) {
                        if (isConnected()) {
                            error(ERR_SERVER_TIMEOUT, m_sServer);
                        } else {
                            error(ERR_CONNECTION_BROKEN, m_sServer);
                        }
                        closeConnection();
                        return;
                    }
                    if (ending == '.' && readlen > 1 && buf[0] == '\r' && buf[1] == '\n') {
                        readBufferLen = readlen - 2;
                        memcpy(readBuffer, &buf[2], readBufferLen);
                        break;
                    }
                    bool newline = (ending == '\n');

                    if (buf[readlen - 1] == '\n') {
                        ending = '\n';
                    } else if (buf[readlen - 1] == '.' && ((readlen > 1) ? buf[readlen - 2] == '\n' : ending == '\n')) {
                        ending = '.';
                    } else {
                        ending = ' ';
                    }

                    char *buf1 = buf, *buf2 = destbuf;
                    // ".." at start of a line means only "."
                    // "." means end of data
                    for (ssize_t i = 0; i < readlen; i++) {
                        if (*buf1 == '\r' && eat) {
                            endOfMail = true;
                            if (i == readlen - 1 /* && !AtEOF() */) {
                                myRead(buf, 1);
                            } else if (i < readlen - 2) {
                                readBufferLen = readlen - i - 2;
                                memcpy(readBuffer, &buf[i + 2], readBufferLen);
                            }
                            break;
                        } else if (*buf1 == '\n') {
                            newline = true;
                            eat = false;
                        } else if (*buf1 == '.' && newline) {
                            newline = false;
                            eat = true;
                        } else {
                            newline = false;
                            eat = false;
                        }
                        if (!eat) {
                            *buf2 = *buf1;
                            buf2++;
                        }
                        buf1++;
                    }

                    if (buf2 > destbuf) {
                        data(QByteArray::fromRawData(destbuf, buf2 - destbuf));
                    }

                    if (endOfMail) {
                        break;
                    }

                    if (!noProgress) {
                        p_size += readlen;
                        processedSize(p_size);
                    }
                }
                infoMessage(QStringLiteral("message complete"));
            } else {
                qCDebug(POP3_LOG) << "Could not login. Bad RETR Sorry";
                closeConnection();
                error(ERR_SLAVE_DEFINED,
                      i18n("Error during communication with the POP3 server while "
                           "trying to download mail: %1",
                           m_sError));
                return;
            }
        }
        qCDebug(POP3_LOG) << "Finishing up";
        data(QByteArray());
        finished();
    } else if ((cmd == QLatin1String("uid")) || (cmd == QLatin1String("list"))) {
        // QString qbuf;
        (void)path.toInt(&ok);

        if (!ok) {
            return; //  We fscking need a number!
        }

        if (cmd == QLatin1String("uid")) {
            path.prepend(QLatin1String("UIDL "));
        } else {
            path.prepend(QLatin1String("LIST "));
        }

        memset(buf, 0, sizeof(buf));
        if (command(path.toLatin1(), buf, sizeof(buf) - 1) == Ok) {
            const int len = strlen(buf);
            mimeType(QStringLiteral("text/plain"));
            totalSize(len);
            data(QByteArray::fromRawData(buf, len));
            processedSize(len);
            qCDebug(POP3_LOG) << buf;
            qCDebug(POP3_LOG) << "Finishing up uid";
            data(QByteArray());
            finished();
        } else {
            closeConnection();
            error(ERR_INTERNAL, i18n("Unexpected response from POP3 server."));
            return;
        }
    } else if (cmd == QLatin1String("commit")) {
        qCDebug(POP3_LOG) << "Issued QUIT";
        closeConnection();
        finished();
        // m_cmd = CMD_NONE;
        return;
    }
}

void POP3Protocol::listDir(const QUrl &url)
{
    Q_UNUSED(url)

    bool isINT;
    int num_messages = 0;
    QByteArray q_buf(MAX_RESPONSE_LEN, 0);

    // Try and open a connection
    if (!pop3_open()) {
        qCDebug(POP3_LOG) << "pop3_open failed";
        error(ERR_CANNOT_CONNECT, m_sServer);
        return;
    }
    // Check how many messages we have. STAT is by law required to
    // at least return +OK num_messages total_size
    if (command("STAT", q_buf.data(), MAX_RESPONSE_LEN) != Ok) {
        error(ERR_INTERNAL, i18n("The POP3 command 'STAT' failed"));
        return;
    }
    qCDebug(POP3_LOG) << "The stat buf is :" << q_buf << ":";
    if (q_buf.indexOf(" ") == -1) {
        error(ERR_INTERNAL, i18n("Invalid POP3 response, should have at least one space."));
        closeConnection();
        return;
    }
    q_buf.remove(q_buf.indexOf(" "), q_buf.length());

    num_messages = q_buf.toUInt(&isINT);
    if (!isINT) {
        error(ERR_INTERNAL, i18n("Invalid POP3 STAT response."));
        closeConnection();
        return;
    }
    UDSEntry entry;
    QString fname;
    for (int i = 0; i < num_messages; i++) {
        entry.reserve(6);
        fname = QStringLiteral("Message %1");

        entry.fastInsert(KIO::UDSEntry::UDS_NAME, fname.arg(i + 1));
        entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QStringLiteral("text/plain"));

        QUrl uds_url;
        if (isAutoSsl()) {
            uds_url.setScheme(QStringLiteral("pop3s"));
        } else {
            uds_url.setScheme(QStringLiteral("pop3"));
        }

        uds_url.setUserName(m_sUser);
        uds_url.setPassword(m_sPass);
        uds_url.setHost(m_sServer);
        uds_url.setPath(QStringLiteral("/download/%1").arg(i + 1));
        entry.fastInsert(KIO::UDSEntry::UDS_URL, uds_url.url());

        entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
        entry.fastInsert(KIO::UDSEntry::UDS_SIZE, realGetSize(i + 1));
        entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IXUSR | S_IWUSR);

        listEntry(entry);
        entry.clear();
    }

    finished();
}

void POP3Protocol::stat(const QUrl &url)
{
    QString _path = url.path();

    if (_path.at(0) == QLatin1Char('/')) {
        _path.remove(0, 1);
    }

    UDSEntry entry;
    entry.reserve(3);
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, _path);
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
    entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QStringLiteral("message/rfc822"));

    // TODO: maybe get the size of the message?
    statEntry(entry);

    finished();
}

void POP3Protocol::del(const QUrl &url, bool /*isfile */)
{
    QString invalidURI;
    bool isInt;

    if (!pop3_open()) {
        qCDebug(POP3_LOG) << "pop3_open failed";
        error(ERR_CANNOT_CONNECT, m_sServer);
        return;
    }

    QString _path = url.path();
    if (_path.at(0) == QLatin1Char('/')) {
        _path.remove(0, 1);
    }

    _path.toUInt(&isInt);
    if (!isInt) {
        invalidURI = _path;
    } else {
        _path.prepend(QLatin1String("DELE "));
        if (command(_path.toLatin1()) != Ok) {
            invalidURI = _path;
        }
    }

    qCDebug(POP3_LOG) << "Path:" << _path;
    finished();
}

#include "pop3.moc"
