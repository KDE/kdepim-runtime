/*  This file is part of the KDE project
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

#include "dimapcachecollectionmigrator.h"

#include "kmigratorbase.h"

#include "mixedmaildirstore.h"

#include "filestore/itemfetchjob.h"

#include <kmime/kmime_message.h>

#include <akonadi/agentinstance.h>
#include <akonadi/collection.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/item.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>

#include <KConfigGroup>
#include <KGlobal>
#include <KLocale>
#include <KStandardDirs>

#include <QFileInfo>
#include <QVariant>

using namespace Akonadi;

typedef QHash<QString, QString> UidHash;

class DImapCacheCollectionMigrator::Private
{
  DImapCacheCollectionMigrator *const q;

  public:
    explicit Private( DImapCacheCollectionMigrator *parent )
      : q( parent ), mStore( 0 ), mHiddenSession( 0 ),
        mImportNewMessages( false ), mImportCachedMessages( false ), mRemoveDeletedMessages( false )
    {
    }

    ~Private()
    {
      delete mStore;
      delete mHiddenSession;
    }

    Collection cacheCollection( const Collection &collection ) const;

  public:
    QString mCacheFolder;

    MixedMaildirStore *mStore;

    Session *mHiddenSession;

    Collection mCurrentCollection;
    Item::List mItems;
    UidHash mUidHash;

    bool mImportNewMessages;
    bool mImportCachedMessages;
    bool mRemoveDeletedMessages;

  public: // slots
    void fetchItemsResult( KJob *job );
    void processNextItem();
    void fetchItemResult( KJob *job );
    void itemCreateResult( KJob *job );
};

Collection DImapCacheCollectionMigrator::Private::cacheCollection( const Collection &collection ) const
{
  if ( collection.parentCollection() == Collection::root() ) {
    Collection cache;
    cache.setRemoteId( mCacheFolder );
    cache.setParentCollection( mStore->topLevelCollection() );
    return cache;
  }

  const QString remoteId = collection.remoteId();

  Collection cache;
  cache.setRemoteId( remoteId.mid( 1 ) );
  cache.setParentCollection( cacheCollection( collection.parentCollection() ) );
  return cache;
}

void DImapCacheCollectionMigrator::Private::fetchItemsResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kWarning() << "Store Fetch for item list in collection" << mCurrentCollection.remoteId()
               << "returned error: code=" << job->error() << "text=" << job->errorString();
    mCurrentCollection = Collection();
    q->collectionProcessed();
    return;
  }

  FileStore::ItemFetchJob *fetchJob = qobject_cast<FileStore::ItemFetchJob*>( job );
  Q_ASSERT( fetchJob != 0 );

  const QVariant uidHashVar = fetchJob->property( "remoteIdToIndexUid" );
  if ( !uidHashVar.isValid() ) {
    kDebug() << "No UIDs from index for collection" << mCurrentCollection.name();
    mCurrentCollection = Collection();
    q->collectionProcessed();
    return;
  }

  mUidHash.clear();
  const QHash<QString, QVariant> uidHash = uidHashVar.value< QHash<QString, QVariant> >();
  QHash<QString, QVariant>::const_iterator it    = uidHash.constBegin();
  QHash<QString, QVariant>::const_iterator endIt = uidHash.constEnd();
  for ( ; it != endIt; ++it ) {
    const QString uid = it.value().isValid() ? it.value().value<QString>() : QString();
    mUidHash.insert( it.key(), uid );
  }

  mItems = fetchJob->items();
  kDebug() << mItems.count() << "items for target collection" << mCurrentCollection.remoteId();

  processNextItem();
}

void DImapCacheCollectionMigrator::Private::processNextItem()
{
  kDebug() << "mCurrentCollection=" << mCurrentCollection.name()
           << mItems.count() << "items to go";
  if ( mItems.isEmpty() ) {
    mCurrentCollection = Collection();
    q->collectionProcessed();
    return;
  }

  const Item item = mItems.front();
  mItems.pop_front();

  // TODO should we check for Deleted flags here?

  FileStore::ItemFetchJob *job = mStore->fetchItem( item );
  job->fetchScope().fetchFullPayload( true );
  connect( job, SIGNAL( result( KJob* ) ), q, SLOT( fetchItemResult( KJob* ) ) );
}

void DImapCacheCollectionMigrator::Private::fetchItemResult( KJob *job )
{
  FileStore::ItemFetchJob *fetchJob = qobject_cast<FileStore::ItemFetchJob*>( job );
  Q_ASSERT( fetchJob != 0 );

  Item item = fetchJob->item();
  if ( job->error() != 0 ) {
    kWarning() << "Store Fetch for single item" << item.remoteId() << "returned error: code="
               << job->error() << "text=" << job->errorString();
    processNextItem();
    return;
  }

  const Item::List items = fetchJob->items();
  if ( items.isEmpty() ) {
    kWarning() << "Store Fetch for single item" << item.remoteId() << "returned empty list";
  } else {
    const Item cacheItem = items[ 0 ];
    if ( !cacheItem.hasPayload<KMime::Message::Ptr>() ) {
      kWarning() << "Store Fetch for single item" << item.remoteId() << "returned item without payload";
    } else {
      item.setPayload<KMime::Message::Ptr>( cacheItem.payload<KMime::Message::Ptr>() );
    }
  }

  ItemCreateJob *createJob = 0;

  const QString uid = mUidHash[ item.remoteId() ];
  if ( uid.isEmpty() && mImportNewMessages ) {
    item.setRemoteId( QString() );
    createJob = new ItemCreateJob( item, mCurrentCollection );

    kDebug() << "unsynchronized cacheItem: remoteId=" << item.remoteId()
             << "mimeType=" << item.mimeType()
             << "flags=" << item.flags();
  } else if ( mImportCachedMessages ) {
    item.setRemoteId( uid );
    createJob = new ItemCreateJob( item, mCurrentCollection, mHiddenSession );

    kDebug() << "synchronized cacheItem: remoteId=" << item.remoteId()
             << "mimeType=" << item.mimeType()
             << "flags=" << item.flags();
  }

  if ( createJob != 0 ) {
    connect( createJob, SIGNAL( result( KJob* ) ), q, SLOT( itemCreateResult( KJob* ) ) );
  } else {
    kDebug() << "Skipping cacheItem: remoteId=" << item.remoteId()
             << "mimeType=" << item.mimeType()
             << "flags=" << item.flags();
    processNextItem();
  }
}

void DImapCacheCollectionMigrator::Private::itemCreateResult( KJob *job )
{
  ItemCreateJob *createJob = qobject_cast<ItemCreateJob*>( job );
  Q_ASSERT( createJob != 0 );

  const Item item = createJob->item();

  if ( job->error() != 0 ) {
    kWarning() << "Akonadi Create for single item" << item.remoteId() << "returned error: code="
               << job->error() << "text=" << job->errorString();
  }

  processNextItem();
}

DImapCacheCollectionMigrator::DImapCacheCollectionMigrator( const Akonadi::AgentInstance &resource, QObject *parent )
  : AbstractCollectionMigrator( resource, parent ), d( new Private( this ) )
{
  const KConfigGroup config( KGlobal::config(), QLatin1String( "Disconnected IMAP" ) );
  if ( config.isValid() ) {
    d->mImportNewMessages = config.readEntry( QLatin1String( "ImportNewMessages" ), false );
    d->mImportCachedMessages = config.readEntry( QLatin1String( "ImportCachedMessages" ), false );
    d->mRemoveDeletedMessages = config.readEntry( QLatin1String( "RemoveDeletedMessages" ), false );
  }

  kDebug() << "Migration options: new messages=" << d->mImportNewMessages
           << ", cachedMessages=" << d->mImportCachedMessages
           << ", removeDeleted=" << d->mRemoveDeletedMessages;
}

DImapCacheCollectionMigrator::~DImapCacheCollectionMigrator()
{
  delete d;
}

bool DImapCacheCollectionMigrator::migrationOptionsEnabled() const
{
  return d->mImportNewMessages || d->mImportCachedMessages || d->mRemoveDeletedMessages;
}

void DImapCacheCollectionMigrator::setCacheFolder( const QString &cacheFolder )
{
  d->mCacheFolder = cacheFolder;

  d->mHiddenSession = new Session( resource().identifier().toAscii() );

  const QFileInfo dimapBaseDir( KStandardDirs::locateLocal( "data", QLatin1String( "kmail/dimap" ) ) );
  if ( dimapBaseDir.exists() && dimapBaseDir.isDir() ) {
    d->mStore = new MixedMaildirStore;
    d->mStore->setPath( dimapBaseDir.absoluteFilePath() );
  }
}

void DImapCacheCollectionMigrator::migrateCollection( const Akonadi::Collection &collection )
{
  if ( !migrationOptionsEnabled() ) {
    return;
  }

  // check that we don't get entered while we are still processing
  Q_ASSERT( !d->mCurrentCollection.isValid() );

  if ( d->mStore == 0 ) {
    emit message( KMigratorBase::Skip,
                  i18nc( "@info:status", "No cache for account %1 available",
                         resource().name() ) );
    migrationDone();
    return;
  }

  if ( collection.parentCollection() == Collection::root() ) {
    collectionProcessed();
    return;
  }

  kDebug() << "remoteId=" << collection.remoteId() << ", parent=" << collection.parentCollection().remoteId();

  Collection cache = d->cacheCollection( collection );
  kDebug() << "remoteId=" << cache.remoteId() << ", parent=" << cache.parentCollection().remoteId();

  d->mCurrentCollection = collection;

  emit message( KMigratorBase::Info, i18nc( "@info:status", "Starting cache migration for folder %1 of account %2", collection.name(), resource().name() ) );

  if ( d->mImportNewMessages || d->mImportCachedMessages ) {
    FileStore::ItemFetchJob *job = d->mStore->fetchItems( cache );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( fetchItemsResult( KJob * ) ) );
  } else {
    // TODO when just "RemoveDeleted" is activated
    collectionProcessed();
  }
}

#include "dimapcachecollectionmigrator.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
