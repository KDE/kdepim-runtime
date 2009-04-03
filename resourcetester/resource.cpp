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

#include "global.h"
#include "resourcesynchronizationjob.h"
#include "test.h"

#include <akonadi/agentmanager.h>
#include <akonadi/agentinstancecreatejob.h>

#include <KDebug>
#include <qtest_kde.h>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

using namespace Akonadi;

Resource* Resource::mSelf = 0;

Resource::Resource(QObject* parent) :
  QObject( parent )
{
  Q_ASSERT( !mSelf );
  mSelf = this;
}

Resource::~Resource()
{
  destroy();
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

void Resource::setPathOption(const QString& key, const QString& path)
{
  if ( QFileInfo( path ).isAbsolute() )
    setOption( key, path );
  else
    setOption( key, Global::basePath() + QDir::separator() + path );
}


bool Resource::createResource()
{
  if ( mInstance.isValid() )
    return false;

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

  ResourceSynchronizationJob *syncJob = new ResourceSynchronizationJob( mInstance, this );
  if ( !syncJob->exec() )
    kError() << "Synching resource failed: " << syncJob->errorString();

  return true;
}

void Resource::create()
{
  if ( !createResource() )
    Test::instance()->fail( "Creating resource failed." );
}


void Resource::destroy()
{
  if ( !mInstance.isValid() )
    return;
  AgentManager::self()->removeInstance( mInstance );
  mInstance = AgentInstance();
}

void Resource::write()
{
  QDBusInterface iface( "org.freedesktop.Akonadi", "/notifications/debug", "org.freedesktop.Akonadi.NotificationManager" );
  Q_ASSERT( iface.isValid() );
  QDBusReply<void> result = iface.call( "emitPendingNotifications" );
  if ( !result.isValid() )
    Test::instance()->fail( result.error().message() );
  ResourceSynchronizationJob *syncJob = new ResourceSynchronizationJob( mInstance, this );
  if ( !syncJob->exec() )
    kError() << "Synching resource failed: " << syncJob->errorString();
}

void Resource::recreate()
{
  write();
  destroy();
  create();
}

Resource* Resource::instance()
{
  return mSelf;
}

#include "resource.moc"
