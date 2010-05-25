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

#include "libmaildir/maildir.h"
#include "mixedmaildirstore.h"

#include "filestore/itemfetchjob.h"

#include <kmime/kmime_message.h>

#include <akonadi/agentinstance.h>
#include <akonadi/collection.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/item.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>

#include <Nepomuk/Tag>

#include <KConfigGroup>
#include <KGlobal>
#include <KLocale>
#include <KStandardDirs>

#include <QFileInfo>
#include <QVariant>

using namespace Akonadi;
using KPIM::Maildir;

typedef QHash<QString, QString> UidHash;

class DImapCacheCollectionMigrator::Private
{
  DImapCacheCollectionMigrator *const q;

  public:
    explicit Private( DImapCacheCollectionMigrator *parent )
      : q( parent ), mStore( 0 ), mHiddenSession( 0 ),
        mImportNewMessages( false ), mImportCachedMessages( false ), mRemoveDeletedMessages( false ),
        mKMailConfig( 0 )
    {
    }

    ~Private()
    {
      delete mStore;
      delete mHiddenSession;
    }

    Collection cacheCollection( const Collection &collection, QString &cachePath ) const;

  public:
    QString mCacheFolder;

    MixedMaildirStore *mStore;

    Session *mHiddenSession;

    Collection mCurrentCollection;
    Item::List mItems;
    UidHash mUidHash;
    QStringList mDeletedUids;

    bool mImportNewMessages;
    bool mImportCachedMessages;
    bool mRemoveDeletedMessages;

    KConfig *mKMailConfig;
    KConfigGroup mCurrentFolderGroup;

    QHash<QString, QVariant> mTagListHash;

  public: // slots
    void fetchItemsResult( KJob *job );
    void processNextItem();
    void processNextDeletedUid();
    void fetchItemResult( KJob *job );
    void itemCreateResult( KJob *job );
    void itemDeletePhase1Result( KJob *job );
    void itemDeletePhase2Result( KJob *job );
};

Collection DImapCacheCollectionMigrator::Private::cacheCollection( const Collection &collection, QString &cachePath ) const
{
  if ( collection.parentCollection() == Collection::root() ) {
    Collection cache;
    cache.setRemoteId( mCacheFolder );
    cache.setParentCollection( mStore->topLevelCollection() );
    cachePath = Maildir::subDirNameForFolderName( mCacheFolder ) + QLatin1Char( '/' ) + cachePath;
    return cache;
  }

  const QString remoteId = collection.remoteId().mid( 1 );

  if ( cachePath.isEmpty() ) {
    cachePath = remoteId;
  } else {
    cachePath = Maildir::subDirNameForFolderName( remoteId ) + QLatin1Char( '/' ) + cachePath;
  }

  Collection cache;
  cache.setRemoteId( remoteId );
  cache.setParentCollection( cacheCollection( collection.parentCollection(), cachePath ) );
  return cache;
}

void DImapCacheCollectionMigrator::Private::fetchItemsResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kWarning() << "Store Fetch for item list in collection" << mCurrentCollection.remoteId()
               << "returned error: code=" << job->error() << "text=" << job->errorString();
    if ( mRemoveDeletedMessages ) {
      processNextDeletedUid();
    } else {
      mCurrentCollection = Collection();
      q->collectionProcessed();
    }
    return;
  }

  FileStore::ItemFetchJob *fetchJob = qobject_cast<FileStore::ItemFetchJob*>( job );
  Q_ASSERT( fetchJob != 0 );

  const QVariant uidHashVar = fetchJob->property( "remoteIdToIndexUid" );
  if ( !uidHashVar.isValid() ) {
    kDebug() << "No UIDs from index for collection" << mCurrentCollection.name();
    if ( mRemoveDeletedMessages ) {
      processNextDeletedUid();
    } else {
      mCurrentCollection = Collection();
      q->collectionProcessed();
    }
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

  const QVariant tagListHashVar = fetchJob->property( "remoteIdToTagList" );
  if ( !tagListHashVar.isValid() ) {
    kDebug() << "No tags from index for collection" << mCurrentCollection.name();
  } else {
    mTagListHash = tagListHashVar.value< QHash<QString, QVariant> >();
    kDebug() << mTagListHash.count() << "items have tags";
  }

  processNextItem();
}

void DImapCacheCollectionMigrator::Private::processNextItem()
{
  kDebug() << "mCurrentCollection=" << mCurrentCollection.name()
           << mItems.count() << "items to go";
  if ( mItems.isEmpty() ) {
    if ( mDeletedUids.isEmpty() ) {
      mCurrentCollection = Collection();
      q->collectionProcessed();
    } else if ( mRemoveDeletedMessages ) {
      processNextDeletedUid();
    }
    return;
  }

  const Item item = mItems.front();
  mItems.pop_front();

  // TODO should we check for Deleted flags here?

  FileStore::ItemFetchJob *job = mStore->fetchItem( item );
  job->fetchScope().fetchFullPayload( true );
  connect( job, SIGNAL( result( KJob* ) ), q, SLOT( fetchItemResult( KJob* ) ) );
}

void DImapCacheCollectionMigrator::Private::processNextDeletedUid()
{
  kDebug() << "mCurrentCollection=" << mCurrentCollection.name()
           << mDeletedUids.count() << "items to go";

  if ( mDeletedUids.isEmpty() ) {
    if ( mCurrentFolderGroup.isValid() ) {
      const QString key = QLatin1String( "UIDSDeletedSinceLastSync" );
      if ( !mCurrentFolderGroup.readEntry( key, QStringList() ).isEmpty() ) {
        mCurrentFolderGroup.deleteEntry( QLatin1String( "UIDSDeletedSinceLastSync" ) );
        mCurrentFolderGroup.sync();
      }
    }
    mCurrentFolderGroup = KConfigGroup();
    mCurrentCollection = Collection();
    q->collectionProcessed();
    return;
  }

  const QString uid = mDeletedUids.front();
  mDeletedUids.pop_front();

  // we need to first create an item using the hidden session so that Akonadi knows the item
  // and then delete it using the normal session so that the IMAP resource gets the delete
  Item item;
  item.setMimeType( KMime::Message::mimeType() );
  item.setRemoteId( uid );

  ItemCreateJob *createJob = new ItemCreateJob( item, mCurrentCollection, mHiddenSession );
  connect( createJob, SIGNAL( result( KJob* ) ), q, SLOT( itemDeletePhase1Result( KJob* ) ) );
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

  const QString storeRemoteId = item.remoteId();
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
    createJob->setProperty( "storeRemoteId", storeRemoteId );
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
  const QString storeRemoteId = job->property( "storeRemoteId" ).value<QString>();

  if ( job->error() != 0 ) {
    kWarning() << "Akonadi Create for single item" << item.remoteId() << "returned error: code="
               << job->error() << "text=" << job->errorString();
  } else if ( !storeRemoteId.isEmpty() ) {
    const QStringList tagList = mTagListHash[ storeRemoteId ].value<QStringList>();
    if ( !tagList.isEmpty() ) {
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
    }
  }

  processNextItem();
}

void DImapCacheCollectionMigrator::Private::itemDeletePhase1Result( KJob *job )
{
  ItemCreateJob *createJob = qobject_cast<ItemCreateJob*>( job );
  Q_ASSERT( createJob != 0 );

  const Item item = createJob->item();

  if ( job->error() != 0 ) {
    kWarning() << "Akonadi Create for single item" << item.remoteId() << "returned error: code="
               << job->error() << "text=" << job->errorString();
    processNextDeletedUid();
  } else {
    ItemDeleteJob *deleteJob = new ItemDeleteJob( item );
    connect( deleteJob, SIGNAL( result( KJob* ) ), q, SLOT( itemDeletePhase2Result( KJob* ) ) );
  }
}

void DImapCacheCollectionMigrator::Private::itemDeletePhase2Result( KJob *job )
{
  ItemDeleteJob *deleteJob = qobject_cast<ItemDeleteJob*>( job );
  Q_ASSERT( deleteJob != 0 );

  const Item::List items = deleteJob->deletedItems();
  const Item item = items.isEmpty() ? Item() : items[ 0 ];

  if ( job->error() != 0 ) {
    kWarning() << "Akonadi Delete for single item" << item.remoteId() << "returned error: code="
               << job->error() << "text=" << job->errorString();
  }

  processNextDeletedUid();
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

void DImapCacheCollectionMigrator::setKMailConfig( KConfig *config )
{
  d->mKMailConfig = config;
}

bool DImapCacheCollectionMigrator::migrationOptionsEnabled() const
{
  return d->mImportNewMessages || d->mImportCachedMessages || d->mRemoveDeletedMessages;
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

  kDebug() << "Akonadi collection remoteId=" << collection.remoteId() << ", parent=" << collection.parentCollection().remoteId();

  QString cachePath;
  Collection cache = d->cacheCollection( collection, cachePath );
  kDebug() << "Cache collection remoteId=" << cache.remoteId() << ", parent=" << cache.parentCollection().remoteId();

  d->mDeletedUids.clear();
  if ( d->mRemoveDeletedMessages ) {
    Q_ASSERT( d->mKMailConfig );

    const QString groupName = QLatin1String( "Folder-" ) + cachePath;
    if ( d->mKMailConfig->hasGroup( groupName ) ) {
      d->mCurrentFolderGroup = KConfigGroup( d->mKMailConfig, groupName );
      d->mDeletedUids = d->mCurrentFolderGroup.readEntry( QLatin1String( "UIDSDeletedSinceLastSync" ), QStringList() );
    }
    kDebug() << "DeleteUids=" << d->mDeletedUids;
  }

  d->mCurrentCollection = collection;
  d->mTagListHash.clear();

  emit message( KMigratorBase::Info, i18nc( "@info:status", "Starting cache migration for folder %1 of account %2", collection.name(), resource().name() ) );

  if ( d->mImportNewMessages || d->mImportCachedMessages ) {
    FileStore::ItemFetchJob *job = d->mStore->fetchItems( cache );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( fetchItemsResult( KJob * ) ) );
  } else if ( d->mRemoveDeletedMessages ) {
    d->processNextDeletedUid();
  } else {
    collectionProcessed();
  }
}

#include "dimapcachecollectionmigrator.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
