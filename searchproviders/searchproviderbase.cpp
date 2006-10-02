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

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QStringList>

class Akonadi::SearchProviderBasePrivate
{
  public:
    int ticketCounter;
    QHash<SearchProviderThread*,QDBusMessage> pendingReplys;
};

Akonadi::SearchProviderBase::SearchProviderBase( const QString &id ) :
    d( new SearchProviderBasePrivate )
{
  d->ticketCounter = 0;

  new SearchProviderAdaptor( this );
  Q_ASSERT( QDBusConnection::sessionBus().registerService( "org.kde.Akonadi.SearchProvider." + id ) );
  Q_ASSERT( QDBusConnection::sessionBus().registerObject( "/", this, QDBusConnection::ExportAdaptors ) );
}

Akonadi::SearchProviderBase::~ SearchProviderBase()
{
  delete d;
}

int Akonadi::SearchProviderBase::search(const QString & targetCollection, const QString & searchQuery)
{
  qDebug() << "SearchProviderBase::search()" << targetCollection << searchQuery;
  // TODO
  return d->ticketCounter++;
}

// FIXME: move back to QList<int>!
QStringList Akonadi::SearchProviderBase::fetchResponse(const QList<QString> uids, const QString & field, const QDBusMessage &msg)
{
  qDebug() << "SearchProviderBase::fetchResponse()" << uids << field;
  SearchProviderThread* thread = workerThread( d->ticketCounter++ );
  QList<int> list;
  foreach ( QString uid, uids )
    list << uid.toInt();
  thread->requestFetchResponse( list, field );
  return performRequest( thread, msg );
}

void Akonadi::SearchProviderBase::slotThreadFinished(SearchProviderThread * thread)
{
  Q_ASSERT( d->pendingReplys.contains( thread ) );
  QDBusMessage reply = d->pendingReplys.take( thread );
  reply << thread->fetchResponse();
  QDBusConnection::sessionBus().send( reply );
  thread->deleteLater();
}

void Akonadi::SearchProviderBase::quit()
{
  QCoreApplication::quit();
}

QStringList Akonadi::SearchProviderBase::performRequest(SearchProviderThread *thread, const QDBusMessage & msg)
{
  msg.setDelayedReply( true );
  d->pendingReplys.insert( thread, msg.createReply() );
  connect( thread, SIGNAL(finished(SearchProviderThread*)), SLOT(slotThreadFinished(SearchProviderThread*)) );
  thread->start();
  return QStringList();
}

#include  "searchproviderbase.moc"
