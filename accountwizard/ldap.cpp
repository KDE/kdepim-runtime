/*
    Copyright (c) 2010 Laurent Montel <montel@kde.org>

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

#include "ldap.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocale>


Ldap::Ldap( QObject *parent )
  : SetupObject( parent )
{
}

Ldap::~Ldap()
{
}

void Ldap::create()
{
  emit info( i18n( "Setting up LDAP server..." ) );

  if ( m_server.isEmpty() || m_user.isEmpty() )
    return;

  const QString host = m_server;

  // Figure out the basedn
  QString basedn = host;
  // If the user gave a full email address, the domain name
  // of that overrides the server name for the ldap dn
  const QString user = m_user;
  int pos = user.indexOf( "@" );
  if ( pos > 0 ) {
    const QString h = user.mid( pos+1 );
    if ( !h.isEmpty() )
      // The user did type in a domain on the email address. Use that
      basedn = h;
  }
  { // while we're here, write default domain
    KConfig c( "kmail2rc" );
    KConfigGroup group = c.group( "General" );
    group.writeEntry( "Default domain", basedn );
  }

  basedn.replace( '.', ",dc=" );
  basedn.prepend( "dc=" );

  // Set the changes
  KConfig c( "kabldaprc" );
  KConfigGroup group = c.group( "LDAP" );
  bool hasMyServer = false;
  uint selHosts = group.readEntry( "NumSelectedHosts", 0 );
  for ( uint i = 0 ; i < selHosts && !hasMyServer; ++i )
    if ( group.readEntry( QString( "SelectedHost%1" ).arg( i ), QString() ) == host )
      hasMyServer = true;
  if ( !hasMyServer ) {
    group.writeEntry( "NumSelectedHosts", selHosts + 1 );
    group.writeEntry( QString( "SelectedHost%1" ).arg( selHosts ), host );
    group.writeEntry( QString( "SelectedBase%1" ).arg( selHosts ), basedn );
    group.writeEntry( QString( "SelectedPort%1" ).arg( selHosts ), "389" );
    if ( !m_authMethod.isEmpty() ) {
      group.writeEntry( QString( "SelectedAuth%1" ).arg( selHosts ), m_authMethod );
      group.writeEntry( QString( "SelectedBind%1" ).arg( selHosts ), m_bindDn );
      group.writeEntry( QString( "SelectedPwdBind%1" ).arg( selHosts ), m_password );
    }
  }
  emit finished( i18n( "LDAP set up." ) );
}

void Ldap::destroy()
{
  emit info( i18n( "LDAP not configuring." ) );
}

void Ldap::setUser( const QString &user )
{
  m_user = user;
}

void Ldap::setServer( const QString &server )
{
  m_server = server;
}

void Ldap::setAuthenticationMethod(const QString& meth)
{
  m_authMethod = meth;
}

void Ldap::setBindDn(const QString& bindDn)
{
  m_bindDn = bindDn;
}

void Ldap::setPassword(const QString& password)
{
  m_password = password;
}

#include "ldap.moc"
