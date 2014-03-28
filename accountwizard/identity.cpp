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
#include "transport.h"

#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identity.h>

#include <KLocalizedString>

Identity::Identity( QObject *parent )
  : SetupObject( parent ),
  m_transport( 0 )
{
  m_manager = new KPIMIdentities::IdentityManager( false, this, "mIdentityManager" );
  m_identity = &m_manager->newFromScratch( QString() );
  Q_ASSERT( m_identity != 0 );
}

Identity::~Identity()
{
  delete m_manager;
}

void Identity::create()
{
  emit info( i18n( "Setting up identity..." ) );

  // store identity information
  // TODO now that we have the identity object around anyway we can probably get rid of most of the other members
  m_identity->setIdentityName( identityName() );
  m_identity->setFullName( m_realName );
  m_identity->setPrimaryEmailAddress( m_email );
  m_identity->setOrganization( m_organization );
  if ( m_transport && m_transport->transportId() > 0 )
    m_identity->setTransport( QString::number( m_transport->transportId() ) );
  if ( !m_signature.isEmpty() ) {
    const KPIMIdentities::Signature sig( m_signature );
    m_identity->setSignature( sig );
  }
  if ( !m_prefCryptoFormat.isEmpty() )
    m_identity->setPreferredCryptoMessageFormat( m_prefCryptoFormat );
  if ( !m_xface.isEmpty() ) {
    m_identity->setXFaceEnabled( true );
    m_identity->setXFace( m_xface );
  }
  m_manager->setAsDefault( m_identity->uoid() );
  m_manager->commit();

  emit finished( i18n( "Identity set up." ) );
}

QString Identity::identityName() const
{
  // create identity name
  QString name( m_identityName );
  if ( name.isEmpty() ) {
    name = i18nc( "Default name for new email accounts/identities.", "Unnamed" );

    QString idName = m_email;
    int pos = idName.indexOf( QLatin1Char('@') );
    if ( pos != -1 ) {
      name = idName.mid( 0, pos );
    }

    // Make the name a bit more human friendly
    name.replace( QLatin1Char('.'), QLatin1Char(' ') );
    pos = name.indexOf( QLatin1Char(' ') );
    if ( pos != 0 ) {
      name[ pos + 1 ] = name[ pos + 1 ].toUpper();
    }
    name[ 0 ] = name[ 0 ].toUpper();
  }

  if ( !m_manager->isUnique( name ) ) {
    name = m_manager->makeUnique( name );
  }
  return name;
}


void Identity::destroy()
{
  m_manager->removeIdentityForced( m_identity->identityName() );
  m_manager->commit();
  m_identity = 0;
  emit info( i18n( "Identity removed." ) );
}

void Identity::setIdentityName(const QString& name)
{
  m_identityName = name;
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

uint Identity::uoid() const
{
  return m_identity->uoid();
}

void Identity::setTransport(QObject* transport)
{
  m_transport = qobject_cast<Transport*>( transport );
  setDependsOn( qobject_cast<SetupObject*>( transport ) );
}

void Identity::setSignature(const QString& sig)
{
  m_signature = sig;
}

void Identity::setPreferredCryptoMessageFormat(const QString& format)
{
  m_prefCryptoFormat = format;
}

void Identity::setXFace(const QString& xface)
{
  m_xface = xface;
}

