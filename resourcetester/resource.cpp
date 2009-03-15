/*
 * Copyright (c) 2009  Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 * Copyright (c) 2009 Volker Krause <vkrause@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "resource.h"

#include <akonadi/agentmanager.h>
#include <akonadi/agentinstancecreatejob.h>

#include <KDebug>
#include <qtest_kde.h>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

using namespace Akonadi;

Resource::Resource(QObject* parent) :
  QObject( parent )
{
}

void Resource::setType(const QString& type)
{
  mTypeIdentifier = type;
}

QString Resource::identifier() const
{
  return mInstance.identifier();
}

void Resource::setOption(const QString& key, const QVariant& value)
{
  mSettings.insert( key, value );
}

bool Resource::create()
{
  const AgentType type = AgentManager::self()->type( mTypeIdentifier );
  if ( !type.isValid() )
    return false;

  AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type, this );
  if ( !job->exec() ) {
    kWarning() << job->errorText();
    return false;
  }
  mInstance = job->instance();

  QDBusInterface iface( "org.freedesktop.Akonadi.Resource." + identifier(), "/Settings" );
  if ( !iface.isValid() )
    return false;

  // configure resource
  for ( QHash<QString, QVariant>::const_iterator it = mSettings.constBegin(); it != mSettings.constEnd(); ++it ) {
    kDebug() << "Setting up " << it.key() << " for agent " << identifier();
    const QString methodName = QString::fromLatin1("set%1").arg( it.key() );
    const QVariant arg = it.value();
    QDBusReply<void> reply = iface.call( methodName, arg );
    if ( !reply.isValid() )
      kError() << "Setting " << it.key() << " failed for agent " << identifier() << ":" << reply.error().message();
  }
  mInstance.reconfigure();

  // synchronize resource
  QDBusInterface *resIface = new QDBusInterface( QString::fromLatin1( "org.freedesktop.Akonadi.Resource.%1").arg( identifier() ),
                                                 "/", "org.freedesktop.Akonadi.Resource", QDBusConnection::sessionBus(), this );
  if ( resIface->isValid() ) {
    mInstance.synchronize();
    kDebug() << "waiting for resource to synchronize...";
    // TODO: this crashs as soon as the signal is emitted, deep in qdbus stuff
//     QTest::kWaitForSignal( resIface, SIGNAL(synchronized()) );
    kDebug() << "done";
  }
  delete resIface;

  return true;
}

void Resource::destroy()
{
  AgentManager::self()->removeInstance( mInstance );
  mInstance = AgentInstance();
}

#include "resource.moc"
