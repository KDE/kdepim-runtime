/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "searchproviderbase.h"
#include "searchproviderthread.h"
#include "searchprovideradaptor.h"

#include <QStringList>

class Akonadi::SearchProviderBasePrivate
{
  public:
    int ticketCounter;
};

Akonadi::SearchProviderBase::SearchProviderBase() :
    d( new SearchProviderBasePrivate )
{
  d->ticketCounter = 0;

  new SearchProviderAdaptor( this );
  Q_ASSERT( QDBusConnection::sessionBus().registerObject( "/", this, QDBusConnection::ExportAdaptors ) );
}

Akonadi::SearchProviderBase::~ SearchProviderBase()
{
  delete d;
}

int Akonadi::SearchProviderBase::search(const QString & targetCollection, const QString & searchQuery)
{
  // TODO
  return d->ticketCounter++;
}

int Akonadi::SearchProviderBase::fetchResponse(const QList< int > uids, const QString & field)
{
  SearchProviderThread* thread = workerThread( d->ticketCounter++ );
  connect( thread, SIGNAL(finished(SearchProviderThread*)), SLOT(slotThreadFinished(SearchProviderThread*)) );
  thread->start();
  return thread->ticket();
}

void Akonadi::SearchProviderBase::slotThreadFinished(SearchProviderThread * thread)
{
  emit fetchResponseAvailable( thread->ticket(), thread->fetchResponse() );
}

#include  "searchproviderbase.moc"
