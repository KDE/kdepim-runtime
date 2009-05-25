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

#include "itemfilterproxymodel.h"

#include <KDebug>

#include <Akonadi/Attribute>
#include <Akonadi/Item>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModel>

#include <kmime/kmime_message.h>
#include <boost/shared_ptr.hpp>

#include <outboxinterface/addressattribute.h>
#include <outboxinterface/dispatchmodeattribute.h>
#include <outboxinterface/transportattribute.h>

using namespace Akonadi;
using namespace OutboxInterface;


/**
 * @internal
 */
class ItemFilterProxyModel::Private
{
  public:
    Private( ItemFilterProxyModel *qq )
      : q( qq )
    {
      currentJob = 0;
    }

    bool itemAccepted( const QModelIndex &index );

    // slot
    void itemFetchResult( KJob *job );

    ItemFilterProxyModel *q;
    ItemFetchJob *currentJob;

};


bool ItemFilterProxyModel::Private::itemAccepted( const QModelIndex &index )
{
  const Item item = q->sourceModel()->data( index, ItemModel::ItemRole ).value<Item>();

  if( item.remoteId().isEmpty() ) {
    kDebug() << "item" << item.id() << "has an empty remoteId.";
    // This probably means that it hasn't yet been stored on disk by the
    // maildir resource, so I'll let it go for now, and process it when
    // it's ready.
    return false;
  }

  if( !item.hasAttribute<AddressAttribute>() ||
      !item.hasAttribute<DispatchModeAttribute>() ||
      !item.hasAttribute<TransportAttribute>() ) {
    kWarning() << "item" << item.id() << "does not have all required attributes.";
    return false;
  }

  if( !item.hasFlag( "queued" ) ) {
    kDebug() << "item" << item.id() << "has no 'queued' flag.";
    return false;
  }

  const DispatchModeAttribute *mA = item.attribute<DispatchModeAttribute>();
  if( mA->dispatchMode() == DispatchModeAttribute::AfterDueDate &&
      mA->dueDate() > QDateTime::currentDateTime() ) {
    kDebug() << "item" << item.id() << "is to be sent in the future.";
    return false;

    // TODO: something needs to tell the model to re-check these when time
    // passes.  Find out how the imap resource does interval checks.
  }

  const TransportAttribute *tA = item.attribute<TransportAttribute>();
  if( tA->transport() == 0 ) {
    kWarning() << "item" << item.id() << "has invalid transport.";
    return false;
  }

  // This check requires that the source ItemModel have fetchScope.fetchFullPayload.
  // That takes time (I assume), so let's not do it. TODO: ask around
  /*
  if( !item.hasPayload<KMime::Message::Ptr>() ) {
    kWarning() << "item" << item.id() << "does not have KMime::Message::Ptr payload.";
    return false;
  }
  */

  kDebug() << "item" << item.id() << "is accepted into the model.";
  return true;
}

void ItemFilterProxyModel::Private::itemFetchResult( KJob *job )
{
  Q_ASSERT( job == currentJob );
  Item::List items = currentJob->items();
  if( items.count() != 1 ) {
    kWarning() << "ItemFetchJob fetched" << items.count() << "items; expected 1.";
    
    // TODO: When can this happen?  How to handle this?
    // HACK: Remove from model and try next one.
    Q_ASSERT( q->rowCount() >= 1 );
    q->removeRows( 0, 1 );
    q->fetchAnItem();
  } else {
    Item item = items.first();
    kDebug() << "fetched item" << item.id() << "from collection" << item.collectionId();
    emit q->itemFetched( item );
  }
  Q_UNUSED( job );
  currentJob = 0;
}


ItemFilterProxyModel::ItemFilterProxyModel( QObject *parent )
  : QSortFilterProxyModel( parent ),
    d( new Private( this ) )
{
  // Let me know when the source model changes.
  setDynamicSortFilter( true );

  // TODO: This fixes the problem when the MDA tried to send an item (which
  // involves modifying it), and the maildir resource tried to store the item
  // at the same time (which involves modifying it as well).  This meant about
  // half the time the MDA would get 'NO item was modified elsewhere, STORE
  // aborted'.  But this is expensive, updating the model at every change --
  // look into better ways of doing this.  (Also, why does the maildir
  // resource disable revision number checks?!)
}

ItemFilterProxyModel::~ItemFilterProxyModel()
{
  delete d;
}

void ItemFilterProxyModel::fetchAnItem()
{
  if( rowCount() < 1 ) {
    kWarning() << "empty model!";
    return;
  }

  Item item = data( index( 0, 0 ), ItemModel::ItemRole ).value<Item>();
  Q_ASSERT( d->currentJob == 0 );
  d->currentJob = new ItemFetchJob( item );
  d->currentJob->fetchScope().fetchFullPayload();
  d->currentJob->fetchScope().fetchAllAttributes();
  connect( d->currentJob, SIGNAL( result( KJob* ) ),
      this, SLOT( itemFetchResult( KJob* ) ) );
}

bool ItemFilterProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent) const
{
  return d->itemAccepted( sourceModel()->index( sourceRow, 0, sourceParent ) );
}


#include "itemfilterproxymodel.moc"
