/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "outboxqueue.h"

#include <KDebug>

#include <Akonadi/Attribute>
#include <Akonadi/Item>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/Monitor>

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

#include <outboxinterface/localfolders.h>
#include <outboxinterface/addressattribute.h>
#include <outboxinterface/dispatchmodeattribute.h>
#include <outboxinterface/sentcollectionattribute.h>
#include <outboxinterface/transportattribute.h>

using namespace Akonadi;
using namespace OutboxInterface;


/**
 * @internal
 */
class OutboxQueue::Private
{
  public:
    Private( OutboxQueue *qq )
      : q( qq )
      , outbox( -1 )
    {
    }


    OutboxQueue *const q;

    Collection outbox;
    Monitor *monitor;
    QList<Item> queue;

    void initQueue();
    void addIfComplete( const Item &item );

    // slots:
    void collectionFetched( KJob *job );
    void itemFetched( KJob *job );
    void outboxChanged();
    void itemAdded( const Item &item );
    void itemChanged( const Item &item );
    void itemMoved( const Item &item, const Collection &source, const Collection &dest );
    void itemRemoved( const Item &item );

};


void OutboxQueue::Private::initQueue()
{
  queue.clear();

  kDebug() << "Fetching items in collection" << outbox.id();
  ItemFetchJob *job = new ItemFetchJob( outbox );
  job->fetchScope().fetchAllAttributes();
  job->fetchScope().fetchFullPayload( false );
  connect( job, SIGNAL( result( KJob* ) ), q, SLOT( collectionFetched( KJob* ) ) );
}

void OutboxQueue::Private::addIfComplete( const Item &item )
{
  if( item.remoteId().isEmpty() ) {
    kDebug() << "Item" << item.id() << "has an empty remoteId.";
    // HACK:
    // This probably means that it hasn't yet been stored on disk by the
    // maildir resource, so I'll let it go for now, and process it when
    // it's ready.
    return;
  }

  if( !item.hasAttribute<AddressAttribute>() ||
      !item.hasAttribute<DispatchModeAttribute>() ||
      !item.hasAttribute<SentCollectionAttribute>() ||
      !item.hasAttribute<TransportAttribute>() ) {
    kWarning() << "Item" << item.id() << "does not have all required attributes.";
    return;
  }

  if( !item.hasFlag( "queued" ) ) {
    kDebug() << "Item" << item.id() << "has no 'queued' flag.";
    return;
  }

  const DispatchModeAttribute *mA = item.attribute<DispatchModeAttribute>();
  if( mA->dispatchMode() == DispatchModeAttribute::AfterDueDate &&
      mA->dueDate() > QDateTime::currentDateTime() ) {
    kDebug() << "Item" << item.id() << "is to be sent in the future.";
    return;

    // TODO: QTimer to check again on due date.
  }

  const TransportAttribute *tA = item.attribute<TransportAttribute>();
  if( tA->transport() == 0 ) {
    kWarning() << "Item" << item.id() << "has invalid transport.";
    return;
  }

  // This check requires fetchFullPayload. -> slow (?)
  /*
  if( !item.hasPayload<KMime::Message::Ptr>() ) {
    kWarning() << "Item" << item.id() << "does not have KMime::Message::Ptr payload.";
    return;
  }
  */

  kDebug() << "Item" << item.id() << "is accepted into the queue.";
  queue.append( item );
  emit q->newItems();
}

void OutboxQueue::Private::collectionFetched( KJob *job )
{
  if( job->error() ) {
    kWarning() << "Failed to fetch outbox collection.  Queue will be empty until the outbox changes.";
    return;
  }

  ItemFetchJob *fjob = dynamic_cast<ItemFetchJob*>( job );
  Q_ASSERT( fjob );
  kDebug() << "Fetched" << fjob->items().count() << "items.";
  foreach( Item item, fjob->items() ) {
    addIfComplete( item );
  }
}

void OutboxQueue::Private::itemFetched( KJob *job )
{
  if( job->error() ) {
    kDebug() << "Error fetching item:" << job->errorString() << ". Trying next item in queue.";
    q->fetchOne();
  }

  ItemFetchJob *fjob = dynamic_cast<ItemFetchJob*>( job );
  Q_ASSERT( fjob );
  if( fjob->items().count() != 1 ) {
    kDebug() << "Fetched" << fjob->items().count() << ", expected 1. Trying next item in queue.";
    q->fetchOne();
  }

  emit q->itemReady( fjob->items().first() );
}

void OutboxQueue::Private::outboxChanged()
{
  // Called on startup, and whenever the local folders change.
  Collection col = LocalFolders::self()->outbox();
  Q_ASSERT( col.isValid() );
  if( col == outbox ) {
    return;
  }

  monitor->setCollectionMonitored( outbox, false );
  monitor->setCollectionMonitored( col, true );
  outbox = col;
  kDebug() << "Changed outbox to" << outbox.id();
  initQueue();
}

void OutboxQueue::Private::itemAdded( const Item &item )
{
  addIfComplete( item );
}

void OutboxQueue::Private::itemChanged( const Item &item )
{
  addIfComplete( item );
  // TODO: if the item is moved out of the outbox, will I get itemChanged?
}

void OutboxQueue::Private::itemMoved( const Item &item, const Collection &source, const Collection &dest )
{
  if( source == outbox ) {
    queue.removeAll( item );
  } else if( dest == outbox ) {
    addIfComplete( item );
  }
}

void OutboxQueue::Private::itemRemoved( const Item &item )
{
  queue.removeAll( item );
}




OutboxQueue::OutboxQueue( QObject *parent )
  : QObject( parent )
  , d( new Private( this ) )
{
  d->monitor = new Monitor( this );
  d->monitor->itemFetchScope().fetchAllAttributes();
  d->monitor->itemFetchScope().fetchFullPayload( false );
  connect( d->monitor, SIGNAL( itemAdded( Akonadi::Item, Akonadi::Collection ) ),
      this, SLOT( itemAdded( Akonadi::Item ) ) );
  connect( d->monitor, SIGNAL( itemChanged( Akonadi::Item, QSet<QByteArray> ) ),
      this, SLOT( itemChanged( Akonadi::Item ) ) );
  connect( d->monitor, SIGNAL( itemMoved( Akonadi::Item, Akonadi::Collection, Akonadi::Collection ) ),
      this, SLOT( itemMoved( Akonadi::Item, Akonadi::Collection, Akonadi::Collection ) ) );
  connect( d->monitor, SIGNAL( itemRemoved( Akonadi::Item ) ),
      this, SLOT( itemRemoved( Akonadi::Item ) ) );

  connect( LocalFolders::self(), SIGNAL( foldersReady() ),
      this, SLOT( outboxChanged() ) );
  LocalFolders::self()->fetch();
}

OutboxQueue::~OutboxQueue()
{
  delete d;
}

bool OutboxQueue::isEmpty() const
{
  return d->queue.isEmpty();
}

void OutboxQueue::fetchOne()
{
  if( isEmpty() ) {
    kDebug() << "Empty queue.";
    return;
  }

  Item item = d->queue.takeFirst();

  ItemFetchJob *job = new ItemFetchJob( item );
  job->fetchScope().fetchAllAttributes();
  job->fetchScope().fetchFullPayload();
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( itemFetched( KJob* ) ) );
}


#include "outboxqueue.moc"
