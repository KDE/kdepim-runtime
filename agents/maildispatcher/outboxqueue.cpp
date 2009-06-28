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

#include <QMultiMap>
#include <QSet>
#include <QTimer>

#include <KDebug>

#include <Akonadi/Attribute>
#include <Akonadi/Item>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/Monitor>
#include <akonadi/kmime/addressattribute.h>
#include <akonadi/kmime/localfolders.h>

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

#include <outboxinterface/dispatchmodeattribute.h>
#include <outboxinterface/sentbehaviourattribute.h>
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
      , futureTimer( 0 )
    {
    }

    OutboxQueue *const q;

    Collection outbox;
    Monitor *monitor;
    QList<Item> queue;
    QSet<Item> futureItems; // keeps track of items removed in the meantime
    QMultiMap<QDateTime, Item> futureMap;
    QTimer *futureTimer;
    qulonglong totalSize;

#if 0
    // If an item is modified externally between the moment we pass it to
    // the MDA and the time the MDA marks it as sent, then we will get
    // itemChanged() and may mistakenly re-add the item to the queue.
    // So we ignore the item that we pass to the MDA, until the MDA finishes
    // sending it.
    Item currentItem;
#endif
    // HACK: The above is not enough.
    // Apparently change notifications are delayed sometimes (???)
    // and we re-add an item long after it was sent.  So keep a list of sent
    // items.
    // TODO debug and figure out why this happens.
    QSet<Item::Id> ignore;

    void initQueue();
    void addIfComplete( const Item &item );

    // slots:
    void checkFuture();
    void collectionFetched( KJob *job );
    void itemFetched( KJob *job );
    void outboxChanged();
    void itemAdded( const Item &item );
    void itemChanged( const Item &item );
    void itemMoved( const Item &item, const Collection &source, const Collection &dest );
    void itemRemoved( const Item &item );
    void itemProcessed( const Item &item, bool result );

};


void OutboxQueue::Private::initQueue()
{
  totalSize = 0;
  queue.clear();

  kDebug() << "Fetching items in collection" << outbox.id();
  ItemFetchJob *job = new ItemFetchJob( outbox );
  job->fetchScope().fetchAllAttributes();
  job->fetchScope().fetchFullPayload( false );
  connect( job, SIGNAL( result( KJob* ) ), q, SLOT( collectionFetched( KJob* ) ) );
}

void OutboxQueue::Private::addIfComplete( const Item &item )
{
  if( ignore.contains( item.id() ) ) {
    kDebug() << "Item" << item.id() << "is ignored.";
    return;
  }

  if( queue.contains( item ) ) {
    kDebug() << "Item" << item.id() << "already in queue!";
    return;
  }

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
      !item.hasAttribute<SentBehaviourAttribute>() ||
      !item.hasAttribute<TransportAttribute>() ) {
    kWarning() << "Item" << item.id() << "does not have all required attributes.";
    return;
  }

  if( !item.hasFlag( "queued" ) ) {
    kDebug() << "Item" << item.id() << "has no 'queued' flag.";
    return;
  }

  const DispatchModeAttribute *mA = item.attribute<DispatchModeAttribute>();
  Q_ASSERT( mA );
  if( mA->dispatchMode() == DispatchModeAttribute::Never ) {
    kDebug() << "Item" << item.id() << "is queued to be sent manually.";
    return;
  }

  const TransportAttribute *tA = item.attribute<TransportAttribute>();
  Q_ASSERT( tA );
  if( tA->transport() == 0 ) {
    kWarning() << "Item" << item.id() << "has invalid transport.";
    return;
  }

  const SentBehaviourAttribute *sA = item.attribute<SentBehaviourAttribute>();
  Q_ASSERT( sA );
  if( sA->sentBehaviour() == SentBehaviourAttribute::MoveToCollection &&
      sA->moveToCollection() < 0 ) {
    kWarning() << "Item" << item.id() << "has invalid sent-mail collection.";
    return;
  }

  // This check requires fetchFullPayload. -> slow (?)
  /*
  if( !item.hasPayload<KMime::Message::Ptr>() ) {
    kWarning() << "Item" << item.id() << "does not have KMime::Message::Ptr payload.";
    return;
  }
  */

  if( mA->dispatchMode() == DispatchModeAttribute::AfterDueDate &&
      mA->dueDate() > QDateTime::currentDateTime() ) {
    // All the above was OK, so accept it for the future.
    kDebug() << "Item" << item.id() << "is accepted to be sent in the future.";
    futureMap.insert( mA->dueDate(), item );
    Q_ASSERT( !futureItems.contains( item ) );
    futureItems.insert( item );
    checkFuture();
    return;
  }

  kDebug() << "Item" << item.id() << "is accepted into the queue (size" << item.size() << ").";
  Q_ASSERT( !queue.contains( item ) );
  totalSize += item.size();
  queue.append( item );
  emit q->newItems();
}

void OutboxQueue::Private::checkFuture()
{
  kDebug() << "The future is here." << futureMap.count() << "items in futureMap.";
  Q_ASSERT( futureTimer );
  futureTimer->stop();
  // By default, re-check in one hour.
  futureTimer->setInterval( 60 * 60 * 1000 );

  // Check items in ascending order of date.
  while( !futureMap.isEmpty() ) {
    QMap<QDateTime, Item>::iterator it = futureMap.begin();
    kDebug() << "Item with due date" << it.key();
    if( it.key() > QDateTime::currentDateTime() ) {
      int secs = QDateTime::currentDateTime().secsTo( it.key() ) + 1;
      kDebug() << "Future, in" << secs << "seconds.";
      Q_ASSERT( secs >= 0 );
      if( secs < 60 * 60 ) {
        futureTimer->setInterval( secs * 1000 );
      }
      break; // all others are in the future too
    }
    if( !futureItems.contains( it.value() ) ) {
      kDebug() << "Item disappeared.";
    } else {
      kDebug() << "Due date is here. Queuing.";
      addIfComplete( it.value() );
      futureItems.remove( it.value() );
    }
    futureMap.erase( it );
  }

  kDebug() << "Timer set to checkFuture again in" << futureTimer->interval() / 1000 << "seconds"
    << "(that is" << futureTimer->interval() / 1000 / 60 << "minutes).";
  futureTimer->start();
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
  foreach( const Item &item, fjob->items() ) {
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
    itemRemoved( item );
  } else if( dest == outbox ) {
    addIfComplete( item );
  }
}

void OutboxQueue::Private::itemRemoved( const Item &item )
{
  // @p item has size=0, so get the size from our own copy.
  int idx = queue.indexOf( item );
  if( idx == -1 ) {
    // Item was not in queue at all.
    return;
  }
  Item my( queue.takeAt( idx ) );
  kDebug() << "Item" << my.id() << "(size" << my.size() << ") was removed from the queue.";
  totalSize -= my.size();

  futureItems.remove( item );
}

void OutboxQueue::Private::itemProcessed( const Item &item, bool result )
{
  Q_ASSERT( ignore.contains( item.id() ) );
  if( !result ) {
    // Give the user a chance to re-send the item if it failed.
    ignore.remove( item.id() );
  }
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

  d->futureTimer = new QTimer( this );
  connect( d->futureTimer, SIGNAL(timeout()), this, SLOT(checkFuture()) );
  d->futureTimer->start( 60 * 60 * 1000 ); // 1 hour
}

OutboxQueue::~OutboxQueue()
{
  delete d;
}

bool OutboxQueue::isEmpty() const
{
  return d->queue.isEmpty();
}

int OutboxQueue::count() const
{
  if( d->queue.count() == 0 ) {
    // TODO Is this asking for too much?
    Q_ASSERT( d->totalSize == 0 );
  }
  return d->queue.count();
}

qulonglong OutboxQueue::totalSize() const
{
  return d->totalSize;
}

void OutboxQueue::fetchOne()
{
  if( isEmpty() ) {
    kDebug() << "Empty queue.";
    return;
  }

  Item item = d->queue.takeFirst();
  d->totalSize -= item.size();
  Q_ASSERT( !d->ignore.contains( item.id() ) );
  d->ignore.insert( item.id() );

  ItemFetchJob *job = new ItemFetchJob( item );
  job->fetchScope().fetchAllAttributes();
  job->fetchScope().fetchFullPayload();
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( itemFetched( KJob* ) ) );
}


#include "outboxqueue.moc"
