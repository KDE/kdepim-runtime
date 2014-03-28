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
#include <KLocalizedString>

#include <QMetaMethod>
#include <QVariant>
#include <QtDBus/qdbusinterface.h>
#include <QtDBus/qdbusreply.h>

using namespace Akonadi;

static QVariant::Type argumentType( const QMetaObject *mo, const QString &method )
{
  QMetaMethod m;
  for ( int i = 0; i < mo->methodCount(); ++i ) {
    const QString signature = QString::fromLatin1( mo->method( i ).signature() );
    if ( signature.contains( method + QLatin1Char( '(' ) ) ) {
      m = mo->method( i );
      break;
    }
  }

  if ( !m.signature() ) {
    kWarning() << "Did not find D-Bus method: " << method << " available methods are:";
    for ( int i = 0; i < mo->methodCount(); ++i )
      kWarning() << mo->method( i ).signature();
    return QVariant::Invalid;
  }

  const QList<QByteArray> argTypes = m.parameterTypes();
  if ( argTypes.count() != 1 )
    return QVariant::Invalid;

  return QVariant::nameToType( argTypes.first() );
}

Resource::Resource(const QString& type, QObject* parent) :
  SetupObject( parent ),
  m_typeIdentifier( type )
{
}

void Resource::setOption( const QString &key, const QVariant &value )
{
  m_settings.insert( key, value );
}

void Resource::setName( const QString &name )
{
  m_name = name;
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
      kDebug() << instance.type().identifier() << ( instance.type() == type );
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
    QDBusInterface iface( QLatin1String("org.freedesktop.Akonadi.Resource.") + m_instance.identifier(), QLatin1String("/Settings") );
    if ( !iface.isValid() ) {
      emit error( i18n( "Unable to configure resource instance." ) );
      return;
    }

    // configure resource
    if ( !m_name.isEmpty() )
      m_instance.setName( m_name );
    QMap<QString, QVariant>::const_iterator end( m_settings.constEnd() );
    for ( QMap<QString, QVariant>::const_iterator it = m_settings.constBegin(); it != end; ++it ) {
      kDebug() << "Setting up " << it.key() << " for agent " << m_instance.identifier();
      const QString methodName = QString::fromLatin1( "set%1" ).arg( it.key() );
      QVariant arg = it.value();
      const QVariant::Type targetType = argumentType( iface.metaObject(), methodName );
      if ( !arg.canConvert( targetType ) ) {
        emit error( i18n( "Could not convert value of setting '%1' to required type %2.", it.key(), QLatin1String(QVariant::typeToName( targetType )) ) );
        return;
      }
      arg.convert( targetType );
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

QString Resource::identifier()
{
    return m_instance.identifier();
}

void Resource::reconfigure()
{
    m_instance.reconfigure();
}


