/*
    Copyright (c) 2010 Klarälvdalens Datakonsult AB,
                       a KDAB Group company <info@kdab.com>
    Author: Kevin Ottens <kevin@kdab.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef SESSIONPOOL_H
#define SESSIONPOOL_H

#include <QObject>
#include <QStringList>

#include <kimap/listjob.h>
#include <kimap/session.h>
#include <kimap/sessionuiproxy.h>

namespace KIMAP {
struct MailBoxDescriptor;
}

class ImapAccount;
class PasswordRequesterInterface;

class SessionPool : public QObject
{
    Q_OBJECT
    Q_ENUMS(ConnectError)

public:
    enum ErrorCodes {
        NoError,
        PasswordRequestError,
        ReconnectNeededError,
        EncryptionError,
        LoginFailError,
        CapabilitiesTestError,
        IncompatibleServerError,
        NoAvailableSessionError,
        CouldNotConnectError,
        CancelledError
    };

    enum SessionTermination {
        LogoutSession,
        CloseSession
    };

    explicit SessionPool(int maxPoolSize, QObject *parent = nullptr);
    ~SessionPool();

    PasswordRequesterInterface *passwordRequester() const;
    void setPasswordRequester(PasswordRequesterInterface *requester);
    void cancelPasswordRequests();

    KIMAP::SessionUiProxy::Ptr sessionUiProxy() const;
    void setSessionUiProxy(KIMAP::SessionUiProxy::Ptr proxy);

    void setClientId(const QByteArray &clientId);

    bool isConnected() const;
    bool connect(ImapAccount *account);
    void disconnect(SessionTermination termination = LogoutSession);

    qint64 requestSession();
    void cancelSessionRequest(qint64 id);
    void releaseSession(KIMAP::Session *session);

    ImapAccount *account() const;
    QStringList serverCapabilities() const;
    QList<KIMAP::MailBoxDescriptor> serverNamespaces() const;
    enum Namespace {
        Personal,
        User,
        Shared
    };
    QList<KIMAP::MailBoxDescriptor> serverNamespaces(Namespace) const;

Q_SIGNALS:
    void connectionLost(KIMAP::Session *session);

    void sessionRequestDone(qint64 requestNumber, KIMAP::Session *session, int errorCode = NoError, const QString &errorString = QString());
    void connectDone(int errorCode = NoError, const QString &errorString = QString());
    void disconnectDone();

private Q_SLOTS:
    void processPendingRequests();

    void onPasswordRequestDone(int resultType, const QString &password);
    void onLoginDone(KJob *job);
    void onCapabilitiesTestDone(KJob *job);
    void onNamespacesTestDone(KJob *job);
    void onIdDone(KJob *job);

    void onSessionDestroyed(QObject *);

private:
    void onConnectionLost();
    void killSession(KIMAP::Session *session, SessionTermination termination);
    void declareSessionReady(KIMAP::Session *session);
    void cancelSessionCreation(KIMAP::Session *session, int errorCode, const QString &errorString);

    static qint64 m_requestCounter;

    int m_maxPoolSize;
    ImapAccount *m_account = nullptr;
    PasswordRequesterInterface *m_passwordRequester = nullptr;
    KIMAP::SessionUiProxy::Ptr m_sessionUiProxy;

    bool m_initialConnectDone = false;
    KIMAP::Session *m_pendingInitialSession = nullptr;

    QList<qint64> m_pendingRequests;
    QList<KIMAP::Session *> m_connectingPool; // in preparation
    QList<KIMAP::Session *> m_unusedPool;    // ready to be used
    QList<KIMAP::Session *> m_reservedPool;  // currently used

    QStringList m_capabilities;
    QList<KIMAP::MailBoxDescriptor> m_namespaces;
    QList<KIMAP::MailBoxDescriptor> m_personalNamespaces;
    QList<KIMAP::MailBoxDescriptor> m_userNamespaces;
    QList<KIMAP::MailBoxDescriptor> m_sharedNamespaces;
    QByteArray m_clientId;
};

#endif
