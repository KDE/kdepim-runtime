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

#include <akonadi/itemfetchscope.h>

#include <KLocale>

#include <QtDBus/QDBusConnection>

using namespace Akonadi::FileStore;

ResourceBase::ResourceBase( const QString &id )
  : Akonadi::ResourceBase( id ),
    mStore( 0 )
{
  setCollectionStreamingEnabled( true );
  setItemStreamingEnabled( true );
}

ResourceBase::~ResourceBase()
{
  delete mStore;
}

void ResourceBase::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  if ( mStore == 0 ) {
    // TODO better message
    cancelTask( i18n( "Resource not configured yet" ) );
  } else {
    ItemCreateJob *job = mStore->createItem( item, collection );
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( itemCreateDone( KJob* ) ) );
  }
}

void ResourceBase::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  if ( mStore == 0 ) {
    // TODO better message
    cancelTask( i18n( "Resource not configured yet" ) );
  } else {
    ItemModifyJob *job = mStore->modifyItem( item, parts.isEmpty() );
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( itemModifyDone( KJob* ) ) );
  }
}

void ResourceBase::itemRemoved( const Akonadi::Item &item )
{
  if ( mStore == 0 ) {
    // TODO better message
    cancelTask( i18n( "Resource not configured yet" ) );
  } else {
    ItemDeleteJob *job = mStore->deleteItem( item );
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( itemDeleteDone( KJob* ) ) );
  }
}

void ResourceBase::retrieveCollections()
{
  if ( mStore == 0 ) {
    // TODO better message
    cancelTask( i18n( "Resource not configured yet" ) );
  } else {
    Akonadi::Collection topLevelCollection = mStore->topLevelCollection();
    topLevelCollection.setParentCollection( Akonadi::Collection::root() );

    collectionsRetrieved( Akonadi::Collection::List() << topLevelCollection );

    CollectionFetchJob *job = mStore->fetchCollections( topLevelCollection, CollectionFetchJob::Recursive );
    connect( job, SIGNAL( collectionsReceived( Akonadi::Collection::List ) ),
             this, SLOT( collectionsReceived( Akonadi::Collection::List ) ) );
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( collectionFetchDone( KJob* ) ) );
  }
}

void ResourceBase::retrieveItems( const Akonadi::Collection &collection )
{
  if ( mStore == 0 ) {
    // TODO better message
    cancelTask( i18n( "Resource not configured yet" ) );
  } else {
    ItemFetchJob *job = mStore->fetchItems( collection );
    connect( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ),
             this, SLOT( itemsReceived( Akonadi::Item::List ) ) );
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( itemFetchDone( KJob* ) ) );
  }
}

bool ResourceBase::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  if ( mStore == 0 ) {
    // TODO better message
    cancelTask( i18n( "Resource not configured yet" ) );
  } else {
    ItemFetchJob *job = mStore->fetchItem( item );

    job->fetchScope().fetchAllAttributes( true );
    if ( parts.contains( Akonadi::Item::FullPayload ) ) {
      job->fetchScope().fetchFullPayload( true );
    } else {
      foreach ( const QByteArray &part, parts ) {
        job->fetchScope().fetchPayloadPart( part );
      }
    }
    connect( job, SIGNAL( itemsReceived( Akonadi::Item::List ) ),
             this, SLOT( itemsReceived( Akonadi::Item::List ) ) );
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( itemFetchDone( KJob* ) ) );
  }

  return mStore != 0;
}

void ResourceBase::collectionsReceived( const Akonadi::Collection::List &collections )
{
  collectionsRetrieved( collections );
}

void ResourceBase::collectionFetchDone( KJob * job )
{
  if ( job->error() != 0 ) {
    cancelTask( job->errorText() );
  } else {
    collectionsRetrievalDone();
  }
}

void ResourceBase::itemsReceived( const Akonadi::Item::List &items )
{
  itemsRetrieved( items );
}

void ResourceBase::itemFetchDone( KJob * job )
{
  if ( job->error() != 0 ) {
    cancelTask( job->errorText() );
  } else {
    // check whether this is for retrieveItems() or retrieveItem()
    if ( currentCollection().isValid() ) {
      itemsRetrievalDone();
    } else {
      ItemFetchJob *itemFetch = dynamic_cast<ItemFetchJob*>( job );
      Q_ASSERT( itemFetch != 0 );
      Q_ASSERT( itemFetch->items().count() == 1 );

      itemRetrieved( itemFetch->items().first() );
    }
  }
}

void ResourceBase::itemCreateDone( KJob *job )
{
  if ( job->error() != 0 ) {
    cancelTask( job->errorText() );
  } else {
    ItemCreateJob *itemCreate = dynamic_cast<ItemCreateJob*>( job );
    Q_ASSERT( itemCreate != 0 );

    changeCommitted( itemCreate->item() );
  }
}

void ResourceBase::itemModifyDone( KJob *job )
{
  if ( job->error() != 0 ) {
    cancelTask( job->errorText() );
  } else {
    ItemModifyJob *itemModify = dynamic_cast<ItemModifyJob*>( job );
    Q_ASSERT( itemModify != 0 );

    changeCommitted( itemModify->item() );
  }
}

void ResourceBase::itemDeleteDone( KJob *job )
{
  if ( job->error() != 0 ) {
    cancelTask( job->errorText() );
  } else {
    ItemDeleteJob *itemDelete = dynamic_cast<ItemDeleteJob*>( job );
    Q_ASSERT( itemDelete != 0 );

    changeCommitted( itemDelete->item() );
  }
}

void ResourceBase::compactStore( const QVariant &parameter )
{
  Q_UNUSED( parameter );

  if ( mStore == 0 ) {
    // TODO better message
    cancelTask( i18n( "Resource not configured yet" ) );
  } else {
    StoreCompactJob *job = mStore->compactStore();
    connect( job, SIGNAL( result( KJob* ) ),
             this, SLOT( storeCompactDone( KJob* ) ) );
  }
}

void ResourceBase::storeCompactDone( KJob *job )
{
  if ( job->error() != 0 ) {
    cancelTask( job->errorText() );
  } else {
    taskDone();
  }
}

#include "resourcebase.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
