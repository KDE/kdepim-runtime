/*
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

#include "resourcesynchronizationjob.h"

#include <akonadi/agentinstance.h>

#include <KDebug>
#include <KLocale>

#include <QDBusConnection>
#include <QDBusInterface>

namespace Akonadi
{

class ResourceSynchronizationJobPrivate
{
  public:
    AgentInstance instance;
    QDBusInterface* interface;
};

ResourceSynchronizationJob::ResourceSynchronizationJob(const AgentInstance& instance, QObject* parent) :
  KJob( parent ),
  d( new ResourceSynchronizationJobPrivate )
{
  d->instance = instance;
}

ResourceSynchronizationJob::~ResourceSynchronizationJob()
{
  delete d;
}

void ResourceSynchronizationJob::start()
{
  if ( !d->instance.isValid() ) {
    setError( UserDefinedError );
    setErrorText( "Invalid resource instance." );
    emitResult();
    return;
  }

  d->interface = new QDBusInterface( QString::fromLatin1( "org.freedesktop.Akonadi.Resource.%1").arg( d->instance.identifier() ),
                                      "/", "org.freedesktop.Akonadi.Resource", QDBusConnection::sessionBus(), this );
  connect( d->interface, SIGNAL(synchronized()), SLOT(slotSynchronized()) );

  if ( d->interface->isValid() ) {
    d->instance.synchronize();
  } else {
    setError( UserDefinedError );
    setErrorText( i18n( "Unable to obtain D-Bus interface for resource '%1'", d->instance.identifier() ) );
    emitResult();
    return;
  }
}

void ResourceSynchronizationJob::slotSynchronized()
{
  emitResult();
}

}

#include "resourcesynchronizationjob.moc"
