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

#include "imapcachelocalimporter.h"

#include "maildirsettings.h"
#include "mixedmaildirstore.h"

#include "filestore/collectionfetchjob.h"
#include "filestore/itemfetchjob.h"

#include <kmime/kmime_message.h>

#include <akonadi/agentinstance.h>
#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/collection.h>
#include <akonadi/collectioncreatejob.h>
#include <akonadi/item.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/session.h>

#include <KDebug>
#include <KLocale>

using namespace Akonadi;

static QString remoteIdPath( const Collection& collection ) {
  const Collection parent = collection.parentCollection();
  if ( parent.remoteId().isEmpty() ) {
    return collection.remoteId();
  }

  return remoteIdPath( parent ) + QLatin1Char( '/' ) + collection.remoteId();
}

class ImapCacheLocalImporter::Private
{
  ImapCacheLocalImporter *const q;

  public:
    Private( ImapCacheLocalImporter *parent, MixedMaildirStore *store )
      : q( parent ), mStore( store ), mHiddenSession( 0 )
    {
    }

    ~Private()
    {
      delete mHiddenSession;
    }

    void processNextCollection();
    void processNextItem();

  public:
    MixedMaildirStore *mStore;
    Session *mHiddenSession;

    QString mTopLevelFolder;
    QString mAccountName;

    AgentInstance mResource;

    Collection::List mPendingCollections;
    Item::List mPendingItems;

    typedef QHash<QString, Collection> CollectionHash;
    CollectionHash mStoreCollectionsByPath;
    CollectionHash mAkonadiCollectionsByPath;

  public: // slots
    void createResourceResult( KJob *job );
    void configureResource();
    void collectionFetchResult( KJob *job );
    void collectionCreateResult( KJob *job );
    void itemFetchResult( KJob *job );
    void itemCreateResult( KJob *job );
};

void ImapCacheLocalImporter::Private::processNextCollection()
{
  if ( mPendingCollections.isEmpty() ) {
    configureResource();
    return;
  }

  const Collection storeCollection = mPendingCollections.front();
  mPendingCollections.pop_front();

  const QString idPath = remoteIdPath( storeCollection );
  mStoreCollectionsByPath.insert( idPath, storeCollection );
//   kDebug( KDE_DEFAULT_DEBUG_AREA ) << "inserting storeCollection" << storeCollection.remoteId()
//                                    << "at idPath" << idPath;

  // create on Akonadi server
  Collection collection = storeCollection;
  const Collection parent = collection.parentCollection();
  if ( parent.remoteId() == mStore->topLevelCollection().remoteId() ) {
    collection.setParentCollection( Collection::root() );

    const QFileInfo pathInfo( QDir( mStore->path() ), mTopLevelFolder );
    collection.setRemoteId( pathInfo.absoluteFilePath() );
    collection.setName( i18nc( "@title account name", "Local Copies of %1", mAccountName ) );
  } else {
    CollectionHash::const_iterator findIt = mAkonadiCollectionsByPath.constFind( remoteIdPath( parent ) );
    if ( findIt != mAkonadiCollectionsByPath.constEnd() ) {
      collection.setParentCollection( findIt.value() );
    } else {
      kError() << "storeCollection with idPath" << idPath << "parent=" << parent.remoteId()
               << "does not have parent in Akonadi collection hash";
    }
  }

  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Processing collection" << collection.remoteId()
                                   << "remoteIdPath=" << idPath
                                   << ", " << mPendingCollections.count() << "remaining";

  CollectionCreateJob *createJob = new CollectionCreateJob( collection, mHiddenSession );
  createJob->setProperty( "remoteIdPath", idPath );
  QObject::connect( createJob, SIGNAL( result( KJob* ) ), q, SLOT( collectionCreateResult( KJob* ) ) );
}

void ImapCacheLocalImporter::Private::processNextItem()
{
  if ( mPendingItems.isEmpty() ) {
    processNextCollection();
    return;
  }

  const Item item = mPendingItems.front();
  mPendingItems.pop_front();

//   kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Processing item" << item.remoteId()
//                                    << "parentCollection" << item.parentCollection().id()
//                                    << "remoteIdPath=" << remoteIdPath( item.parentCollection() )
//                                    << ", " << mPendingCollections.count() << "remaining";

  ItemCreateJob *createJob = new ItemCreateJob( item, item.parentCollection(), mHiddenSession );
  QObject::connect( createJob, SIGNAL( result( KJob* ) ), q, SLOT( itemCreateResult( KJob* ) ) );
}

void ImapCacheLocalImporter::Private::createResourceResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kError() << "Creation of Maildir resource for local cache copy of account"
             << mAccountName << "failed:" << job->errorString();
    emit q->importFinished( i18nc( "@info", "Cannot provide access to local copies of "
                                            "disconnected IMAP account %1",
                                            mAccountName ) );
    return;
  }

  AgentInstanceCreateJob *createJob = qobject_cast<AgentInstanceCreateJob*>( job );
  mResource = createJob->instance();

  mHiddenSession = new Session( mResource.identifier().toAscii() );

  Collection topLevelCollection;
  topLevelCollection.setRemoteId( mTopLevelFolder );
  topLevelCollection.setParentCollection( mStore->topLevelCollection() );
  topLevelCollection.setContentMimeTypes( QStringList() << Collection::mimeType() << KMime::Message::mimeType() );

  mPendingCollections << topLevelCollection;

  FileStore::CollectionFetchJob *fetchJob = mStore->fetchCollections( topLevelCollection, FileStore::CollectionFetchJob::Recursive );
  QObject::connect( fetchJob, SIGNAL( result( KJob* ) ), q, SLOT( collectionFetchResult( KJob* ) ) );
}

void ImapCacheLocalImporter::Private::configureResource()
{
  OrgKdeAkonadiMaildirSettingsInterface *iface = new OrgKdeAkonadiMaildirSettingsInterface(
    "org.freedesktop.Akonadi.Resource." + mResource.identifier(),
    "/Settings", QDBusConnection::sessionBus(), q );

  if (!iface->isValid() ) {
    q->importFinished( i18n("Failed to obtain D-Bus interface for remote configuration.") );
    return;
  }

  const QFileInfo pathInfo( QDir( mStore->path() ), mTopLevelFolder );
  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "resource working path=" << pathInfo.absoluteFilePath();
  iface->setPath( pathInfo.absoluteFilePath()  );

  mResource.setName( i18nc( "@title account name", "Local Copies of %1", mAccountName ) );
  mResource.reconfigure();

  emit q->importFinished( QString() );
}

void ImapCacheLocalImporter::Private::collectionFetchResult( KJob *job )
{
  if ( job->error() != 0 ) {
    kError() << "Store CollectionFetch failed:" << job->errorString();
  } else {
    FileStore::CollectionFetchJob *fetchJob = qobject_cast<FileStore::CollectionFetchJob*>( job );
    Q_ASSERT( fetchJob != 0 );

    mPendingCollections << fetchJob->collections();
  }

  processNextCollection();
}

void ImapCacheLocalImporter::Private::collectionCreateResult( KJob *job )
{
  const QString idPath = job->property( "remoteIdPath" ).value<QString>();
  if ( job->error() != 0 ) {
    kError() << "Akonadi CollectionCreate for" << idPath << "failed:" << job->errorString();
    processNextCollection();
    return;
  }

  CollectionCreateJob *createJob = qobject_cast<CollectionCreateJob*>( job );
  Q_ASSERT( createJob != 0 );

  const Collection collection = createJob->collection();
/*  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "inserting collection" << collection.id()
                                   << collection.remoteId()
                                   << "at idPath" << idPath;*/
  mAkonadiCollectionsByPath.insert( idPath, collection );

/*  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Checking for storeCollection with idPath" << idPath;*/
  const Collection storeCollection = mStoreCollectionsByPath[ idPath ];
  Q_ASSERT( !storeCollection.remoteId().isEmpty() );

  FileStore::ItemFetchJob *fetchJob = mStore->fetchItems( storeCollection );
  fetchJob->setProperty( "remoteIdPath", idPath );
  QObject::connect( fetchJob, SIGNAL( result( KJob* ) ), q, SLOT( itemFetchResult( KJob* ) ) );
}

void ImapCacheLocalImporter::Private::itemFetchResult( KJob *job )
{
  const QString idPath = job->property( "remoteIdPath" ).value<QString>();
  if ( job->error() != 0 ) {
    kError() << "Store ItemFetch for" << idPath << "failed:" << job->errorString();
    processNextCollection();
    return;
  }

  FileStore::ItemFetchJob *fetchJob = qobject_cast<FileStore::ItemFetchJob*>( job );
  Q_ASSERT( fetchJob != 0 );

  const Collection collection = mAkonadiCollectionsByPath[ idPath ];
  Q_ASSERT( collection.isValid() );

  Item::List items = fetchJob->items();
  Item::List::iterator it    = items.begin();
  Item::List::iterator endIt = items.end();
  for ( ; it != endIt; ++it ) {
    (*it).setParentCollection( collection );
  }

  mPendingItems << items;

  processNextItem();
}

void ImapCacheLocalImporter::Private::itemCreateResult( KJob *job )
{
  ItemCreateJob *createJob = qobject_cast<ItemCreateJob*>( job );
  Q_ASSERT( createJob != 0 );
  if ( createJob->error() != 0 ) {
    const Item item = createJob->item();
    kError() << "Akonadi ItemCreate for" << item.parentCollection().remoteId()
             << "/" << item.remoteId() << "failed:" << job->errorString();
    return;
  }

  processNextItem();
}

ImapCacheLocalImporter::ImapCacheLocalImporter( MixedMaildirStore *store, QObject *parent )
  : QObject( parent ), d( new Private( this, store ) )
{
  Q_ASSERT( store != 0 );
}

ImapCacheLocalImporter::~ImapCacheLocalImporter()
{
  delete d;
}

void ImapCacheLocalImporter::setTopLevelFolder( const QString &topLevelFolder )
{
  d->mTopLevelFolder = topLevelFolder;
}

void ImapCacheLocalImporter::setAccountName( const QString &accountName )
{
  d->mAccountName = accountName;
}

void ImapCacheLocalImporter::startImport()
{
  Q_ASSERT( !d->mTopLevelFolder.isEmpty() );
  Q_ASSERT( !d->mAccountName.isEmpty() );

  const QString typeId = QLatin1String( "akonadi_maildir_resource" );
  AgentInstanceCreateJob *job = new AgentInstanceCreateJob( typeId, this );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( createResourceResult( KJob* ) ) );
  job->start();
}

#include "imapcachelocalimporter.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
