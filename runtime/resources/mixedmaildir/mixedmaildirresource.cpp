/*  This file is part of the KDE project
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

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

#include "mixedmaildirresource.h"

#include "compactchangehelper.h"
#include "configdialog.h"
#include "mixedmaildirstore.h"
#include "settings.h"
#include "settingsadaptor.h"

#include "kmindexreader/messagestatus.h"

#include "filestore/collectioncreatejob.h"
#include "filestore/collectiondeletejob.h"
#include "filestore/collectionfetchjob.h"
#include "filestore/collectionmodifyjob.h"
#include "filestore/collectionmovejob.h"
#include "filestore/entitycompactchangeattribute.h"
#include "filestore/itemcreatejob.h"
#include "filestore/itemdeletejob.h"
#include "filestore/itemfetchjob.h"
#include "filestore/itemmodifyjob.h"
#include "filestore/itemmovejob.h"
#include "filestore/storecompactjob.h"

#include <akonadi/kmime/messageparts.h>
#include <akonadi/changerecorder.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/collectionfetchscope.h>

#include <kmime/kmime_message.h>

#include <Nepomuk/Tag>
#include <Nepomuk/Resource>

#include <KDebug>
#include <KLocale>
#include <KWindowSystem>

#include <QtCore/QDir>
#include <QtDBus/QDBusConnection>

using namespace Akonadi;

MixedMaildirResource::MixedMaildirResource( const QString &id )
    : ResourceBase( id ), mStore( new MixedMaildirStore() ), mCompactHelper( 0 )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                              Settings::self(), QDBusConnection::ExportAdaptors );
  connect( this, SIGNAL( reloadConfiguration() ), SLOT( reapplyConfiguration() ) );

  // We need to enable this here, otherwise we neither get the remote ID of the
  // parent collection when a collection changes, nor the full item when an item
  // is added.
  changeRecorder()->fetchCollection( true );
  changeRecorder()->itemFetchScope().fetchFullPayload( true );
  changeRecorder()->itemFetchScope().setAncestorRetrieval( ItemFetchScope::All );
  changeRecorder()->collectionFetchScope().setAncestorRetrieval( CollectionFetchScope::All );

  setHierarchicalRemoteIdentifiersEnabled( true );

  if ( ensureSaneConfiguration() ) {
    const bool changeName = name().isEmpty() || name() == identifier() ||
                            name() == mStore->topLevelCollection().name();
    mStore->setPath( Settings::self()->path() );
    if ( changeName ) {
      setName( mStore->topLevelCollection().name() );
    }
  }

  const QByteArray compactHelperSessionId = id.toUtf8() + "-compacthelper";
  mCompactHelper = new CompactChangeHelper( compactHelperSessionId, this );
}

MixedMaildirResource::~MixedMaildirResource()
{
  delete mStore;
}

void MixedMaildirResource::aboutToQuit()
{
  // The settings may not have been saved if e.g. they have been modified via
  // DBus instead of the config dialog.
  Settings::self()->writeConfig();
}

void MixedMaildirResource::configure( WId windowId )
{
  ConfigDialog dlg;
  if ( windowId ) {
    KWindowSystem::setMainWindow( &dlg, windowId );
  }

  bool fullSync = false;

  if ( dlg.exec() ) {
    const bool changeName = name().isEmpty() || name() == identifier() ||
                            name() == mStore->topLevelCollection().name();

    const QString oldPath = mStore->path();
    mStore->setPath( Settings::self()->path() );

    fullSync = oldPath != mStore->path();

    if ( changeName ) {
      setName( mStore->topLevelCollection().name() );
    }
    emit configurationDialogAccepted();
  } else {
    emit configurationDialogRejected();
  }

  if ( ensureDirExists() ) {
    if ( fullSync ) {
      mSynchronizedCollections.clear();
      mPendingSynchronizeCollections.clear();
    }
    synchronizeCollectionTree();
  }
}

void MixedMaildirResource::itemAdded( const Item &item, const Collection& collection )
{
/*  kDebug() << "item.id=" << item.id() << "col=" << collection.remoteId();*/
  if ( !ensureSaneConfiguration() ) {
    const QString message = i18nc( "@info:status", "Unusable configuration." );
    kError() << message;
    cancelTask( message );
    return;
  }

  FileStore::ItemCreateJob *job = mStore->createItem( item, collection );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( itemAddedResult( KJob* ) ) );
}

void MixedMaildirResource::itemChanged( const Item &item, const QSet<QByteArray>& parts )
{
/*  kDebug() << "item.id=" << item.id() << "col=" << item.parentCollection().remoteId()
           << "parts=" << parts;*/
  if ( !ensureSaneConfiguration() ) {
    const QString message = i18nc( "@info:status", "Unusable configuration." );
    kError() << message;
    cancelTask( message );
    return;
  }

  if ( Settings::self()->readOnly() ) {
    changeProcessed();
    return;
  }

  // TODO this is probably something the store should decide
  const bool payloadChanged = parts.contains( Item::FullPayload ) ||
                              parts.contains( MessagePart::Header ) ||
                              parts.contains( MessagePart::Envelope ) ||
                              parts.contains( MessagePart::Body );

  Item storeItem( item );
  storeItem.setRemoteId( mCompactHelper->currentRemoteId( item ) );

  FileStore::ItemModifyJob *job = mStore->modifyItem( storeItem );
  job->setIgnorePayload( !payloadChanged || !item.hasPayload<KMime::Message::Ptr>() );
  job->setProperty( "originalRemoteId", storeItem.remoteId() );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( itemChangedResult( KJob* ) ) );
}

void MixedMaildirResource::itemMoved( const Item &item, const Collection &source, const Collection &destination )
{
/*  kDebug() << "item.id=" << item.id() << "remoteId=" << item.remoteId()
           << "source=" << source.remoteId() << "dest=" << destination.remoteId();*/
  if ( source == destination ) {
    changeProcessed();
    return;
  }

  if ( !ensureSaneConfiguration() ) {
    const QString message = i18nc( "@info:status", "Unusable configuration." );
    kError() << message;
    cancelTask( message );
    return;
  }

  Item moveItem = item;
  moveItem.setRemoteId( mCompactHelper->currentRemoteId( item ) );
  moveItem.setParentCollection( source );

  FileStore::ItemMoveJob *job = mStore->moveItem( moveItem, destination );
  job->setProperty( "originalRemoteId", moveItem.remoteId() );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( itemMovedResult( KJob* ) ) );
}

void MixedMaildirResource::itemRemoved(const Item &item)
{
/*  kDebug() << "item.id=" << item.id() << "col=" << collection.remoteId()
           << "collection.remoteRevision=" << item.parentCollection().remoteRevision();*/
  if ( !ensureSaneConfiguration() ) {
    const QString message = i18nc( "@info:status", "Unusable configuration." );
    kError() << message;
    cancelTask( message );
    return;
  }

  Item storeItem( item );
  storeItem.setRemoteId( mCompactHelper->currentRemoteId( item ) );
  FileStore::ItemDeleteJob *job = mStore->deleteItem( storeItem );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( itemRemovedResult( KJob* ) ) );
}

void MixedMaildirResource::retrieveCollections()
{
  if ( !ensureSaneConfiguration() ) {
    const QString message = i18nc( "@info:status", "Unusable configuration." );
    kError() << message;
    cancelTask( message );
    return;
  }

  FileStore::CollectionFetchJob *job = mStore->fetchCollections( mStore->topLevelCollection(), FileStore::CollectionFetchJob::Recursive );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( retrieveCollectionsResult( KJob* ) ) );

  status( Running, i18nc( "@info:status", "Synchronizing email folders" ) );
}

void MixedMaildirResource::retrieveItems( const Collection & col )
{
  if ( !ensureSaneConfiguration() ) {
    const QString message = i18nc( "@info:status", "Unusable configuration." );
    kError() << message;
    cancelTask( message );
    return;
  }

  FileStore::ItemFetchJob *job = mStore->fetchItems( col );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( retrieveItemsResult( KJob* ) ) );

  status( Running, i18nc( "@info:status", "Synchronizing email folder %1", col.name() ) );
}

bool MixedMaildirResource::retrieveItem( const Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );

  FileStore::ItemFetchJob *job = mStore->fetchItem( item );
  if ( parts.contains( Item::FullPayload ) ) {
    job->fetchScope().fetchFullPayload( true );
  } else {
    Q_FOREACH( const QByteArray &part, parts ) {
      job->fetchScope().fetchPayloadPart( part, true );
    }
  }
  connect( job, SIGNAL( result( KJob* ) ), SLOT( retrieveItemResult( KJob* ) ) );

  return true;
}

void MixedMaildirResource::collectionAdded(const Collection &collection, const Collection &parent)
{
  if ( !ensureSaneConfiguration() ) {
    const QString message = i18nc( "@info:status", "Unusable configuration." );
    kError() << message;
    cancelTask( message );
    return;
  }

  FileStore::CollectionCreateJob *job = mStore->createCollection( collection, parent );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( collectionAddedResult( KJob* ) ) );
}

void MixedMaildirResource::collectionChanged(const Collection &collection)
{
  if ( !ensureSaneConfiguration() ) {
    const QString message = i18nc( "@info:status", "Unusable configuration." );
    kError() << message;
    cancelTask( message );
    return;
  }

  mCompactHelper->checkCollectionChanged( collection );

  FileStore::CollectionModifyJob *job = mStore->modifyCollection( collection );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( collectionChangedResult( KJob* ) ) );
}

void MixedMaildirResource::collectionChanged(const Collection &collection, const QSet<QByteArray> &changedAttributes )
{
  if ( !ensureSaneConfiguration() ) {
    const QString message = i18nc( "@info:status", "Unusable configuration." );
    kError() << message;
    cancelTask( message );
    return;
  }

  mCompactHelper->checkCollectionChanged( collection );

  Q_UNUSED( changedAttributes );

  FileStore::CollectionModifyJob *job = mStore->modifyCollection( collection );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( collectionChangedResult( KJob* ) ) );
}

void MixedMaildirResource::collectionMoved( const Collection &collection, const Collection &source, const Collection &dest )
{
  //kDebug( KDE_DEFAULT_DEBUG_AREA ) << collection << source << dest;

  if ( !ensureSaneConfiguration() ) {
    const QString message = i18nc( "@info:status", "Unusable configuration." );
    kError() << message;
    cancelTask( message );
    return;
  }

  if ( collection.parentCollection() == Collection::root() ) {
    const QString message = i18nc( "@info:status", "Cannot move root maildir folder '%1'." ,collection.remoteId() );
    kError() << message;
    cancelTask( message );
    return;
  }

  if ( source == dest ) { // should not happen, but who knows...
    changeProcessed();
    return;
  }

  Collection moveCollection = collection;
  moveCollection.setParentCollection( source );

  FileStore::CollectionMoveJob *job = mStore->moveCollection( moveCollection, dest );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( collectionMovedResult( KJob* ) ) );
}

void MixedMaildirResource::collectionRemoved( const Collection &collection )
{
   if ( !ensureSaneConfiguration() ) {
    const QString message = i18nc( "@info:status", "Unusable configuration." );
    kError() << message;
    cancelTask( message );
    return;
  }

  if ( collection.parentCollection() == Collection::root() ) {
    emit error( i18n("Cannot delete top-level maildir folder '%1'.", Settings::self()->path() ) );
    changeProcessed();
    return;
  }

  FileStore::CollectionDeleteJob *job = mStore->deleteCollection( collection );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( collectionRemovedResult( KJob* ) ) );
}

bool MixedMaildirResource::ensureDirExists()
{
  QDir dir( Settings::self()->path() );
  if ( !dir.exists() ) {
    if ( !dir.mkpath( Settings::self()->path() ) ) {
      const QString message = i18nc( "@info:status", "Unable to create maildir '%1'.", Settings::self()->path() );
      kError() << message;
      status( Broken, message );
      return false;
    }
  }
  return true;
}

bool MixedMaildirResource::ensureSaneConfiguration()
{
  if ( Settings::self()->path().isEmpty() ) {
    const QString message = i18nc( "@info:status", "No usable storage location configured.");
    kError() << message;
    status( Broken, message );
    return false;
  }
  return true;
}

void MixedMaildirResource::checkForInvalidatedIndexCollections( KJob * job )
{
  // when operations invalidate the on-disk index, we need to make sure all index
  // data has been transferred into Akonadi by synchronizing the collections
  const QVariant var = job->property( "onDiskIndexInvalidated" );
  if ( var.isValid() ) {
    const Collection::List collections = var.value<Collection::List>();
    kDebug( KDE_DEFAULT_DEBUG_AREA ) << "On disk index of" << collections.count()
      << "collections invalidated after" << job->metaObject()->className();

    Q_FOREACH( const Collection &collection, collections ) {
      const Collection::Id id = collection.id();
      if ( !mSynchronizedCollections.contains( id ) && !mPendingSynchronizeCollections.contains( id ) ) {
        kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Requesting sync of collection" << collection.name()
          << ", id=" << collection.id();
        mPendingSynchronizeCollections << id;
        synchronizeCollection( id );
      }
    }
  }
}

void MixedMaildirResource::reapplyConfiguration()
{
  if ( ensureSaneConfiguration() && ensureDirExists() ) {
    const QString oldPath = mStore->path();
    mStore->setPath( Settings::self()->path() );

    if ( oldPath != mStore->path() ) {
      mSynchronizedCollections.clear();
      mPendingSynchronizeCollections.clear();
    }
    synchronizeCollectionTree();
  }
}

void MixedMaildirResource::retrieveCollectionsResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kError() << job->errorString();
    status( Broken, job->errorString() );
    cancelTask( job->errorString() );
    return;
  }

  FileStore::CollectionFetchJob *fetchJob = qobject_cast<FileStore::CollectionFetchJob*>( job );
  Q_ASSERT( fetchJob != 0 );

  Collection topLevelCollection = mStore->topLevelCollection();
  if ( !name().isEmpty() && name() != identifier() ) {
    topLevelCollection.setName( name() );
  }

  Collection::List collections;
  collections << topLevelCollection;
  collections << fetchJob->collections();
  collectionsRetrieved( collections );
}

void MixedMaildirResource::retrieveItemsResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kError() << job->errorString();
    status( Broken, job->errorString() );
    cancelTask( job->errorString() );
    return;
  }

  FileStore::ItemFetchJob *fetchJob = qobject_cast<FileStore::ItemFetchJob*>( job );
  Q_ASSERT( fetchJob != 0 );

  // messages marked as deleted have been deleted from mbox files but never got purged
  // TODO FileStore could provide deleteItems() to deleted all filtered items in one go
  KJob* deleteJob = 0;
  Item::List items;
  Q_FOREACH( const Item &item, fetchJob->items() ) {
    KPIM::MessageStatus status;
    status.setStatusFromFlags( item.flags() );
    if ( status.isDeleted() ) {
      deleteJob = mStore->deleteItem( item );
    } else {
      items << item;
    }
  }

  if ( deleteJob != 0 ) {
    kDebug( KDE_DEFAULT_DEBUG_AREA ) << items.count() << "of" << fetchJob->items().count()
      << "items remain after checking for items marked as Deleted";
    // last item delete triggers mbox purge, i.e. store compact
    Q_ASSERT( connect( deleteJob, SIGNAL( result( KJob* ) ), this, SLOT( itemsDeleted( KJob* ) ) ) );
  }

  // if some items have tags, we need to complete the retrieval and schedule tagging
  // to a later time so we can then fetch the items to get their Akonadi URLs
  const QVariant var = fetchJob->property( "remoteIdToTagList" );
  if ( var.isValid() ) {
    const QHash<QString, QVariant> tagListHash = var.value< QHash<QString, QVariant> >();
    if ( !tagListHash.isEmpty() ) {
      kDebug() << tagListHash.count() << "of" << items.count()
               << "items in collection" << fetchJob->collection().remoteId() << "have tags";

      TagContextList taggedItems;
      Q_FOREACH( const Item &item, items ) {
        const QVariant tagListVar = tagListHash[ item.remoteId() ];
        if ( tagListVar.isValid() ) {
          const QStringList tagList = tagListVar.value<QStringList>();
          if ( !tagListHash.isEmpty() ) {
            TagContext tag;
            tag.mItem = item;
            tag.mTagList = tagList;

            taggedItems << tag;
          }
        }
      }

      if ( !taggedItems.isEmpty() ) {
        mTagContextByColId.insert( fetchJob->collection().id(), taggedItems );

        scheduleCustomTask( this, "restoreTags", QVariant::fromValue<Collection>( fetchJob->collection() ) );
      }
    }
  }

  mSynchronizedCollections << fetchJob->collection().id();
  mPendingSynchronizeCollections.remove( fetchJob->collection().id() );

  itemsRetrieved( items );
}

void MixedMaildirResource::retrieveItemResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kError() << job->errorString();
    status( Broken, job->errorString() );
    cancelTask( job->errorString() );
    return;
  }

  FileStore::ItemFetchJob *fetchJob = qobject_cast<FileStore::ItemFetchJob*>( job );
  Q_ASSERT( fetchJob != 0 );
  Q_ASSERT( !fetchJob->items().isEmpty() );

  itemRetrieved( fetchJob->items()[ 0 ] );
}

void MixedMaildirResource::itemAddedResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kError() << job->errorString();
    status( Broken, job->errorString() );
    cancelTask( job->errorString() );
    return;
  }

  FileStore::ItemCreateJob *itemJob = qobject_cast<FileStore::ItemCreateJob*>( job );
  Q_ASSERT( itemJob != 0 );

/*  kDebug() << "item.id=" << itemJob->item().id() << "remoteId=" << itemJob->item().remoteId();*/
  changeCommitted( itemJob->item() );

  checkForInvalidatedIndexCollections( job );
}

void MixedMaildirResource::itemChangedResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kError() << job->errorString();
    status( Broken, job->errorString() );
    cancelTask( job->errorString() );
    return;
  }

  FileStore::ItemModifyJob *itemJob = qobject_cast<FileStore::ItemModifyJob*>( job );
  Q_ASSERT( itemJob != 0 );

  changeCommitted( itemJob->item() );

  const QString remoteId = itemJob->property( "originalRemoteId" ).value<QString>();

  const QVariant compactStoreVar = itemJob->property( "compactStore" );
  if ( compactStoreVar.isValid() && compactStoreVar.toBool() ) {
    scheduleCustomTask( this, "compactStore", QVariant() );
  }

  checkForInvalidatedIndexCollections( job );
}

void MixedMaildirResource::itemMovedResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kError() << job->errorString();
    status( Broken, job->errorString() );
    cancelTask( job->errorString() );
    return;
  }

  FileStore::ItemMoveJob *itemJob = qobject_cast<FileStore::ItemMoveJob*>( job );
  Q_ASSERT( itemJob != 0 );

  changeCommitted( itemJob->item() );

  const QString remoteId = itemJob->property( "originalRemoteId" ).value<QString>();
//   kDebug() << "item.id=" << itemJob->item().id() << "remoteId=" << itemJob->item().remoteId()
//            << "old remoteId=" << remoteId;

  const QVariant compactStoreVar = itemJob->property( "compactStore" );
  if ( compactStoreVar.isValid() && compactStoreVar.toBool() ) {
    scheduleCustomTask( this, "compactStore", QVariant() );
  }

  checkForInvalidatedIndexCollections( job );
}

void MixedMaildirResource::itemRemovedResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kError() << job->errorString();
    status( Broken, job->errorString() );
    cancelTask( job->errorString() );
    return;
  }

  FileStore::ItemDeleteJob *itemJob = qobject_cast<FileStore::ItemDeleteJob*>( job );
  Q_ASSERT( itemJob != 0 );

  changeCommitted( itemJob->item() );

  const QString remoteId = itemJob->item().remoteId();

  const QVariant compactStoreVar = itemJob->property( "compactStore" );
  if ( compactStoreVar.isValid() && compactStoreVar.toBool() ) {
    scheduleCustomTask( this, "compactStore", QVariant() );
  }

  checkForInvalidatedIndexCollections( job );
}

void MixedMaildirResource::itemsDeleted( KJob *job )
{
  Q_UNUSED( job );
  scheduleCustomTask( this, "compactStore", QVariant() );
}

void MixedMaildirResource::collectionAddedResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kError() << job->errorString();
    status( Broken, job->errorString() );
    cancelTask( job->errorString() );
    return;
  }

  FileStore::CollectionCreateJob *colJob = qobject_cast<FileStore::CollectionCreateJob*>( job );
  Q_ASSERT( colJob != 0 );

  changeCommitted( colJob->collection() );
}

void MixedMaildirResource::collectionChangedResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kError() << job->errorString();
    status( Broken, job->errorString() );
    cancelTask( job->errorString() );
    return;
  }

  FileStore::CollectionModifyJob *colJob = qobject_cast<FileStore::CollectionModifyJob*>( job );
  Q_ASSERT( colJob != 0 );

  changeCommitted( colJob->collection() );

  checkForInvalidatedIndexCollections( job );
}

void MixedMaildirResource::collectionMovedResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kError() << job->errorString();
    status( Broken, job->errorString() );
    cancelTask( job->errorString() );
    return;
  }

  FileStore::CollectionMoveJob *colJob = qobject_cast<FileStore::CollectionMoveJob*>( job );
  Q_ASSERT( colJob != 0 );

  changeCommitted( colJob->collection() );

  checkForInvalidatedIndexCollections( job );
}

void MixedMaildirResource::collectionRemovedResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kError() << job->errorString();
    status( Broken, job->errorString() );
    cancelTask( job->errorString() );
    return;
  }

  FileStore::CollectionDeleteJob *colJob = qobject_cast<FileStore::CollectionDeleteJob*>( job );
  Q_ASSERT( colJob != 0 );

  changeCommitted( colJob->collection() );
}

void MixedMaildirResource::compactStore( const QVariant &arg )
{
  Q_UNUSED( arg );

  FileStore::StoreCompactJob *job = mStore->compactStore();
  connect( job, SIGNAL( result( KJob* ) ), SLOT( compactStoreResult( KJob* ) ) );
}

void MixedMaildirResource::compactStoreResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kError() << job->errorString();
    status( Broken, job->errorString() );
    cancelTask( job->errorString() );
    return;
  }

  FileStore::StoreCompactJob *compactJob = qobject_cast<FileStore::StoreCompactJob*>( job );
  Q_ASSERT( compactJob != 0 );

  const Item::List items = compactJob->changedItems();
  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Compacting store resulted in" << items.count() << "changed items";

  mCompactHelper->addChangedItems( items );

  taskDone();

  checkForInvalidatedIndexCollections( job );
}

void MixedMaildirResource::restoreTags( const QVariant &arg )
{
  if ( !arg.isValid() ) {
    kError() << "Given variant is not valid";
    cancelTask();
    return;
  }

  const Collection collection = arg.value<Collection>();
  if ( !collection.isValid() ) {
    kError() << "Given variant is not valid";
    cancelTask();
    return;
  }

  const TagContextList taggedItems = mTagContextByColId[ collection.id() ];
  mPendingTagContexts << taggedItems;

  QMetaObject::invokeMethod( this, "processNextTagContext", Qt::QueuedConnection );
  taskDone();
}

void MixedMaildirResource::processNextTagContext()
{
  kDebug() << mPendingTagContexts.count() << "items to go";
  if ( mPendingTagContexts.isEmpty() ) {
    return;
  }

  const TagContext tagContext = mPendingTagContexts.front();
  mPendingTagContexts.pop_front();

  ItemFetchJob *fetchJob = new ItemFetchJob( tagContext.mItem );
  fetchJob->setProperty( "tagList", tagContext.mTagList );
  connect( fetchJob, SIGNAL( result( KJob* ) ), SLOT( tagFetchJobResult( KJob* ) ) );
}

void MixedMaildirResource::tagFetchJobResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kError() << job->errorString();
    processNextTagContext();
    return;
  }

  ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>( job );
  Q_ASSERT( fetchJob != 0 );

  Q_ASSERT( !fetchJob->items().isEmpty() );

  const Item item = fetchJob->items()[ 0 ];
  const QStringList tagList = job->property( "tagList" ).value<QStringList>();
  kDebug() << "Tagging item" << item.url() << "with" << tagList;

  QList<Nepomuk::Tag> nepomukTags;
  Q_FOREACH( const QString &tag, tagList ) {
    if ( tag.isEmpty() ) {
      kWarning() << "TagList for item" << item.url() << "contains an empty tag";
    } else {
      nepomukTags << Nepomuk::Tag( tag );
    }
  }

  Nepomuk::Resource nepomukResource( item.url() );
  nepomukResource.setTags( nepomukTags );

  processNextTagContext();
}

#include "mixedmaildirresource.moc"

AKONADI_RESOURCE_MAIN( MixedMaildirResource )

// kate: space-indent on; indent-width 2; replace-tabs on;
