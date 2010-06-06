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
#include <kimap/namespacejob.h>
#include <kimap/listjob.h>
#include <kimap/loginjob.h>
#include <kimap/logoutjob.h>

#include "settings.h"

/**
 * Maps the enum used to represent authentication in MailTransport (kdepimlibs)
 * to the one used by the imap resource.
 * @param authType the MailTransport auth enum value
 * @return the corresponding KIMAP auth value.
 * @note will cause fatal error if there is no mapping, so be careful not to pass invalid auth options (e.g., APOP) to this function.
 */
KIMAP::LoginJob::AuthenticationMode ImapAccount::mapTransportAuthToKimap( MailTransport::Transport::EnumAuthenticationType::type authType )
{
  // typedef these for readability
  typedef MailTransport::Transport::EnumAuthenticationType MTAuth;
  typedef KIMAP::LoginJob KIAuth;
  switch ( authType ) {
    case MTAuth::ANONYMOUS:
      return KIAuth::Anonymous;
    case MTAuth::PLAIN:
      return KIAuth::Plain;
    case MTAuth::NTLM:
      return KIAuth::NTLM;
    case MTAuth::LOGIN:
      return KIAuth::Login;
    case MTAuth::GSSAPI:
      return KIAuth::GSSAPI;
    case MTAuth::DIGEST_MD5:
      return KIAuth::DigestMD5;
    case MTAuth::CRAM_MD5:
      return KIAuth::CramMD5;
    case MTAuth::CLEAR:
      return KIAuth::ClearText;
    default:
      kFatal() << "mapping from Transport::EnumAuthenticationType ->  KIMAP::LoginJob::AuthenticationMode not possible";
  }
  return KIAuth::ClearText; // dummy value, shouldn't get here.
}

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
  : QObject( parent ), m_mainSession( 0 ), m_port( 0 ),
    m_encryption( KIMAP::LoginJob::Unencrypted ),
    m_authentication( KIMAP::LoginJob::ClearText ),
    m_subscriptionEnabled( false )
{
  m_server = settings->imapServer();
  if ( settings->imapPort() >= 0 ) {
    m_port = settings->imapPort();
  }
  m_userName = settings->userName();
  m_subscriptionEnabled = settings->subscriptionEnabled();

  QString safety = settings->safety();
  if( safety == "SSL" )
    m_encryption = KIMAP::LoginJob::AnySslVersion;
  else if ( safety == "STARTTLS" )
    m_encryption = KIMAP::LoginJob::TlsV1;
  else
    m_encryption = KIMAP::LoginJob::Unencrypted;
  m_authentication = mapTransportAuthToKimap( (MailTransport::TransportBase::EnumAuthenticationType::type) settings->authentication() );
}

ImapAccount::ImapAccount( QObject *parent )
  : QObject( parent ), m_mainSession( 0 ),
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

KIMAP::Session *ImapAccount::mainSession() const
{
  return m_mainSession;
}

QStringList ImapAccount::capabilities() const
{
  return m_capabilities;
}

QList<KIMAP::MailBoxDescriptor> ImapAccount::namespaces() const
{
  return m_namespaces;
}

void ImapAccount::doConnect( const QString &password )
{
  m_capabilities.clear();
  m_namespaces.clear();
  m_mainSession = createSessionInternal( password );
}

KIMAP::Session *ImapAccount::extraSession( const QString &id, const QString &password )
{
  if ( id.isEmpty() ) {
    return 0;
  }

  if ( m_extraSessions.contains( id ) ) {
    return m_extraSessions[id];
  }

  KIMAP::Session *session = createSessionInternal( password );

  if ( session!=0 ) {
    m_extraSessions[id] = session;
  }

  return session;
}

KIMAP::Session *ImapAccount::createSessionInternal( const QString &password )
{
  if ( m_server.isEmpty() ) {
    return 0;
  }

  QString server = m_server.section( ':', 0, 0 );
  int port = m_server.section( ':', 1, 1 ).toInt();
  if ( port <= 0 ) {
    port = m_port;
  }

  if ( m_encryption != KIMAP::LoginJob::Unencrypted && !QSslSocket::supportsSsl() ) {
    kWarning() << "Crypto not supported!";
    emit error( 0, EncryptionError,
                i18n( "You requested TLS/SSL to connect to %1, but your "
                      "system does not seem to be set up for that.", m_server ) );
    return 0;
  }

  if ( port <= 0 ) {
    if ( m_encryption!=KIMAP::LoginJob::Unencrypted
     && m_encryption!=KIMAP::LoginJob::TlsV1 ) {
      port = 993;
    } else {
      port = 143;
    }
  }

  KIMAP::Session *session = new KIMAP::Session( server, port, this );
  session->setUiProxy( new SessionUiProxy );

  KIMAP::LoginJob *loginJob = new KIMAP::LoginJob( session );
  loginJob->setUserName( m_userName );
  loginJob->setPassword( password );
  loginJob->setEncryptionMode( m_encryption );
  loginJob->setAuthenticationMode( m_authentication );

  QObject::connect( loginJob, SIGNAL( result( KJob* ) ),
                    this, SLOT( onLoginDone( KJob* ) ) );
  loginJob->start();

  return session;
}

/******************* Slots  ***********************************************/

void ImapAccount::onLoginDone( KJob *job )
{
  KIMAP::LoginJob *login = static_cast<KIMAP::LoginJob*>( job );

  if ( job->error()==0 ) {
    if ( login->session()!=m_mainSession ) return; // No need to test for extra sessions
    KIMAP::CapabilitiesJob *capJob = new KIMAP::CapabilitiesJob( m_mainSession );
    QObject::connect( capJob, SIGNAL( result( KJob* ) ), SLOT( onCapabilitiesTestDone( KJob* ) ) );
    capJob->start();
    return;
  } else {
    kWarning() << "Error during login:" << job->errorString();
    emit error( login->session(), LoginFailError,
                i18n( "Could not connect to the IMAP-server %1.\n%2", m_server, job->errorString() ) );

    QString id = m_extraSessions.key( login->session() );
    if ( !id.isEmpty() ) {
      m_extraSessions.remove( id );
    }

    if ( login->session() == m_mainSession ) {
      m_mainSession = 0;
    }

    KIMAP::LogoutJob *logout = new KIMAP::LogoutJob( login->session() );
    QObject::connect( logout, SIGNAL( result( KJob* ) ),
                      login->session(), SLOT( deleteLater() ) );
    logout->start();

  }
}

void ImapAccount::onCapabilitiesTestDone( KJob *job )
{
  KIMAP::CapabilitiesJob *capJob = qobject_cast<KIMAP::CapabilitiesJob*>( job );

  if ( job->error() ) {
    emit error( capJob->session(),
                CapabilitiesTestError,
                i18n( "Could not test the capabilities supported by the IMAP server %1.", m_server ) );
    return;
  }

  m_capabilities = capJob->capabilities();
  QStringList expected;
  expected << "IMAP4REV1";

  // Both GMail and GMX servers seem to be lying about their capabilities
  // They don't report UIDPLUS correctly so don't check for it explicitly
  // if it's one of those servers.
  if ( !m_capabilities.contains( "X-GM-EXT-1" )
    && !capJob->session()->serverGreeting().contains( "GMX" ) ) {
    expected << "UIDPLUS";
  }

  QStringList missing;

  foreach ( const QString &capability, expected ) {
    if ( !m_capabilities.contains( capability ) ) {
      missing << capability;
    }
  }

  if ( !missing.isEmpty() ) {
    emit error( capJob->session(),
                IncompatibleServerError,
                i18n( "Cannot use the IMAP server %1, some mandatory capabilities are missing: %2. "
                      "Please ask your sysadmin to upgrade the server.", m_server, missing.join( ", " ) ) );
    return;
  }

  // If the extension is supported, grab the namespaces from the server
  if ( m_capabilities.contains( "NAMESPACE" ) ) {
    KIMAP::NamespaceJob *nsJob = new KIMAP::NamespaceJob( m_mainSession );
    QObject::connect( nsJob, SIGNAL( result( KJob* ) ), SLOT( onNamespacesTestDone( KJob* ) ) );
    nsJob->start();
    return;
  } else {
    emit success( capJob->session() );
  }
}

void ImapAccount::onNamespacesTestDone( KJob *job )
{
  KIMAP::NamespaceJob *nsJob = qobject_cast<KIMAP::NamespaceJob*>( job );

  if ( nsJob->containsEmptyNamespace() ) {
    // When we got the empty namespace here, we assume that the other
    // ones can be freely ignored and that the server will give us all
    // the mailboxes if we list from the empty namespace itself...

    m_namespaces.clear();

  } else {
    // ... otherwise we assume that we have to list explicitly each
    // namespace

    m_namespaces = nsJob->personalNamespaces()
                 + nsJob->userNamespaces()
                 + nsJob->sharedNamespaces();
  }

  emit success( nsJob->session() );
}

/******************* Private ***********************************************/

bool ImapAccount::connect( const QString &password )
{
  if ( m_mainSession==0 ) {
    doConnect( password );
    return true;
  } else {
    return false;
  }
}

void ImapAccount::disconnect()
{
  QList<KIMAP::Session*> sessions;
  sessions << m_mainSession;
  sessions+= m_extraSessions.values();

  foreach ( KIMAP::Session *session, sessions ) {
    KIMAP::LogoutJob *logout = new KIMAP::LogoutJob( session );
    QObject::connect( logout, SIGNAL( result( KJob* ) ), session, SLOT( deleteLater() ) );
    logout->start();
  }

  m_mainSession = 0;
  m_extraSessions.clear();
}

#include "imapaccount.moc"


