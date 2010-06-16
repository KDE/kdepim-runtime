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

#include <QtCore/QObject>
#include <QtCore/QStringList>

#include <kimap/listjob.h>

namespace KIMAP
{
  class MailBoxDescriptor;
  class Session;
  class SessionUiProxy;
}

class ImapAccount;
class PasswordRequesterInterface;

class SessionPool : public QObject
{
  Q_OBJECT
  Q_ENUMS( ConnectError )

public:
  enum ErrorCodes {
    NoError,
    PasswordRequestError,
    EncryptionError,
    LoginFailError,
    CapabilitiesTestError,
    IncompatibleServerError,
    NoAvailableSessionError,
  };

  explicit SessionPool( int maxPoolSize, QObject *parent = 0 );
  ~SessionPool();

  PasswordRequesterInterface *passwordRequester() const;
  void setPasswordRequester( PasswordRequesterInterface *requester );

  KIMAP::SessionUiProxy *sessionUiProxy() const;
  void setSessionUiProxy( KIMAP::SessionUiProxy *proxy );

  bool connect( ImapAccount *account );
  void disconnect();

  qint64 requestSession();
  void releaseSession( KIMAP::Session *session );

  ImapAccount *account() const;
  QStringList serverCapabilities() const;
  QList<KIMAP::MailBoxDescriptor> serverNamespaces() const;

signals:
  void sessionRequestDone( qint64 requestNumber, KIMAP::Session *session,
                           int errorCode = NoError, const QString &errorString = QString() );
  void connectDone( int errorCode = NoError, const QString &errorString = QString() );
  void disconnectDone();

private slots:
  void processPendingRequests();

  void onPasswordRequestDone(int resultType, const QString &password);
  void onLoginDone( KJob *job );
  void onCapabilitiesTestDone( KJob *job );
  void onNamespacesTestDone( KJob *job );

private:
  void killSession( KIMAP::Session *session );
  void declareSessionReady( KIMAP::Session *session );
  void cancelSessionCreation( KIMAP::Session *session, int errorCode, const QString &errorString );

  static qint64 m_requestCounter;

  int m_maxPoolSize;
  ImapAccount *m_account;
  PasswordRequesterInterface *m_passwordRequester;
  KIMAP::SessionUiProxy *m_sessionUiProxy;
  bool m_initialConnectDone;

  QList<qint64> m_pendingRequests;
  QList<KIMAP::Session*> m_idlePool;
  QList<KIMAP::Session*> m_reservedPool;

  QStringList m_capabilities;
  QList<KIMAP::MailBoxDescriptor> m_namespaces;
};

#endif
