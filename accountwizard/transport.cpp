/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "transport.h"

#include <mailtransport/transportmanager.h>

#include <KDebug>
#include <KLocalizedString>

#define TABLE_SIZE x

template <typename T>
struct StringValueTable {
  const char * name;
  typename T::type value;
  typedef typename T::type value_type;
};

static const StringValueTable<MailTransport::Transport::EnumType> transportTypeEnums[] = {
  { "smtp", MailTransport::Transport::EnumType::SMTP },
  { "sendmail", MailTransport::Transport::EnumType::Sendmail },
  { "akonadi", MailTransport::Transport::EnumType::Akonadi }
};
static const int transportTypeEnumsSize = sizeof( transportTypeEnums ) / sizeof ( *transportTypeEnums );

static const StringValueTable<MailTransport::Transport::EnumEncryption> encryptionEnum[] = {
  { "none", MailTransport::Transport::EnumEncryption::None },
  { "ssl", MailTransport::Transport::EnumEncryption::SSL },
  { "tls", MailTransport::Transport::EnumEncryption::TLS }
};
static const int encryptionEnumSize = sizeof( encryptionEnum ) / sizeof( *encryptionEnum );

static const StringValueTable<MailTransport::Transport::EnumAuthenticationType> authenticationTypeEnum[] = {
  { "login",      MailTransport::Transport::EnumAuthenticationType::LOGIN },
  { "plain",      MailTransport::Transport::EnumAuthenticationType::PLAIN },
  { "cram-md5",   MailTransport::Transport::EnumAuthenticationType::CRAM_MD5 },
  { "digest-md5", MailTransport::Transport::EnumAuthenticationType::DIGEST_MD5 },
  { "gssapi",     MailTransport::Transport::EnumAuthenticationType::GSSAPI },
  { "ntlm",       MailTransport::Transport::EnumAuthenticationType::NTLM },
  { "apop",       MailTransport::Transport::EnumAuthenticationType::APOP },
  { "clear",      MailTransport::Transport::EnumAuthenticationType::CLEAR },
  { "anonymous",  MailTransport::Transport::EnumAuthenticationType::ANONYMOUS }
};
static const int authenticationTypeEnumSize = sizeof( authenticationTypeEnum ) / sizeof( *authenticationTypeEnum );

template <typename T>
static typename T::value_type stringToValue( const T *table, const int tableSize, const QString &string )
{
  const QString ref = string.toLower();
  for ( int i = 0; i < tableSize; ++i ) {
    if ( ref == QLatin1String( table[i].name ) )
      return table[i].value;
  }
  return table[0].value; // TODO: error handling
}

Transport::Transport(const QString& type, QObject* parent) :
  SetupObject( parent ),
  m_transportId( -1 ),
  m_port( -1 ),
  m_encr( MailTransport::Transport::EnumEncryption::TLS ),
  m_auth( MailTransport::Transport::EnumAuthenticationType::PLAIN )
{
  m_transportType = stringToValue( transportTypeEnums, transportTypeEnumsSize, type );
  if ( m_transportType == MailTransport::Transport::EnumType::SMTP )
    m_port = 25;
}

void Transport::create()
{
  emit info( i18n( "Setting up mail transport account..." ) );
  MailTransport::Transport* mt = MailTransport::TransportManager::self()->createTransport();
  mt->setName( m_name );
  mt->setHost( m_host );
  if ( m_port > 0 )
    mt->setPort( m_port );
  if ( !m_user.isEmpty() ) {
    mt->setUserName( m_user );
    mt->setRequiresAuthentication( true );
  }
  if ( !m_password.isEmpty() ) {
    mt->setStorePassword( true );
    mt->setPassword( m_password );
  }
  mt->setEncryption( m_encr );
  mt->setAuthenticationType( m_auth );
  m_transportId = mt->id();
  mt->writeConfig();
  MailTransport::TransportManager::self()->addTransport( mt );
  MailTransport::TransportManager::self()->setDefaultTransport( mt->id() );
  emit finished( i18n( "Mail transport account set up." ) );
}

void Transport::destroy()
{
  MailTransport::TransportManager::self()->removeTransport( m_transportId );
  emit info( i18n( "Mail transport account deleted." ) );
}

void Transport::setName( const QString &name )
{
  m_name = name;
}

void Transport::setHost( const QString &host )
{
  m_host = host;
}

void Transport::setPort( int port )
{
  m_port = port;
}

void Transport::setUsername( const QString &user )
{
  m_user = user;
}

void Transport::setPassword( const QString &password )
{
  m_password = password;
}

void Transport::setEncryption( const QString &encryption )
{
  m_encr = stringToValue( encryptionEnum, encryptionEnumSize, encryption );
}

void Transport::setAuthenticationType( const QString &authType )
{
  m_auth = stringToValue( authenticationTypeEnum, authenticationTypeEnumSize, authType );
}

int Transport::transportId() const
{
  return m_transportId;
}

