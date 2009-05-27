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

#include "imapaccount.h"

#include <QtNetwork/QSslSocket>

#include <kdebug.h>
#include <klocale.h>
#include <kpassworddialog.h>
#include <kmessagebox.h>
#include <KWindowSystem>
#include <KAboutData>

#include <kio/sslui.h>

#include <kimap/session.h>
#include <kimap/sessionuiproxy.h>

#include <kimap/capabilitiesjob.h>
#include <kimap/loginjob.h>
#include <kimap/logoutjob.h>

#include "settings.h"

class SessionUiProxy : public KIMAP::SessionUiProxy {
  public:
    bool ignoreSslError(const KSslErrorUiData& errorData) {
      if (KIO::SslUi::askIgnoreSslErrors(errorData, KIO::SslUi::RecallAndStoreRules)) {
        return true;
      } else {
        return false;
      }
    }
};

ImapAccount::ImapAccount( Settings *settings, QObject *parent )
  : QObject( parent ), m_session( 0 ),
    m_encryption( KIMAP::LoginJob::Unencrypted ),
    m_authentication( KIMAP::LoginJob::ClearText ),
    m_subscriptionEnabled( false )
{
  m_server = settings->imapServer();
  m_userName = settings->userName();
  m_subscriptionEnabled = settings->subscriptionEnabled();

  switch ( settings->safety() ) {
  case 1:
    m_encryption = KIMAP::LoginJob::Unencrypted;
    break;
  case 2:
    m_encryption = KIMAP::LoginJob::AnySslVersion;
    break;
  case 3:
    m_encryption = KIMAP::LoginJob::TlsV1;
    break;
  default:
    kFatal("Shouldn't happen...");
  }

  switch ( settings->authentication() ) {
  case 1:
    m_authentication = KIMAP::LoginJob::ClearText;
    break;
  case 2:
    m_authentication = KIMAP::LoginJob::Login;
    break;
  case 3:
    m_authentication = KIMAP::LoginJob::Plain;
    break;
  case 4:
    m_authentication = KIMAP::LoginJob::CramMD5;
    break;
  case 5:
    m_authentication = KIMAP::LoginJob::DigestMD5;
    break;
  case 6:
    m_authentication = KIMAP::LoginJob::NTLM;
    break;
  case 7:
    m_authentication = KIMAP::LoginJob::GSSAPI;
    break;
  case 8:
    m_authentication = KIMAP::LoginJob::Anonymous;
    break;
  default:
    kFatal("Shouldn't happen...");
  }
}

ImapAccount::ImapAccount( QObject *parent )
  : QObject( parent ), m_session( 0 ),
    m_encryption( KIMAP::LoginJob::Unencrypted ),
    m_authentication( KIMAP::LoginJob::ClearText ),
    m_subscriptionEnabled( false )
{
}

ImapAccount::~ImapAccount()
{
}

void ImapAccount::setServer( const QString &server )
{
  m_server = server;
}

QString ImapAccount::server() const
{
  return m_server;
}

void ImapAccount::setUserName( const QString &userName )
{
  m_userName = userName;
}

QString ImapAccount::userName() const
{
  return m_userName;
}

void ImapAccount::setEncryptionMode( KIMAP::LoginJob::EncryptionMode mode)
{
  m_encryption = mode;
}

KIMAP::LoginJob::EncryptionMode ImapAccount::encryptionMode() const
{
  return m_encryption;
}

void ImapAccount::setAuthenticationMode( KIMAP::LoginJob::AuthenticationMode mode)
{
  m_authentication = mode;
}

KIMAP::LoginJob::AuthenticationMode ImapAccount::authenticationMode() const
{
  return m_authentication;
}

void ImapAccount::setSubscriptionEnabled( bool enabled )
{
  m_subscriptionEnabled = enabled;
}

bool ImapAccount::isSubscriptionEnabled() const
{
  return m_subscriptionEnabled;
}

KIMAP::Session *ImapAccount::session() const
{
  return m_session;
}

QStringList ImapAccount::capabilities() const
{
  return m_capabilities;
}

void ImapAccount::doConnect( const QString &password )
{
  m_capabilities.clear();

  if ( m_server.isEmpty() ) {
    return;
  }

  QString server = m_server.section( ":", 0, 0 );
  int port = m_server.section( ":", 1, 1 ).toInt();

  if ( m_encryption != KIMAP::LoginJob::Unencrypted && !QSslSocket::supportsSsl() ) {
    kWarning() << "Crypto not supported!";
    emit error( EncryptionError,
                i18n( "You requested TLS/SSL to connect to %1, but your "
                      "system does not seem to be set up for that.", m_server ) );
    return;
  }

  if ( port <= 0 ) {
    if ( m_encryption!=KIMAP::LoginJob::Unencrypted
     && m_encryption!=KIMAP::LoginJob::TlsV1 ) {
      port = 993;
    } else {
      port = 143;
    }
  }

  m_session = new KIMAP::Session( server, port, this );
  m_session->setUiProxy( new SessionUiProxy );

  KIMAP::LoginJob *loginJob = new KIMAP::LoginJob( m_session );
  loginJob->setUserName( m_userName );
  loginJob->setPassword( password );
  loginJob->setEncryptionMode( m_encryption );
  loginJob->setAuthenticationMode( m_authentication );

  QObject::connect( loginJob, SIGNAL( result( KJob* ) ),
                    this, SLOT( onLoginDone( KJob* ) ) );
  loginJob->start();
}

/******************* Slots  ***********************************************/

void ImapAccount::onLoginDone( KJob *job )
{
  if ( job->error()==0 ) {
    KIMAP::CapabilitiesJob *capJob = new KIMAP::CapabilitiesJob( m_session );
    QObject::connect( capJob, SIGNAL( result( KJob* ) ), SLOT( onCapabilitiesTestDone( KJob* ) ) );
    capJob->start();
    return;
  } else {
    emit error( LoginFailError,
                i18n( "Could not connect to the IMAP-server %1.", m_server ) );
    disconnect();
  }
}

void ImapAccount::onCapabilitiesTestDone( KJob *job )
{
  if ( job->error() ) {
    emit error( CapabilitiesTestError,
                i18n( "Could not test the capabilities supported by the IMAP server %1.", m_server ) );
    disconnect();
    return;
  }

  KIMAP::CapabilitiesJob *capJob = qobject_cast<KIMAP::CapabilitiesJob*>( job );
  m_capabilities = capJob->capabilities();
  QStringList expected;
  expected << "IMAP4rev1";

  if ( !m_capabilities.contains( "X-GM-EXT-1" ) ) {
    expected << "UIDPLUS";
  }

  QStringList missing;

  foreach ( const QString &capability, expected ) {
    if ( !m_capabilities.contains( capability ) ) {
      missing << capability;
    }
  }

  if ( !missing.isEmpty() ) {
    emit error( IncompatibleServerError,
                i18n( "Cannot use the IMAP server %1, some mandatory capabilities are missing: %2. "
                      "Please ask your sysadmin to upgrade the server.", m_server, missing.join( ", " ) ) );
    disconnect();
    return;
  }

  emit success();
}

/******************* Private ***********************************************/

bool ImapAccount::connect( const QString &password )
{
  if ( m_session==0 ) {
    doConnect( password );
    return true;
  } else {
    return false;
  }
}

bool ImapAccount::disconnect()
{
  if ( m_session==0 ) {
    return false;
  } else {
    KIMAP::LogoutJob *logout = new KIMAP::LogoutJob( m_session );
    QObject::connect( logout, SIGNAL( result( KJob* ) ), m_session, SLOT( deleteLater() ) );
    logout->start();
    m_session = 0;
    return true;
  }
}

#include "imapaccount.moc"


