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

#include "imapcachecollectionmigrator.h"

#include "kmigratorbase.h"

#include "kmindexreader/messagestatus.h"
#include "libmaildir/maildir.h"
#include "mixedmaildirstore.h"
#include "subscriptionjob_p.h"

#include "filestore/itemfetchjob.h"
#include "filestore/itemdeletejob.h"

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
#include <QSet>
#include <QVariant>

using namespace Akonadi;
using KPIM::Maildir;

typedef QHash<QString, QString> UidHash;

class ImapCacheCollectionMigrator::Private
{
  ImapCacheCollectionMigrator *const q;

  public:
    Private( ImapCacheCollectionMigrator *parent, MixedMaildirStore *store )
      : q( parent ), mStore( store ), mHiddenSession( 0 ),
        mImportNewMessages( false ), mImportCachedMessages( false ),
        mRemoveDeletedMessages( false ), mDeleteImportedMessages( false ),
        mItemProgress( -1 )
    {
    }

    ~Private()
    {
      delete mHiddenSession;
    }

    Collection cacheCollection( const Collection &collection ) const;

    bool isUnsubscribedImapFolder( const Collection &collection, QString &idPath ) const;

  public:
    MixedMaildirStore *mStore;

    Session *mHiddenSession;

    Collection mCurrentCollection;
    Item::List mItems;
    UidHash mUidHash;
    QStringList mDeletedUids;

    bool mImportNewMessages;
    bool mImportCachedMessages;
    bool mRemoveDeletedMessages;
    bool mDeleteImportedMessages;

    KConfigGroup mCurrentFolderGroup;

    QHash<QString, QVariant> mTagListHash;

    int mItemProgress;

    QSet<QString> mUnsubscribedImapFolders;
    Collection::List mUnsubscribedCollections;

  public: // slots
    void fetchItemsResult( KJob *job );
    void processNextItem();
    void processNextDeletedUid();
    void fetchItemResult( KJob *job );
    void itemCreateResult( KJob *job );
    void itemDeletePhase1Result( KJob *job );
    void itemDeletePhase2Result( KJob *job );
    void cacheItemDeleteResult( KJob *job );
    void unsubscribeCollections();
    void unsubscribeCollectionsResult( KJob *job );
};

Collection ImapCacheCollectionMigrator::Private::cacheCollection( const Collection &collection ) const
{
  if ( collection.parentCollection() == Collection::root() ) {
    Collection cache;
    cache.setRemoteId( q->topLevelFolder() );
    cache.setParentCollection( mStore->topLevelCollection() );
    return cache;
  }

  const QString remoteId = collection.remoteId().mid( 1 );

  Collection cache;
  cache.setRemoteId( remoteId );
  cache.setParentCollection( cacheCollection( collection.parentCollection() ) );
  return cache;
}

bool ImapCacheCollectionMigrator::Private::isUnsubscribedImapFolder( const Collection &collection, QString &idPath ) const
{
  if ( collection.parentCollection() == Collection::root() ) {
    idPath = QString();
    return false;
  }

  bool parentResult = isUnsubscribedImapFolder( collection.parentCollection(), idPath );

  idPath = idPath + collection.remoteId();

  return parentResult || mUnsubscribedImapFolders.contains( idPath );
}

void ImapCacheCollectionMigrator::Private::fetchItemsResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kWarning() << "Store Fetch for item list in collection" << mCurrentCollection.remoteId()
               << "returned error: code=" << job->error() << "text=" << job->errorString();
    if ( mRemoveDeletedMessages ) {
      processNextDeletedUid();
    } else {
      emit q->status( QString() );
      mCurrentCollection = Collection();
      q->collectionProcessed();
    }
    return;
  }

  FileStore::ItemFetchJob *fetchJob = qobject_cast<FileStore::ItemFetchJob*>( job );
  Q_ASSERT( fetchJob != 0 );

  const QVariant uidHashVar = fetchJob->property( "remoteIdToIndexUid" );
  if ( !uidHashVar.isValid() ) {
    kDebug( KDE_DEFAULT_DEBUG_AREA ) << "No UIDs from index for collection" << mCurrentCollection.name();
    if ( mRemoveDeletedMessages ) {
      processNextDeletedUid();
    } else {
      emit q->status( QString() );
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
  kDebug( KDE_DEFAULT_DEBUG_AREA ) << mItems.count() << "items for target collection" << mCurrentCollection.remoteId();

  const QVariant tagListHashVar = fetchJob->property( "remoteIdToTagList" );
  if ( !tagListHashVar.isValid() ) {
    kDebug( KDE_DEFAULT_DEBUG_AREA ) << "No tags from index for collection" << mCurrentCollection.name();
  } else {
    mTagListHash = tagListHashVar.value< QHash<QString, QVariant> >();
    kDebug( KDE_DEFAULT_DEBUG_AREA ) << mTagListHash.count() << "items have tags";
  }

  mItemProgress = -1;
  emit q->progress( 0, mItems.count(), 0 );
  processNextItem();
}

void ImapCacheCollectionMigrator::Private::processNextItem()
{
//   kDebug( KDE_DEFAULT_DEBUG_AREA ) << "mCurrentCollection=" << mCurrentCollection.name()
//                                    << mItems.count() << "items to go";

  emit q->progress( ++mItemProgress );
  emit q->status( i18ncp( "@info:status folder name and number of messages to import before finished",
                          "%1: one message left to import", "%1: %2 messages left to import",
                          mCurrentCollection.name(), mItems.count() ) );

  if ( mItems.isEmpty() ) {
    if ( mDeletedUids.isEmpty() ) {
      emit q->status( QString() );
      mCurrentCollection = Collection();
      q->collectionProcessed();
    } else if ( mRemoveDeletedMessages ) {
      processNextDeletedUid();
    }
    return;
  }

  const Item item = mItems.front();
  mItems.pop_front();

  // don't import items that are marked deleted. These come from normal/online IMAP caches
  // which didn't get their mbox cache files compacted
  KPIM::MessageStatus status;
  status.setStatusFromFlags( item.flags() );
  if ( status.isDeleted() ) {
    //kDebug() << "Cache item" << item.remoteId() << "is marked as Deleted. Skip it";
    QMetaObject::invokeMethod( q, "processNextItem", Qt::QueuedConnection );
    return;
  }

  FileStore::ItemFetchJob *job = mStore->fetchItem( item );
  job->fetchScope().fetchFullPayload( true );
  connect( job, SIGNAL( result( KJob* ) ), q, SLOT( fetchItemResult( KJob* ) ) );
}

void ImapCacheCollectionMigrator::Private::processNextDeletedUid()
{
//   kDebug( KDE_DEFAULT_DEBUG_AREA ) << "mCurrentCollection=" << mCurrentCollection.name()
//                                    << mDeletedUids.count() << "items to go";

  if ( mDeletedUids.isEmpty() ) {
    if ( mCurrentFolderGroup.isValid() ) {
      const QString key = QLatin1String( "UIDSDeletedSinceLastSync" );
      if ( !mCurrentFolderGroup.readEntry( key, QStringList() ).isEmpty() ) {
        mCurrentFolderGroup.deleteEntry( QLatin1String( "UIDSDeletedSinceLastSync" ) );
      }
    }
    mCurrentFolderGroup = KConfigGroup();
    mCurrentCollection = Collection();
    emit q->status( QString() );
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

void ImapCacheCollectionMigrator::Private::fetchItemResult( KJob *job )
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

//    kDebug( KDE_DEFAULT_DEBUG_AREA ) << "unsynchronized cacheItem: remoteId=" << item.remoteId()
//                                     << "mimeType=" << item.mimeType()
//                                     << "flags=" << item.flags();
  } else if ( mImportCachedMessages ) {
    item.setRemoteId( uid );
    createJob = new ItemCreateJob( item, mCurrentCollection, mHiddenSession );

//    kDebug( KDE_DEFAULT_DEBUG_AREA ) << "synchronized cacheItem: remoteId=" << item.remoteId()
//                                     << "mimeType=" << item.mimeType()
//                                     << "flags=" << item.flags();
  }

  if ( createJob != 0 ) {
    createJob->setProperty( "storeRemoteId", storeRemoteId );
    createJob->setProperty( "storeParentCollection", QVariant::fromValue<Collection>( item.parentCollection() ) );
    connect( createJob, SIGNAL( result( KJob* ) ), q, SLOT( itemCreateResult( KJob* ) ) );
  } else {
    kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Skipping cacheItem: remoteId=" << item.remoteId()
                                     << "mimeType=" << item.mimeType()
                                     << "flags=" << item.flags();
    processNextItem();
  }
}

void ImapCacheCollectionMigrator::Private::itemCreateResult( KJob *job )
{
  ItemCreateJob *createJob = qobject_cast<ItemCreateJob*>( job );
  Q_ASSERT( createJob != 0 );

  const Item item = createJob->item();
  const QString storeRemoteId = job->property( "storeRemoteId" ).value<QString>();

  if ( job->error() != 0 ) {
    kWarning() << "Akonadi Create for single item" << item.remoteId() << "returned error: code="
               << job->error() << "text=" << job->errorString();

    processNextItem();
  } else if ( !storeRemoteId.isEmpty() ) {
    const QStringList tagList = mTagListHash[ storeRemoteId ].value<QStringList>();
    if ( !tagList.isEmpty() ) {
      kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Tagging item" << item.url() << "with" << tagList;

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

    const Collection storeCollection = job->property( "storeParentCollection" ).value<Collection>();
    if ( mDeleteImportedMessages && !storeCollection.remoteId().isEmpty() ) {
      Item cacheItem = item;
      cacheItem.setRemoteId( storeRemoteId );
      cacheItem.setParentCollection( storeCollection );
      FileStore::ItemDeleteJob *deleteJob = mStore->deleteItem( cacheItem );
      connect( deleteJob, SIGNAL( result( KJob* ) ), q, SLOT( cacheItemDeleteResult( KJob* ) ) );
    } else {
      processNextItem();
    }
  } else {
    processNextItem();
  }
}

void ImapCacheCollectionMigrator::Private::itemDeletePhase1Result( KJob *job )
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

void ImapCacheCollectionMigrator::Private::itemDeletePhase2Result( KJob *job )
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

void ImapCacheCollectionMigrator::Private::cacheItemDeleteResult( KJob *job )
{
  FileStore::ItemDeleteJob *deleteJob = qobject_cast<FileStore::ItemDeleteJob*>( job );
  Q_ASSERT( deleteJob != 0 );

  const Item item = deleteJob->item();

  if ( job->error() != 0 ) {
    kWarning() << "Store Delete for single item" << item.remoteId() << "returned error: code="
               << job->error() << "text=" << job->errorString();
  }

  processNextItem();
}

void ImapCacheCollectionMigrator::Private::unsubscribeCollections()
{
  if ( !mUnsubscribedCollections.isEmpty() ) {
    kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Locally Unsubscribe" << mUnsubscribedCollections.count() << "collections";

    SubscriptionJob *job = new SubscriptionJob( q );
    job->unsubscribe( mUnsubscribedCollections );
    QObject::connect( job, SIGNAL( result( KJob* ) ), q, SLOT( unsubscribeCollectionsResult( KJob * ) ) );
  }
}

void ImapCacheCollectionMigrator::Private::unsubscribeCollectionsResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kError() << "Unsubscribing of " << mUnsubscribedCollections.count() << "collections failed:"
             << job->error();
  } else {
    kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Unsubscribing of " << mUnsubscribedCollections.count() << "collections succeeded";
  }
}

ImapCacheCollectionMigrator::ImapCacheCollectionMigrator( const AgentInstance &resource, MixedMaildirStore *store, QObject *parent )
  : AbstractCollectionMigrator( resource, parent ), d( new Private( this, store ) )
{
  d->mHiddenSession = new Session( resource.identifier().toAscii() );

  connect( this, SIGNAL( migrationFinished( Akonadi::AgentInstance, QString ) ),
           SLOT( unsubscribeCollections() ) );
}

ImapCacheCollectionMigrator::~ImapCacheCollectionMigrator()
{
  delete d;
}

void ImapCacheCollectionMigrator::setMigrationOptions( const MigrationOptions &options )
{
  MigrationOptions actualOptions = options;

  if ( d->mStore == 0 ) {
    emit message( KMigratorBase::Skip,
                  i18nc( "@info:status", "No cache for account %1 available",
                         resource().name() ) );
    kWarning() << "No store for folder" << topLevelFolder()
               << "so only config migration (instead of" << options << ") for"
               << resource().identifier() << resource().name();
    actualOptions = ConfigOnly;
  }

  d->mImportNewMessages = actualOptions.testFlag( ImportNewMessages );
  d->mImportCachedMessages = actualOptions.testFlag( ImportCachedMessages );
  d->mRemoveDeletedMessages = actualOptions.testFlag( RemoveDeletedMessages );
  d->mDeleteImportedMessages = actualOptions.testFlag( DeleteImportedMessages );
}

ImapCacheCollectionMigrator::MigrationOptions ImapCacheCollectionMigrator::migrationOptions() const
{
  MigrationOptions options;
  if ( d->mImportNewMessages ) {
    options |= ImportNewMessages;
  }
  if ( d->mImportCachedMessages ) {
    options |= ImportCachedMessages;
  }
  if ( d->mRemoveDeletedMessages ) {
    options |= RemoveDeletedMessages;
  }
  if ( d->mDeleteImportedMessages ) {
    options |= DeleteImportedMessages;
  }
  return options;
}

void ImapCacheCollectionMigrator::setUnsubscribedImapFolders( const QStringList &imapFolders )
{
  d->mUnsubscribedImapFolders.clear();
  Q_FOREACH( const QString imapFolder, imapFolders ) {
    if ( imapFolder.endsWith( QLatin1Char( '/' ) ) ) {
      d->mUnsubscribedImapFolders << imapFolder.left( imapFolder.size() - 1 );
    } else {
      d->mUnsubscribedImapFolders << imapFolder;
    }
  }
  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "unsubscribed imap folders:" << d->mUnsubscribedImapFolders;
}

void ImapCacheCollectionMigrator::migrateCollection( const Collection &collection, const QString &folderId )
{
  QString imapIdPath;
  if ( d->isUnsubscribedImapFolder( collection, imapIdPath ) ) {
    kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Collection id=" << collection.id()
      << ", remoteId=" << collection.remoteId() << ", imapIdPath=" << imapIdPath
      << "is locally unsubscribed";

    // could check if this very collection is one of the unsubscribed using imapIdPath.
    // however, KMail treats subfolders of unsubscribed folders as unsubscribed as well
    // so unsubscribe the too. otherwise their contents get downloaded on first interval check
    d->mUnsubscribedCollections << collection;

    emit status( QString() );
    collectionProcessed();
    return;
  }

  if ( migrationOptions() == ConfigOnly ) {
    emit status( QString() );
    collectionProcessed();
    return;
  }

  // check that we don't get entered while we are still processing
  Q_ASSERT( !d->mCurrentCollection.isValid() );

  Q_ASSERT( d->mStore != 0 );

  if ( collection.parentCollection() == Collection::root() ) {
    emit status( QString() );
    collectionProcessed();
    return;
  }

  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Akonadi collection remoteId=" << collection.remoteId() << ", parent=" << collection.parentCollection().remoteId();

  Collection cache = d->cacheCollection( collection );
  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Cache collection remoteId=" << cache.remoteId() << ", parent=" << cache.parentCollection().remoteId();

  d->mDeletedUids.clear();
  if ( d->mRemoveDeletedMessages ) {
    Q_ASSERT( kmailConfig() );

    const QString groupName = QLatin1String( "Folder-" ) + folderId;
    if ( kmailConfig()->hasGroup( groupName ) ) {
      d->mCurrentFolderGroup = KConfigGroup( kmailConfig(), groupName );
      d->mDeletedUids = d->mCurrentFolderGroup.readEntry( QLatin1String( "UIDSDeletedSinceLastSync" ), QStringList() );
    }
    kDebug( KDE_DEFAULT_DEBUG_AREA ) << "DeleteUids=" << d->mDeletedUids;
  }

  d->mCurrentCollection = collection;
  d->mTagListHash.clear();

  emit message( KMigratorBase::Info, i18nc( "@info:status", "Starting cache migration for folder %1 of account %2", collection.name(), resource().name() ) );

  emit status( collection.name() );

  if ( d->mImportNewMessages || d->mImportCachedMessages ) {
    FileStore::ItemFetchJob *job = d->mStore->fetchItems( cache );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( fetchItemsResult( KJob * ) ) );
    emit status( i18nc( "@info:status foldername", "%1: listing messages...", collection.name() ) );
  } else if ( d->mRemoveDeletedMessages ) {
    emit status( collection.name() );
    d->processNextDeletedUid();
  } else {
    emit status( QString() );
    collectionProcessed();
  }
}

void ImapCacheCollectionMigrator::migrationProgress( int processedCollections, int seenCollections )
{
  // if we potentially migrate items, use item progress instead
  // use base implementation if only collections are processed
  if ( migrationOptions() == ConfigOnly ) {
    AbstractCollectionMigrator::migrationProgress( processedCollections, seenCollections );
  }
}

#include "imapcachecollectionmigrator.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
