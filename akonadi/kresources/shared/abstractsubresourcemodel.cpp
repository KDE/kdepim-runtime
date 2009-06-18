/*
    This file is part of kdepim.
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "abstractsubresourcemodel.h"

#include "concurrentjobs.h"
#include "itemfetchadapter.h"

#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/mimetypechecker.h>
#include <akonadi/monitor.h>

#include <KDebug>
#include <KLocale>

#include <QtCore/QHash>
#include <QtCore/QStringList>

using namespace Akonadi;

class AbstractSubResourceModel::AsyncLoadContext
{
  public:
    explicit AsyncLoadContext( AbstractSubResourceModel *parent )
      : mColFetchJob( 0 ), mResult( true )
    {
      mColFetchJob = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );

      connect( mColFetchJob, SIGNAL( collectionsReceived( Akonadi::Collection::List ) ),
               parent, SLOT( asyncCollectionsReceived( Akonadi::Collection::List ) ) );
      connect( mColFetchJob, SIGNAL( result( KJob* ) ),
               parent, SLOT( asyncCollectionsResult( KJob* ) ) );
    }

    ~AsyncLoadContext()
    {
      delete mColFetchJob;

      qDeleteAll( mItemFetchJobs );
    }

  public:
    CollectionFetchJob *mColFetchJob;

    QSet<ItemFetchAdapter*> mItemFetchJobs;

    bool mResult;
    QString mErrorString;
};

AbstractSubResourceModel::AbstractSubResourceModel( const QStringList &supportedMimeTypes, QObject *parent )
  : QObject( parent ),
    mMonitor( new Monitor( this ) ),
    mMimeChecker( new MimeTypeChecker() ),
    mAsyncLoadContext( 0 )
{
  mMimeChecker->setWantedMimeTypes( supportedMimeTypes );

  mMonitor->blockSignals( true );

  foreach ( const QString &mimeType, supportedMimeTypes ) {
    mMonitor->setMimeTypeMonitored( mimeType );
  }

  mMonitor->setCollectionMonitored( Akonadi::Collection::root() );
  mMonitor->fetchCollection( true );
  mMonitor->itemFetchScope().fetchFullPayload();

  connect( mMonitor, SIGNAL( collectionAdded( Akonadi::Collection, Akonadi::Collection ) ),
          SLOT( monitorCollectionAdded( Akonadi::Collection ) ) );
  connect( mMonitor, SIGNAL( collectionChanged( Akonadi::Collection ) ),
          SLOT( monitorCollectionChanged( Akonadi::Collection ) ) );
  connect( mMonitor, SIGNAL( collectionRemoved( Akonadi::Collection ) ),
          SLOT( monitorCollectionRemoved( Akonadi::Collection ) ) );

  connect( mMonitor, SIGNAL( itemAdded( Akonadi::Item, Akonadi::Collection ) ),
          SLOT( monitorItemAdded( Akonadi::Item, Akonadi::Collection ) ) );
  connect( mMonitor, SIGNAL( itemChanged( Akonadi::Item, QSet<QByteArray> ) ),
          SLOT( monitorItemChanged( Akonadi::Item ) ) );
  connect( mMonitor, SIGNAL( itemRemoved( Akonadi::Item ) ),
          SLOT( monitorItemRemoved( Akonadi::Item ) ) );
}

AbstractSubResourceModel::~AbstractSubResourceModel()
{
  delete mAsyncLoadContext;
  delete mMimeChecker;
}

QStringList AbstractSubResourceModel::subResourceIdentifiers() const
{
  return mSubResourceIdentifiers.toList();
}

void AbstractSubResourceModel::startMonitoring()
{
  mMonitor->blockSignals( false );
}

void AbstractSubResourceModel::stopMonitoring()
{
  mMonitor->blockSignals( true );
}

void AbstractSubResourceModel::clear()
{
  clearModel();

  mSubResourceIdentifiers.clear();
}

bool AbstractSubResourceModel::load()
{
  ConcurrentCollectionFetchJob colFetchJob;

  if ( !colFetchJob.exec() ) {
    kError( 5650 ) << "Loading collections failed:" << colFetchJob->errorString();
    emit loadingResult( false, colFetchJob->errorString() );
    return false;
  }

  const Collection::List collections = colFetchJob->collections();
  foreach ( const Collection &collection, collections ) {
    if ( mMimeChecker->isWantedCollection( collection ) ) {
      collectionAdded( collection );
      mMonitor->setCollectionMonitored( collection );
      ConcurrentItemFetchJob itemFetchJob( collection );
      if ( !itemFetchJob.exec() ) {
        kError( 5650 ) << "Loading items for collection (id=" << collection.id()
                       << ", remoteId=" << collection.remoteId()
                       << "failed:" << itemFetchJob->errorString();
        emit loadingResult( false, itemFetchJob->errorString() );
        return false;
      }

      const Item::List items = itemFetchJob->items();
      foreach ( const Item &item, items ) {
        if ( mMimeChecker->isWantedItem( item ) ) {
          itemAdded( item, collection );
        }
      }
    }
  }

  emit loadingResult( true, QString() );
  return true;
}

bool AbstractSubResourceModel::asyncLoad()
{
  if ( mAsyncLoadContext != 0 ) {
    const QString message = i18nc( "@info:status", "Loading already in progress" );
    emit loadingResult( false, message );
    return false;
  }

  mAsyncLoadContext = new AsyncLoadContext( this );

  return true;
}

void AbstractSubResourceModel::monitorCollectionAdded( const Akonadi::Collection &collection )
{
  // TODO check whether we need to do something while an async load is in progress
  if ( mMimeChecker->isWantedCollection( collection ) ) {
    collectionAdded( collection );
  }
}

void AbstractSubResourceModel::monitorCollectionChanged( const Akonadi::Collection &collection )
{
  // TODO check whether we need to do something while an async load is in progress
  if ( mMimeChecker->isWantedCollection( collection ) ) {
    collectionChanged( collection );
  }
}

void AbstractSubResourceModel::monitorCollectionRemoved( const Akonadi::Collection &collection )
{
  // TODO check whether we need to do something while an async load is in progress
  mMonitor->setCollectionMonitored( collection, false );
  collectionRemoved( collection );
}

void AbstractSubResourceModel::monitorItemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  // TODO check whether we need to do something while an async load is in progress
  if ( mMimeChecker->isWantedItem( item ) ) {
    itemAdded( item, collection );
  }
}

void AbstractSubResourceModel::monitorItemChanged( const Akonadi::Item &item )
{
  // TODO check whether we need to do something while an async load is in progress
  if ( mMimeChecker->isWantedItem( item ) ) {
    itemChanged( item );
  }
}

void AbstractSubResourceModel::monitorItemRemoved( const Akonadi::Item &item )
{
  // TODO check whether we need to do something while an async load is in progress
  if ( mMimeChecker->isWantedItem( item ) ) {
    itemRemoved( item );
  }
}

void AbstractSubResourceModel::asyncCollectionsReceived( const Akonadi::Collection::List &collections )
{
  AsyncLoadContext *context = mAsyncLoadContext;
  if ( context == 0 ) {
    return;
  }

  foreach ( const Collection &collection, collections ) {
    if ( mMimeChecker->isWantedCollection( collection ) ) {
      collectionAdded( collection );
      mMonitor->setCollectionMonitored( collection );

      context->mItemFetchJobs.insert( new ItemFetchAdapter( collection, this ) );
    }
  }
}

void AbstractSubResourceModel::asyncCollectionsResult( KJob *job )
{
  AsyncLoadContext *context = mAsyncLoadContext;
  if ( context == 0 ) {
    return;
  }

  Q_ASSERT( job == context->mColFetchJob );

  context->mColFetchJob = 0;

  if ( job->error() != 0 ) {
    mAsyncLoadContext = 0;

    kError( 5650 ) << "Loading collections failed:" << job->errorString();
    emit loadingResult( false, job->errorString() );

    delete context;
    return;
  }

  if ( context->mItemFetchJobs.isEmpty() ) {
    mAsyncLoadContext = 0;

    emit loadingResult( true, QString() );

    delete context;
  }
}

void AbstractSubResourceModel::asyncItemsReceived( const Akonadi::Collection &collection, const Akonadi::Item::List &items )
{
  foreach ( const Item &item, items ) {
    if ( mMimeChecker->isWantedItem( item ) ) {
      itemAdded( item, collection );
    }
  }
}

void AbstractSubResourceModel::asyncItemsResult( ItemFetchAdapter *fetcher, KJob *job )
{
  AsyncLoadContext *context = mAsyncLoadContext;
  if ( context == 0 ) {
    return;
  }

  context->mItemFetchJobs.remove( fetcher );

  if ( job->error() != 0 ) {
    mAsyncLoadContext = 0;

    const Collection collection = fetcher->collection();
    kError( 5650 ) << "Loading items for collection (id=" << collection.id()
                   << ", remoteId=" << collection.remoteId()
                   << "failed:" << job->errorString();
    emit loadingResult( false, job->errorString() );

    delete context;
    return;
  }

  if ( context->mColFetchJob == 0 && mAsyncLoadContext->mItemFetchJobs.isEmpty() ) {
    mAsyncLoadContext = 0;

    emit loadingResult( true, QString() );

    delete context;
  }
}

#include "abstractsubresourcemodel.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
