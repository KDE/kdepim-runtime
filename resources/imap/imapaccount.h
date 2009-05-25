/*
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>
    Copyright (C) 2009 Kevin Ottens <ervin@kde.org>

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

#ifndef __IMAP_ACCOUNT_H__
#define __IMAP_ACCOUNT_H__

class KJob;

#include <akonadi/resourcebase.h>
#include <boost/shared_ptr.hpp>
#include <QtCore/QStringList>

#include <kaccount.h>
#include <kimap/loginjob.h>

namespace KMime
{
  class Message;
}

namespace KIMAP
{
  class Session;
}

class ImapResource;
class Settings;

class ImapAccount : public QObject, public KPIM::KAccount
{
  Q_OBJECT
  Q_ENUMS( ConnectError )

public:
  enum ConnectError {
    EncryptionError,
    LoginFailError,
    CapabilitiesTestError,
    IncompatibleServerError
  };

  ImapAccount( Settings *settings, QObject *parent = 0 );
  ImapAccount( QObject *parent = 0 );
  ~ImapAccount();

  bool connect( const QString &password = QString() );
  bool disconnect();

  void setServer( const QString &server );
  QString server() const;

  void setUserName( const QString &userName );
  QString userName() const;

  void setEncryptionMode( KIMAP::LoginJob::EncryptionMode mode );
  KIMAP::LoginJob::EncryptionMode encryptionMode() const;

  void setAuthenticationMode( KIMAP::LoginJob::AuthenticationMode mode );
  KIMAP::LoginJob::AuthenticationMode authenticationMode() const;

  void setSubscriptionEnabled( bool enabled );
  bool isSubscriptionEnabled() const;

  KIMAP::Session *session() const;
  QStringList capabilities() const;

Q_SIGNALS:
  void success();
  void error( int code, const QString &message );

private Q_SLOTS:
  void onLoginDone( KJob *job );
  void onCapabilitiesTestDone( KJob *job );

private:
  void doConnect( const QString &password );

  KIMAP::Session *m_session;
  QStringList m_capabilities;
  QString m_server;
  QString m_userName;
  KIMAP::LoginJob::EncryptionMode m_encryption;
  KIMAP::LoginJob::AuthenticationMode m_authentication;
  bool m_subscriptionEnabled;
};

#endif
