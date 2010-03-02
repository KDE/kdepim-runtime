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

#include "identity.h"

#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identity.h>

#include <KLocale>


Identity::Identity( QObject *parent )
  : SetupObject( parent )
{
  m_manager = new KPIMIdentities::IdentityManager( false, this, "mIdentityManager" );
}

Identity::~Identity()
{
}

void Identity::create()
{
  emit info( i18n( "Setting up identity..." ) );

  // store identity information
  KPIMIdentities::Identity *identity = 0;
  identity = &m_manager->newFromScratch( identityName() );
  Q_ASSERT( identity != 0 );
  identity->setFullName( m_realName );
  identity->setEmailAddr( m_email );
  identity->setOrganization( m_organization );
  m_manager->commit();

  emit finished( i18n( "Identity set up." ) );
}

QString Identity::identityName() const
{
  // create identity name
  QString name( i18nc( "Default name for new email accounts/identities.", "Unnamed" ) );

  QString idName = m_email;
  int pos = idName.indexOf( '@' );
  if ( pos != -1 ) {
    name = idName.mid( 0, pos );
  }

  // Make the name a bit more human friendly
  name.replace( '.', ' ' );
  pos = name.indexOf( ' ' );
  if ( pos != 0 ) {
    name[ pos + 1 ] = name[ pos + 1 ].toUpper();
  }
  name[ 0 ] = name[ 0 ].toUpper();

  if ( !m_manager->isUnique( name ) ) {
    name = m_manager->makeUnique( name );
  }
  return name;
}


void Identity::destroy()
{
  emit info( i18n( "Identity not configuring." ) );
}

void Identity::setRealName( const QString &name )
{
  m_realName = name;
}

void Identity::setOrganization( const QString &org )
{
  m_organization = org;
}

void Identity::setEmail( const QString &email )
{
  m_email = email;
}

#include "identity.moc"
