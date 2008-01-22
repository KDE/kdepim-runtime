// -*- c-basic-offset: 2 -*-
/*
    This file is part of libkabc.
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#include "resourceakonadi.h"
#include "resourceakonadiconfig.h"

#include <libakonadi/collection.h>
#include <libakonadi/monitor.h>
#include <libakonadi/item.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemdeletejob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>

#include <kdebug.h>
#include <kconfiggroup.h>

#include <QHash>

using namespace Akonadi;
using namespace KABC;

typedef QHash<QString, Item> ItemHash;

class ResourceAkonadi::Private
{
  public:
    Private( ResourceAkonadi *parent )
      : mParent( parent ), mMonitor( new Monitor( mParent ) )
    {
    }

 public:
    ResourceAkonadi *mParent;

    Monitor *mMonitor;

    Collection mCollection;

    ItemHash mItems;

    Addressee::Map mAddrToRemove;

  public:
    void itemAdded( const Akonadi::Item& item, const Akonadi::Collection& collection );
    void itemChanged( const Akonadi::Item& item, const QStringList& partIdentifiers );
    void itemRemoved( const Akonadi::DataReference& reference );
};

ResourceAkonadi::ResourceAkonadi()
  : Resource(), d( new Private( this ) )
{
  init();
}

ResourceAkonadi::ResourceAkonadi( const KConfigGroup &group )
  : Resource( group ), d( new Private( this ) )
{
  KUrl url = group.readEntry( "CollectionUrl", KUrl() );

  if ( !url.isValid() ) {
    // TODO error handling
  } else {
    d->mCollection = Collection::fromUrl( url );
  }

  init();
}

ResourceAkonadi::~ResourceAkonadi()
{
  delete d;
}

void ResourceAkonadi::init()
{
  connect( d->mMonitor,
           SIGNAL( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ),
           this,
           SLOT( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ) );

  connect( d->mMonitor,
           SIGNAL( itemChanged( const Akonadi::Item&, const QStringList& ) ),
           this,
           SLOT( itemChanged( const Akonadi::Item&, const QStringList& ) ) );

  connect( d->mMonitor,
           SIGNAL( itemRemoved( const Akonadi::DataReference&) ),
           this,
           SLOT( itemRemoved( const Akonadi::DataReference& ) ) );
}

void ResourceAkonadi::writeConfig( KConfigGroup &group )
{
  group.writeEntry( "CollectionUrl", d->mCollection.url() );

  Resource::writeConfig( group );
}

bool ResourceAkonadi::doOpen()
{
  if ( !d->mCollection.isValid() )
    return false;

  // TODO: probably check here if collection exists

  d->mMonitor->monitorCollection( d->mCollection );

  return true;
}

void ResourceAkonadi::doClose()
{
}

Ticket *ResourceAkonadi::requestSaveTicket()
{
  if ( !addressBook() ) {
    kDebug(5700) << "no addressbook";
    return 0;
  }

  return createTicket( this );
}

void ResourceAkonadi::releaseSaveTicket( Ticket *ticket )
{
  delete ticket;
}

bool ResourceAkonadi::load()
{
  kDebug(5700);

  clear();

  ItemFetchJob *job = new ItemFetchJob( d->mCollection );
  job->fetchAllParts();

  if ( !job->exec() ) {
    // TODO: error handling
    return false;
  }

  Item::List items = job->items();

  kDebug(5700) << "Item fetch produced" << items.count() << "items";

  foreach ( const Item& item, items ) {
    if ( item.hasPayload<Addressee>() ) {
      Addressee addressee = item.payload<Addressee>();
      addressee.setResource( this );

      mAddrMap.insert( addressee.uid(), addressee );
      d->mItems.insert( addressee.uid(), item );
    }
  }

  return true;
}

bool ResourceAkonadi::asyncLoad()
{
  clear();

  ItemFetchJob *job = new ItemFetchJob( d->mCollection );
  job->fetchAllParts();

  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( loadResult( KJob* ) ) );

  job->start();

  return true;
}

bool ResourceAkonadi::save( Ticket *ticket )
{
  Q_UNUSED( ticket );
  kDebug(5700);

  // first delete items scheduled for removal
  const Addressee::Map removals = d->mAddrToRemove;
  d->mAddrToRemove.clear();

  Addressee::Map::const_iterator addrIt    = removals.begin();
  Addressee::Map::const_iterator addrEndIt = removals.end();
  for ( ; addrIt != addrEndIt; ++addrIt ) {
    ItemHash::iterator itemIt = d->mItems.find( addrIt.key() );

    if ( itemIt != d->mItems.end() ) {
      Item item = itemIt.value();
      d->mItems.erase( itemIt );

      ItemDeleteJob *job = new ItemDeleteJob( item.reference() );

      if ( !job->exec() ) {
        // TODO error handling
        kWarning(5700) << "Deleting item failed:" << job->errorString();
      } else {
        kDebug(5700) << "Deleting item succeeded";
      }
    }
  }

  // then append new ones and store all modified ones
  addrIt    = mAddrMap.begin();
  addrEndIt = mAddrMap.end();
  for ( ; addrIt != addrEndIt; ++addrIt ) {
    kDebug(5700) << "Processing addressee" << addrIt.value().formattedName();
    ItemHash::const_iterator itemIt = d->mItems.find( addrIt.key() );

    if ( itemIt != d->mItems.end() ) {
      Item item = itemIt.value();
      item.setPayload<Addressee>( addrIt.value() );

      ItemStoreJob *job = new ItemStoreJob( item );
      job->storePayload();

      if ( !job->exec() ) {
        // TODO error handling
        kWarning(5700) << "Updating item failed:" << job->errorString();
      } else {
        kDebug(5700) << "Updating item succeeded";
      }
    } else {
      Item item( "text/directory" );
      item.setPayload<Addressee>( addrIt.value() );

      ItemAppendJob *job = new ItemAppendJob( item, d->mCollection );

      if ( !job->exec() ) {
        // TODO error handling
        kWarning(5700) << "Appending item failed:" << job->errorString();
      } else {
        kDebug(5700) << "Appending item succeeded";
      }
    }
  }

  return true;
}

bool ResourceAkonadi::asyncSave( Ticket *ticket )
{
  Q_UNUSED( ticket );
  kDebug(5700);

  // TODO do real async implementation

  if ( !save( ticket ) ) {
    emit savingError( this, QString() );
    return false;
  }

  emit savingFinished( this );

  return true;
}

void ResourceAkonadi::insertAddressee( const Addressee &addr )
{
  Resource::insertAddressee( addr );
}

void ResourceAkonadi::removeAddressee( const Addressee &addr )
{
  Addressee::Map::const_iterator findIt = mAddrMap.find( addr.uid() );
  if ( findIt == mAddrMap.end() )
    return;

  d->mAddrToRemove.insert( addr.uid(), addr );

  Resource::removeAddressee( addr );
}

void ResourceAkonadi::setCollection( const Collection& collection )
{
  // TODO: what to do when loaded?
  d->mCollection = collection;
}

Collection ResourceAkonadi::collection() const
{
  return d->mCollection;
}

void ResourceAkonadi::loadResult( KJob *job )
{
  if ( job->error() != 0 ) {
    emit loadingError( this, job->errorString() );
    return;
  }

  ItemFetchJob *fetchJob = dynamic_cast<ItemFetchJob*>( job );
  Q_ASSERT( fetchJob != 0 );

  Item::List items = fetchJob->items();

  kDebug(5700) << "Item fetch produced" << items.count() << "items";

  foreach ( const Item& item, items ) {
    if ( item.hasPayload<Addressee>() ) {
      Addressee addressee = item.payload<Addressee>();
      addressee.setResource( this );

      mAddrMap.insert( addressee.uid(), addressee );
      d->mItems.insert( addressee.uid(), item );
    }
  }

  emit loadingFinished( this );
}

void ResourceAkonadi::saveResult( KJob *job )
{
  Q_UNUSED( job );
}

void ResourceAkonadi::Private::itemAdded( const Akonadi::Item& item,
                                          const Akonadi::Collection& collection )
{
  if ( collection != mCollection )
    return;

  // TODO: can we assume that remoteId == Addressee.uid()?
  mItems.insert( item.reference().remoteId(), item );
}

void ResourceAkonadi::Private::itemChanged( const Akonadi::Item& item,
                                            const QStringList& partIdentifiers )
{
  Q_UNUSED( item );
  Q_UNUSED( partIdentifiers );
}

void ResourceAkonadi::Private::itemRemoved( const Akonadi::DataReference& reference )
{
  // TODO: can we assume that remoteId == Addressee.uid()?
  mAddrToRemove.remove( reference.remoteId() );
  mItems.remove( reference.remoteId() );
}

#include "resourceakonadi.moc"
