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
#include <libakonadi/itemsync.h>

#include <kdebug.h>
#include <kconfiggroup.h>

#include <QHash>

using namespace Akonadi;
using namespace KABC;

typedef QHash<QString, Item> ItemHash;

// Will probably have to implement it ourselves, ItemSync is
// currently not exported by libakonadi and we use a local copy
// in ../libakonadi
class SaveSequence : public ItemSync
{
  public:
    SaveSequence( const Collection& collection ) : ItemSync( collection )
    {
    }
};

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

  public:
    void itemAdded( const Akonadi::Item& item, const Akonadi::Collection& collection );
    void itemChanged( const Akonadi::Item& item, const QStringList& partIdentifiers );
    void itemRemoved( const Akonadi::DataReference& reference );

    KJob *createSaveSequence() const;
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
  // deactivate reacting to changes, will be enabled in doOpen()
  d->mMonitor->blockSignals( true );

  d->mMonitor->monitorMimeType( "text/directory" );
  d->mMonitor->fetchAllParts();

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

  // activate reacting to changes
  d->mMonitor->blockSignals( false );

  return true;
}

void ResourceAkonadi::doClose()
{
  // deactivate reacting to changes
  d->mMonitor->blockSignals( true );
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

  KJob *job = d->createSaveSequence();
  if ( job == 0 )
    return false;

  if ( !job->exec() ) {
    // TODO error handling
    kError(5700) << "Save Sequence failed:" << job->errorString();
    return false;
  }

  return true;
}

bool ResourceAkonadi::asyncSave( Ticket *ticket )
{
  Q_UNUSED( ticket );
  kDebug(5700);

  KJob *job = d->createSaveSequence();
  if ( job == 0 )
    return false;

  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( saveResult( KJob* ) ) );

  job->start();

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
  if ( job->error() != 0 ) {
    emit savingError( this, job->errorString() );
  } else {
    emit savingFinished( this );
  }
}

void ResourceAkonadi::Private::itemAdded( const Akonadi::Item& item,
                                          const Akonadi::Collection& collection )
{
  if ( collection != mCollection )
    return;

  if ( !item.hasPayload<Addressee>() ) {
    kError(5700) << "Item does not have addressee payload";
    return;
  }

  Addressee addressee = item.payload<Addressee>();

  kDebug(5700) << "Addressee" << addressee.uid() << "("
               << addressee.formattedName() << ")";

  mParent->mAddrMap.insert( addressee.uid(), addressee );
  mItems.insert( addressee.uid(), item );

  mParent->addressBook()->emitAddressBookChanged();
}

void ResourceAkonadi::Private::itemChanged( const Akonadi::Item& item,
                                            const QStringList& partIdentifiers )
{
  Q_UNUSED( partIdentifiers );

  if ( !item.hasPayload<Addressee>() ) {
    kError(5700) << "Item does not have addressee payload";
    return;
  }

  Addressee addressee = item.payload<Addressee>();

  kDebug(5700) << "Addressee" << addressee.uid() << "("
               << addressee.formattedName() << ")";

  ItemHash::iterator itemIt = mItems.find( addressee.uid() );
  if ( itemIt == mItems.end() ) {
    kError(5700) << "Item for addressee " << addressee.uid() << "("
                 << addressee.formattedName() << ") not in local list";
    return;
  }

  itemIt.value() = item;

  Addressee::Map::iterator addrIt = mParent->mAddrMap.find( addressee.uid() );
  if ( addrIt == mParent->mAddrMap.end() ) {
    kWarning(5700) << "Addressee  " << addressee.uid() << "("
                 << addressee.formattedName()
                 << ") changed but no longer in local list";
    return;
  }

  addrIt.value() = addressee;

  mParent->addressBook()->emitAddressBookChanged();
}

void ResourceAkonadi::Private::itemRemoved( const Akonadi::DataReference& reference )
{
  ItemHash::iterator itemIt = mItems.find( reference.remoteId() );
  if ( itemIt == mItems.end() )
    return;

  if ( itemIt.value().reference().id() != reference.id() )
    return;

  mItems.erase( itemIt );

  Addressee::Map::iterator addrIt = mParent->mAddrMap.find( reference.remoteId() );

  // if it does not exist as an addresse we already removed it locally
  if ( addrIt == mParent->mAddrMap.end() )
    return;

  kDebug(5700) << "Addressee" << addrIt.value().uid() << "("
               << addrIt.value().formattedName() << ")";

  mParent->mAddrMap.erase( addrIt );

  mParent->addressBook()->emitAddressBookChanged();
}

KJob *ResourceAkonadi::Private::createSaveSequence() const
{
  Item::List items;

  Addressee::Map::const_iterator addrIt    = mParent->mAddrMap.begin();
  Addressee::Map::const_iterator addrEndIt = mParent->mAddrMap.end();
  for ( ; addrIt != addrEndIt; ++addrIt ) {
    Addressee addressee = addrIt.value();
    ItemHash::const_iterator itemIt = mItems.find( addressee.uid() );

    if ( itemIt == mItems.end() ) {
      Item item( "text/directory" );
      item.setPayload<Addressee>( addressee );

      DataReference reference = item.reference();
      reference.setRemoteId( addressee.uid() );
      item.setReference( reference );

      items << item;
    } else {
      Item item = itemIt.value();
      item.setPayload<Addressee>( addressee );

      items << item;
    }
  }

  SaveSequence *job = new SaveSequence( mCollection );
  job->setRemoteItems( items );

  return job;
}

#include "resourceakonadi.moc"
