/*
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
    SPDX-FileContributor: Kevin Ottens <kevin@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "sessionpool.h"

#include <QTimer>
#include <QSslSocket>

#include "imapresource_debug.h"
#include <KLocalizedString>

#include <kimap/logoutjob.h>
#include <kimap/enablejob.h>

#include "imapaccount.h"
#include "passwordrequesterinterface.h"
#include "preparesessionjob.h"

qint64 SessionPool::m_requestCounter = 0;

SessionPool::SessionPool(int maxPoolSize, QObject *parent)
    : QObject(parent)
    , m_maxPoolSize(maxPoolSize)
{
}

SessionPool::~SessionPool()
{
    disconnect(CloseSession);
}

PasswordRequesterInterface *SessionPool::passwordRequester() const
{
    return m_passwordRequester;
}

void SessionPool::setPasswordRequester(PasswordRequesterInterface *requester)
{
    delete m_passwordRequester;

    m_passwordRequester = requester;
    m_passwordRequester->setParent(this);
    QObject::connect(m_passwordRequester, &PasswordRequesterInterface::done,
                     this, &SessionPool::onPasswordRequestDone);
}

void SessionPool::cancelPasswordRequests()
{
    m_passwordRequester->cancelPasswordRequests();
}

KIMAP::SessionUiProxy::Ptr SessionPool::sessionUiProxy() const
{
    return m_sessionUiProxy;
}

void SessionPool::setSessionUiProxy(KIMAP::SessionUiProxy::Ptr proxy)
{
    m_sessionUiProxy = proxy;
}

bool SessionPool::isConnected() const
{
    return m_initialConnectDone;
}

void SessionPool::requestPassword()
{
    if (m_account->authenticationMode() == KIMAP::LoginJob::GSSAPI) {
        // for GSSAPI we don't have to ask for username/password, because it uses session wide tickets
        QMetaObject::invokeMethod(this, "onPasswordRequestDone", Qt::QueuedConnection,
                                  Q_ARG(int, PasswordRequesterInterface::PasswordRetrieved),
                                  Q_ARG(QString, QString()));
    } else {
        m_passwordRequester->requestPassword();
    }
}

bool SessionPool::connect(ImapAccount *account)
{
    if (m_account) {
        return false;
    }

    m_account = account;
    requestPassword();

    return true;
}

void SessionPool::disconnect(SessionTermination termination)
{
    if (!m_account) {
        return;
    }

    foreach (KIMAP::Session *s, m_unusedPool + m_reservedPool + m_connectingPool) {
        killSession(s, termination);
    }
    m_unusedPool.clear();
    m_reservedPool.clear();
    m_connectingPool.clear();
    m_pendingInitialSession = nullptr;
    m_passwordRequester->cancelPasswordRequests();

    delete m_account;
    m_account = nullptr;
    m_namespaces.clear();
    m_capabilities.clear();
    m_effectiveCapabilities.clear();

    m_initialConnectDone = false;
    Q_EMIT disconnectDone();
}

qint64 SessionPool::requestSession()
{
    if (!m_initialConnectDone) {
        return -1;
    }

    qint64 requestNumber = ++m_requestCounter;

    // The queue was empty, so trigger the processing
    if (m_pendingRequests.isEmpty()) {
        QTimer::singleShot(0, this, &SessionPool::processPendingRequests);
    }

    m_pendingRequests << requestNumber;

    return requestNumber;
}

void SessionPool::cancelSessionRequest(qint64 id)
{
    Q_ASSERT(id > 0);
    m_pendingRequests.removeAll(id);
}

void SessionPool::releaseSession(KIMAP::Session *session)
{
    const int removeSession = m_reservedPool.removeAll(session);
    if (removeSession > 0) {
        m_unusedPool << session;
    }
}

ImapAccount *SessionPool::account() const
{
    return m_account;
}

QStringList SessionPool::serverCapabilities() const
{
    return m_capabilities;
}

QStringList SessionPool::effectiveServerCapabilities() const
{
    return m_effectiveCapabilities;
}

QList<KIMAP::MailBoxDescriptor> SessionPool::serverNamespaces() const
{
    return m_namespaces;
}

QList<KIMAP::MailBoxDescriptor> SessionPool::serverNamespaces(Namespace ns) const
{
    switch (ns) {
    case Personal:
        return m_personalNamespaces;
    case User:
        return m_userNamespaces;
    case Shared:
        return m_sharedNamespaces;
    default:
        break;
    }
    Q_ASSERT(false);
    return QList<KIMAP::MailBoxDescriptor>();
}

void SessionPool::killSession(KIMAP::Session *session, SessionTermination termination)
{
    Q_ASSERT(session);

    if (!m_unusedPool.contains(session) && !m_reservedPool.contains(session) && !m_connectingPool.contains(session)) {
        qCWarning(IMAPRESOURCE_LOG) << "Unmanaged session" << session;
        Q_ASSERT(false);
        return;
    }
    QObject::disconnect(session, &KIMAP::Session::connectionLost,
                        this, &SessionPool::onConnectionLost);
    m_unusedPool.removeAll(session);
    m_reservedPool.removeAll(session);
    m_connectingPool.removeAll(session);

    if (session->state() != KIMAP::Session::Disconnected && termination == LogoutSession) {
        KIMAP::LogoutJob *logout = new KIMAP::LogoutJob(session);
        QObject::connect(logout, &KJob::result,
                         session, &QObject::deleteLater);
        logout->start();
    } else {
        session->close();
        session->deleteLater();
    }
}

void SessionPool::declareSessionReady(KIMAP::Session *session)
{
    //This can happen if we happen to disconnect while capabilities and namespace are being retrieved,
    //resulting in us keeping a dangling pointer to a deleted session
    if (!m_connectingPool.contains(session)) {
        qCWarning(IMAPRESOURCE_LOG) << "Tried to declare a removed session ready";
        return;
    }

    m_pendingInitialSession = nullptr;

    if (!m_initialConnectDone) {
        m_initialConnectDone = true;
        Q_EMIT connectDone();
        // If the slot connected to connectDone() decided to disconnect the SessionPool
        // then we must end here, because we expect the pools to be empty now!
        if (!m_initialConnectDone) {
            return;
        }
    }

    m_connectingPool.removeAll(session);

    if (m_pendingRequests.isEmpty()) {
        m_unusedPool << session;
    } else {
        m_reservedPool << session;
        Q_EMIT sessionRequestDone(m_pendingRequests.takeFirst(), session);

        if (!m_pendingRequests.isEmpty()) {
            QTimer::singleShot(0, this, &SessionPool::processPendingRequests);
        }
    }
}

void SessionPool::cancelSessionCreation(KIMAP::Session *session, int errorCode, const QString &errorMessage)
{
    m_pendingInitialSession = nullptr;

    QString msg;
    if (m_account) {
        msg = i18n("Could not connect to the IMAP-server %1.\n%2", m_account->server(), errorMessage);
    } else {
        // Can happen when we lose all ready connections while trying to establish
        // a new connection, for example.
        msg = i18n("Could not connect to the IMAP server.\n%1", errorMessage);
    }

    if (!m_initialConnectDone) {
        disconnect(); // kills all sessions, including \a session
    } else {
        if (session) {
            killSession(session, LogoutSession);
        }
        if (!m_pendingRequests.isEmpty()) {
            Q_EMIT sessionRequestDone(m_pendingRequests.takeFirst(), nullptr, errorCode, errorMessage);
            if (!m_pendingRequests.isEmpty()) {
                QTimer::singleShot(0, this, &SessionPool::processPendingRequests);
            }
        }
    }
    // Always emit this at the end. This can call SessionPool::disconnect via ImapResource.
    Q_EMIT connectDone(errorCode, msg);
}

void SessionPool::processPendingRequests()
{
    if (!m_account) {
        // The connection to the server is lost; no point processing pending requests
        for (int request : qAsConst(m_pendingRequests)) {
            Q_EMIT sessionRequestDone(request, nullptr,
                                      LoginFailError, i18n("Disconnected from server during login."));
        }
        return;
    }

    if (!m_unusedPool.isEmpty()) {
        // We have a session ready to give out
        KIMAP::Session *session = m_unusedPool.takeFirst();
        m_reservedPool << session;
        if (!m_pendingRequests.isEmpty()) {
            Q_EMIT sessionRequestDone(m_pendingRequests.takeFirst(), session);
            if (!m_pendingRequests.isEmpty()) {
                QTimer::singleShot(0, this, &SessionPool::processPendingRequests);
            }
        }
    } else if (m_unusedPool.size() + m_reservedPool.size() < m_maxPoolSize) {
        // We didn't reach the max pool size yet so create a new one
        requestPassword();
    } else {
        // No session available, and max pool size reached
        if (!m_pendingRequests.isEmpty()) {
            Q_EMIT sessionRequestDone(
                m_pendingRequests.takeFirst(), nullptr, NoAvailableSessionError,
                i18n("Could not create another extra connection to the IMAP-server %1.",
                     m_account->server()));
            if (!m_pendingRequests.isEmpty()) {
                QTimer::singleShot(0, this, &SessionPool::processPendingRequests);
            }
        }
    }
}

void SessionPool::onPasswordRequestDone(int resultType, const QString &password)
{
    QString errorMessage;

    if (!m_account) {
        // it looks like the connection was lost while we were waiting
        // for the password, we should fail all the pending requests and stop there
        for (int request : qAsConst(m_pendingRequests)) {
            Q_EMIT sessionRequestDone(request, nullptr,
                                      LoginFailError, i18n("Disconnected from server during login."));
        }
        return;
    }

    switch (resultType) {
    case PasswordRequesterInterface::PasswordRetrieved:
        // All is fine
        break;
    case PasswordRequesterInterface::ReconnectNeeded:
        cancelSessionCreation(m_pendingInitialSession, ReconnectNeededError, errorMessage);
        return;
    case PasswordRequesterInterface::UserRejected:
        errorMessage = i18n("Could not read the password: user rejected wallet access");
        if (m_pendingInitialSession) {
            cancelSessionCreation(m_pendingInitialSession, LoginFailError, errorMessage);
        } else {
            Q_EMIT connectDone(PasswordRequestError, errorMessage);
        }
        return;
    case PasswordRequesterInterface::EmptyPasswordEntered:
        errorMessage = i18n("Empty password");
        if (m_pendingInitialSession) {
            cancelSessionCreation(m_pendingInitialSession, LoginFailError, errorMessage);
        } else {
            Q_EMIT connectDone(PasswordRequestError, errorMessage);
        }
        return;
    }

    if (m_account->encryptionMode() != KIMAP::LoginJob::Unencrypted && !QSslSocket::supportsSsl()) {
        qCWarning(IMAPRESOURCE_LOG) << "Crypto not supported!";
        Q_EMIT connectDone(EncryptionError,
                           i18n("You requested TLS/SSL to connect to %1, but your "
                                "system does not seem to be set up for that.", m_account->server()));
        disconnect();
        return;
    }

    KIMAP::Session *session = nullptr;
    if (m_pendingInitialSession) {
        session = m_pendingInitialSession;
    } else {
        session = new KIMAP::Session(m_account->server(), m_account->port(), this);
        QObject::connect(session, &QObject::destroyed, this, &SessionPool::onSessionDestroyed);
        session->setUiProxy(m_sessionUiProxy);
        session->setTimeout(m_account->timeout());
        session->setUseNetworkProxy(m_account->useNetworkProxy());
        m_connectingPool << session;
    }

    QObject::connect(session, &KIMAP::Session::connectionLost,
                     this, &SessionPool::onConnectionLost);

    KIMAP::LoginJob *loginJob = new KIMAP::LoginJob(session);
    loginJob->setUserName(m_account->userName());
    loginJob->setPassword(password);
    loginJob->setEncryptionMode(m_account->encryptionMode());
    loginJob->setAuthenticationMode(m_account->authenticationMode());

    QObject::connect(loginJob, &KJob::result,
                     this, &SessionPool::onLoginDone);
    loginJob->start();
}

void SessionPool::onLoginDone(KJob *job)
{
    KIMAP::LoginJob *login = static_cast<KIMAP::LoginJob *>(job);
    // Can happen if we disconnected meanwhile
    if (!m_connectingPool.contains(login->session())) {
        Q_EMIT connectDone(CancelledError, i18n("Disconnected from server during login."));
        return;
    }

    if (job->error() == 0) {
        if (m_initialConnectDone) {
            enableSessionCapabilities(login->session());
        } else {
            // On initial connection we need to prepare the session (and ourselves)
            auto *prepJob = new PrepareSessionJob(login->session(), m_clientId);
            QObject::connect(prepJob, &KJob::result, this, &SessionPool::onPrepareSessionDone);
            prepJob->start();
        }
    } else {
        if (job->error() == KIMAP::LoginJob::ERR_COULD_NOT_CONNECT) {
            if (m_account) {
                cancelSessionCreation(login->session(),
                                      CouldNotConnectError,
                                      i18n("Could not connect to the IMAP-server %1.\n%2",
                                           m_account->server(), job->errorString()));
            } else {
                // Can happen when we lose all ready connections while trying to login.
                cancelSessionCreation(login->session(),
                                      CouldNotConnectError,
                                      i18n("Could not connect to the IMAP-server.\n%1",
                                           job->errorString()));
            }
        } else {
            // Connection worked, but login failed -> ask for a different password or ssl settings.
            m_pendingInitialSession = login->session();
            m_passwordRequester->requestPassword(PasswordRequesterInterface::WrongPasswordRequest,
                                                 job->errorString());
        }
    }
}

void SessionPool::onPrepareSessionDone(KJob *job)
{
    auto *prepareJob = qobject_cast<PrepareSessionJob *>(job);

    // Can happen if we disconnected meanwhile
    if (!m_connectingPool.contains(prepareJob->session())) {
        Q_EMIT connectDone(CancelledError, i18n("Disconnected from server during login."));
        return;
    }

    if (job->error()) {
        QString msg;
        int code = KJob::NoError;
        switch (job->error()) {
        case PrepareSessionJob::CapabilitiesTestError:
            code = CapabilitiesTestError;
            msg = m_account ? i18n("Could not test the capabilities supported by the IMAP server %1.\n%2", m_account->server(), job->errorString())
                            : i18n("Could not test the capabilities supported by the IMAP server.\n%1", job->errorString());
            break;
        case PrepareSessionJob::NamespaceFetchError:
            code = job->error();
            msg = m_account ? i18n("Could not retrieve namespaces from the IMAP server %1.\n%2", m_account->server(), job->errorString())
                            : i18n("Could not retrieve namespaces from the IMAP server.\n%1", job->errorString());
            break;
        case PrepareSessionJob::IdentificationError:
            code = job->error();
            msg = m_account ? i18n("Could not send client ID to the IMAP server %1.\n%2", m_account->server(), job->errorString())
                            : i18n("Could not send client ID to the IMAP server.\n%1", job->errorString());
            break;
        default:
            code = job->error();
            msg = job->errorString();
        }

        cancelSessionCreation(prepareJob->session(), code, msg);
        return;
    }

    m_capabilities = prepareJob->capabilities();
    m_effectiveCapabilities = prepareJob->capabilities();
    m_namespaces = prepareJob->namespaces();
    m_personalNamespaces = prepareJob->personalNamespaces();
    m_userNamespaces = prepareJob->userNamespaces();
    m_sharedNamespaces = prepareJob->sharedNamespaces();

    QStringList missing;
    const QStringList expected = {QStringLiteral("IMAP4REV1")};
    for (const QString &capability : expected) {
        if (!m_capabilities.contains(capability)) {
            missing << capability;
        }
    }

    if (!missing.isEmpty()) {
        cancelSessionCreation(prepareJob->session(),
                              IncompatibleServerError,
                              i18n("Cannot use the IMAP server %1, "
                                   "some mandatory capabilities are missing: %2. "
                                   "Please ask your sysadmin to upgrade the server.",
                                   m_account->server(),
                                   missing.join(QLatin1String(", "))));
        return;
    }

    enableSessionCapabilities(prepareJob->session());
}

void SessionPool::enableSessionCapabilities(KIMAP::Session *session)
{
    QStringList capsToEnable;

    if (m_capabilities.contains(QStringView{u"CONDSTORE"})
        && m_capabilities.contains(QStringView{u"QRESYNC"})) {
        capsToEnable.push_back(QStringLiteral("QRESYNC"));
    }

    if (!capsToEnable.empty()) {
        qCDebug(IMAPRESOURCE_LOG) << "Capabilities to ENABLE on the server:" << capsToEnable;
        auto *job = new KIMAP::EnableJob(session);
        job->setCapabilities(capsToEnable);
        QObject::connect(job, &KJob::result, this, &SessionPool::onEnableDone);
        job->start();
    } else {
        declareSessionReady(session);
    }
}

void SessionPool::onEnableDone(KJob *job)
{
    auto *enableJob = qobject_cast<KIMAP::EnableJob *>(job);

    // Can happen if we disconnected meanwhile
    if (!m_connectingPool.contains(enableJob->session())) {
        Q_EMIT connectDone(CancelledError, i18n("Disconnected from server during login."));
        return;
    }

    if (!enableJob->enabledCapabilities().contains(QStringView{u"QRESYNC"})) {
        qCWarning(IMAPRESOURCE_LOG) << "Failed to enable QRESYNC!";
        m_effectiveCapabilities.removeOne(QStringLiteral("QRESYNC"));
    }

    declareSessionReady(enableJob->session());
}

void SessionPool::setClientId(const QByteArray &clientId)
{
    m_clientId = clientId;
}

void SessionPool::onConnectionLost()
{
    KIMAP::Session *session = static_cast<KIMAP::Session *>(sender());

    m_unusedPool.removeAll(session);
    m_reservedPool.removeAll(session);
    m_connectingPool.removeAll(session);

    if (m_unusedPool.isEmpty() && m_reservedPool.isEmpty()) {
        m_passwordRequester->cancelPasswordRequests();
        delete m_account;
        m_account = nullptr;
        m_namespaces.clear();
        m_capabilities.clear();
        m_effectiveCapabilities.clear();

        m_initialConnectDone = false;
    }

    Q_EMIT connectionLost(session);

    if (!m_pendingRequests.isEmpty()) {
        cancelSessionCreation(nullptr, CouldNotConnectError, QString());
    }

    session->deleteLater();
    if (session == m_pendingInitialSession) {
        m_pendingInitialSession = nullptr;
    }
}

void SessionPool::onSessionDestroyed(QObject *object)
{
    //Safety net for bugs that cause dangling session pointers
    KIMAP::Session *session = static_cast<KIMAP::Session *>(object);
    bool sessionInPool = false;
    if (m_unusedPool.contains(session)) {
        qCWarning(IMAPRESOURCE_LOG) << "Session" << object << "destroyed while still in unused pool!";
        m_unusedPool.removeAll(session);
        sessionInPool = true;
    }
    if (m_reservedPool.contains(session)) {
        qCWarning(IMAPRESOURCE_LOG) << "Session" << object << "destroyed while still in reserved pool!";
        m_reservedPool.removeAll(session);
        sessionInPool = true;
    }
    if (m_connectingPool.contains(session)) {
        qCWarning(IMAPRESOURCE_LOG) << "Session" << object << "destroyed while still in connecting pool!";
        m_connectingPool.removeAll(session);
        sessionInPool = true;
    }
    Q_ASSERT(!sessionInPool);
}
