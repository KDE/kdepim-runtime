/*  This file is part of the KDE project
    Copyright (C) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "resourcebase.h"

#include <akonadi/filestore/collectionfetchjob.h>
#include <akonadi/filestore/itemcreatejob.h>
#include <akonadi/filestore/itemdeletejob.h>
#include <akonadi/filestore/itemfetchjob.h>
#include <akonadi/filestore/itemmodifyjob.h>
#include <akonadi/filestore/storecompactjob.h>
#include <akonadi/filestore/storeinterface.h>

#include <akonadi/changerecorder.h>
#include <akonadi/itemfetchscope.h>

#include <KLocale>

#include <QtDBus/QDBusConnection>

using namespace Akonadi::FileStore;

ResourceBase::ResourceBase( const QString &id )
  : Akonadi::ResourceBase( id ),
    mStore( 0 )
{
  changeRecorder()->fetchCollection( true );
  changeRecorder()->itemFetchScope().fetchFullPayload();
}

ResourceBase::~ResourceBase()
{
  delete mStore;
}

void ResourceBase::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  kDebug() << "item.id=" << item.id() << "mimetype=" << item.mimeType()
           << "collection.id=" << collection.id() << "remoteId=" << collection.remoteId();
  if ( mStore == 0 ) {
    // TODO better message
    cancelTask( i18n( "Resource not configured yet" ) );
  } else {
    ItemCreateJob *job = mStore->createItem( item, collection );
    kDebug() << "ItemCreateJob" << (void*) job;
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( itemCreateDone( KJob* ) ) );
  }
}

void ResourceBase::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  kDebug() << "item.id=" << item.id() << "remoteId=" << item.remoteId()
           << "mimetype=" << item.mimeType();
  if ( mStore == 0 ) {
    // TODO better message
    cancelTask( i18n( "Resource not configured yet" ) );
  } else {
    ItemModifyJob *job = mStore->modifyItem( item, parts.isEmpty() );
    kDebug() << "ItemModifyJob" << (void*) job;
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( itemModifyDone( KJob* ) ) );
  }
}

void ResourceBase::itemRemoved( const Akonadi::Item &item )
{
  kDebug() << "item.id=" << item.id() << "remoteId=" << item.remoteId()
           << "mimetype=" << item.mimeType();
  if ( mStore == 0 ) {
    // TODO better message
    cancelTask( i18n( "Resource not configured yet" ) );
  } else {
    ItemDeleteJob *job = mStore->deleteItem( item );
    kDebug() << "ItemDeleteJob" << (void*) job;
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( itemDeleteDone( KJob* ) ) );
  }
}

void ResourceBase::retrieveCollections()
{
  kDebug();
  if ( mStore == 0 ) {
    // TODO better message
    cancelTask( i18n( "Resource not configured yet" ) );
  } else {
    setCollectionStreamingEnabled( true );

    Akonadi::Collection topLevelCollection = mStore->topLevelCollection();
    topLevelCollection.setParentCollection( Akonadi::Collection::root() );

    kDebug() << "topLevelCollection.remoteId=" << topLevelCollection.remoteId();

    collectionsRetrieved( Akonadi::Collection::List() << topLevelCollection );

    CollectionFetchJob *job = mStore->fetchCollections( topLevelCollection, CollectionFetchJob::Recursive );
    kDebug() << "CollectionFetchJob" << (void*) job;
    connect( job, SIGNAL( collectionsReceived( Akonadi::Collection::List ) ),
             this, SLOT( collectionsReceived( Akonadi::Collection::List ) ) );
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( collectionFetchDone( KJob* ) ) );
  }
}

void ResourceBase::retrieveItems( const Akonadi::Collection &collection )
{
  kDebug() << "collection.id=" << collection.id() << "remoteId=" << collection.remoteId();
  if ( mStore == 0 ) {
    // TODO better message
    cancelTask( i18n( "Resource not configured yet" ) );
  } else {
    setItemStreamingEnabled( true );

    ItemFetchJob *job = mStore->fetchItems( collection );
    kDebug() << "ItemFetchJob" << (void*) job;
    connect( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ),
             this, SLOT( itemsReceived( Akonadi::Item::List ) ) );
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( itemFetchDone( KJob* ) ) );
  }
}

bool ResourceBase::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  kDebug() << "item.id=" << item.id() << "remoteId=" << item.remoteId();
  if ( mStore == 0 ) {
    // TODO better message
    cancelTask( i18n( "Resource not configured yet" ) );
  } else {
    ItemFetchJob *job = mStore->fetchItem( item );
    kDebug() << "ItemFetchJob" << (void*) job;

    job->fetchScope().fetchAllAttributes( true );
    if ( parts.contains( Akonadi::Item::FullPayload ) ) {
      job->fetchScope().fetchFullPayload( true );
    } else {
      foreach ( const QByteArray &part, parts ) {
        job->fetchScope().fetchPayloadPart( part );
      }
    }
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( itemFetchDone( KJob* ) ) );
  }

  return mStore != 0;
}

void ResourceBase::collectionsReceived( const Akonadi::Collection::List &collections )
{
  kDebug() << collections.count() << "collections";
  foreach ( const Akonadi::Collection &collection, collections ) {
    kDebug() << "collection.remoteId=" << collection.remoteId()
             << "parent.remoteId=" << collection.parentCollection().remoteId();
  }
  collectionsRetrieved( collections );
}

void ResourceBase::collectionFetchDone( KJob * job )
{
  kDebug() << "job.error=" << job->error() << "errorText=" << job->errorText();
  if ( job->error() != 0 ) {
    cancelTask( job->errorText() );
  } else {
    collectionsRetrievalDone();
  }
}

void ResourceBase::itemsReceived( const Akonadi::Item::List &items )
{
  kDebug() << items.count() << "items";
  foreach ( const Akonadi::Item &item, items ) {
    kDebug() << "item.remoteId=" << item.remoteId()
             << "parent.remoteId=" << item.parentCollection().remoteId();
  }
  itemsRetrieved( items );
}

void ResourceBase::itemFetchDone( KJob * job )
{
  kDebug() << "job.error=" << job->error() << "errorText=" << job->errorText();
  if ( job->error() != 0 ) {
    cancelTask( job->errorText() );
  } else {
    // check whether this is for retrieveItems() or retrieveItem()
    ItemFetchJob *itemFetch = dynamic_cast<ItemFetchJob*>( job );
    kDebug() << "ItemFetchJob" << (void*) itemFetch;
    Q_ASSERT( itemFetch != 0 );

    if ( !itemFetch->collection().remoteId().isEmpty() ) {
      itemsRetrievalDone();
    } else {
      Q_ASSERT( itemFetch->items().count() == 1 );
      kDebug() << "item.id=" << itemFetch->items().first().id()
               << "item.remoteId=" << itemFetch->items().first().remoteId()
               << "parent.remoteId=" << itemFetch->items().first().parentCollection().remoteId();

      itemRetrieved( itemFetch->items().first() );
    }
  }
}

void ResourceBase::itemCreateDone( KJob *job )
{
  kDebug() << "job.error=" << job->error() << "errorText=" << job->errorText();
  if ( job->error() != 0 ) {
    cancelTask( job->errorText() );
  } else {
    ItemCreateJob *itemCreate = dynamic_cast<ItemCreateJob*>( job );
    kDebug() << "ItemCreateJob" << (void*) itemCreate;
    Q_ASSERT( itemCreate != 0 );

    kDebug() << "item.remoteId=" << itemCreate->item().remoteId()
            << "parent.remoteId=" << itemCreate->item().parentCollection().remoteId();
    changeCommitted( itemCreate->item() );
  }
}

void ResourceBase::itemModifyDone( KJob *job )
{
  kDebug() << "job.error=" << job->error() << "errorText=" << job->errorText();
  if ( job->error() != 0 ) {
    cancelTask( job->errorText() );
  } else {
    ItemModifyJob *itemModify = dynamic_cast<ItemModifyJob*>( job );
    Q_ASSERT( itemModify != 0 );

    kDebug() << "item.remoteId=" << itemModify->item().remoteId()
             << "parent.remoteId=" << itemModify->item().parentCollection().remoteId();
    changeCommitted( itemModify->item() );
  }
}

void ResourceBase::itemDeleteDone( KJob *job )
{
  kDebug() << "job.error=" << job->error() << "errorText=" << job->errorText();
  if ( job->error() != 0 ) {
    cancelTask( job->errorText() );
  } else {
    ItemDeleteJob *itemDelete = dynamic_cast<ItemDeleteJob*>( job );
    Q_ASSERT( itemDelete != 0 );

    kDebug() << "item.remoteId=" << itemDelete->item().remoteId()
            << "parent.remoteId=" << itemDelete->item().parentCollection().remoteId();
    changeCommitted( itemDelete->item() );
  }
}

void ResourceBase::compactStore( const QVariant &parameter )
{
  kDebug();
  Q_UNUSED( parameter );

  if ( mStore == 0 ) {
    // TODO better message
    cancelTask( i18n( "Resource not configured yet" ) );
  } else {
    StoreCompactJob *job = mStore->compactStore();
    kDebug() << "StoreCompactJob" << (void*) job;
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( storeCompactDone( KJob* ) ) );
  }
}

void ResourceBase::storeCompactDone( KJob *job )
{
  kDebug() << "job.error=" << job->error() << "errorText=" << job->errorText();
  if ( job->error() != 0 ) {
    cancelTask( job->errorText() );
  } else {
    kDebug() << "StoreCompactJob" << (void*) job;
    taskDone();
  }
}

#include "resourcebase.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
