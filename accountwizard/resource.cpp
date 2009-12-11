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

#include "resource.h"

#include <akonadi/agenttype.h>
#include <akonadi/agentmanager.h>
#include <akonadi/agentinstancecreatejob.h>

#include <KDebug>
#include <KLocale>

#include <QVariant>
#include <QtDBus/qdbusinterface.h>
#include <QtDBus/qdbusreply.h>

using namespace Akonadi;

Resource::Resource(const QString& type, QObject* parent) :
  SetupObject( parent ),
  m_typeIdentifier( type )
{
}

void Resource::setOption( const QString &key, const QVariant &value )
{
  m_settings.insert( key, value );
}

void Resource::create()
{
  const AgentType type = AgentManager::self()->type( m_typeIdentifier );
  if ( !type.isValid() ) {
    emit error( i18n( "Resource type '%1' is not available.", m_typeIdentifier ) );
    return;
  }

  // check if unique instance already exists
  kDebug() << type.capabilities();
  if ( type.capabilities().contains( QLatin1String( "Unique" ) ) ) {
    foreach ( const AgentInstance &instance, AgentManager::self()->instances() ) {
      kDebug() << instance.type().identifier() << (instance.type() == type);
      if ( instance.type() == type ) {
        emit finished( i18n( "Resource '%1' is already set up.", type.name() ) );
        return;
      }
    }
  }

  emit info( i18n( "Creating resource instance for '%1'...", type.name() ) );
  AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type, this );
  connect( job, SIGNAL(result(KJob*)), SLOT(instanceCreateResult(KJob*)) );
  job->start();
}


void Resource::instanceCreateResult(KJob* job)
{
  if ( job->error() ) {
    emit error( i18n( "Failed to create resource instance: %1", job->errorText() ) );
    return;
  }

  m_instance = qobject_cast<AgentInstanceCreateJob*>( job )->instance();

  if ( !m_settings.isEmpty() ) {
    emit info( i18n( "Configuring resource instance..." ) );
    QDBusInterface iface( "org.freedesktop.Akonadi.Resource." + m_instance.identifier(), "/Settings" );
    if ( !iface.isValid() ) {
      emit error( i18n( "Unable to configure resource instance." ) );
      return;
    }

    // configure resource
    for ( QMap<QString, QVariant>::const_iterator it = m_settings.constBegin(); it != m_settings.constEnd(); ++it ) {
      kDebug() << "Setting up " << it.key() << " for agent " << m_instance.identifier();
      const QString methodName = QString::fromLatin1("set%1").arg( it.key() );
      const QVariant arg = it.value();
      QDBusReply<void> reply = iface.call( methodName, arg );
      if ( !reply.isValid() ) {
        emit error( i18n( "Could not set setting '%1': %2", it.key(), reply.error().message() ) );
        return;
      }
    }
    m_instance.reconfigure();
  }

  emit finished( i18n( "Resource setup completed." ) );
}

void Resource::destroy()
{
  if ( m_instance.isValid() ) {
    AgentManager::self()->removeInstance( m_instance );
    emit info( i18n( "Removed resource instance for '%1'.", m_instance.type().name() ) );
  }
}

#include "resource.moc"
