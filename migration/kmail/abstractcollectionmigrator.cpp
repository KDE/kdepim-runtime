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

#include "abstractcollectionmigrator.h"

#include "libmaildir/maildir.h"

#include <akonadi/agentinstance.h>
#include <akonadi/agentmanager.h>
#include <akonadi/collection.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/monitor.h>

#include <KConfig>
#include <KConfigGroup>
#include <KLocale>

#include <QHash>
#include <QQueue>
#include <QTimer>

using namespace Akonadi;
using KPIM::Maildir;

typedef QHash<Collection::Id, Collection> CollectionHash;
typedef QQueue<Collection> CollectionQueue;

class AbstractCollectionMigrator::Private
{
  AbstractCollectionMigrator *const q;

  public:
    enum Status
    {
      Idle,
      Waiting,
      Scheduling,
      Running,
      Failed,
      Finished
    };

    Private( AbstractCollectionMigrator *parent, const AgentInstance &resource )
      : q( parent ), mResource( resource ), mStatus( Idle ), mKMailConfig( 0 ), mEmailIdentityConfig( 0 ), mMonitor( 0 ),
        mProcessedCollectionsCount( 0 ), mExplicitFetchStatus( Idle )
    {
    }

    void migrateConfig();

  public:
    AgentInstance mResource;
    Status mStatus;
    QString mTopLevelFolder;
    KConfig *mKMailConfig;
    KConfig *mEmailIdentityConfig;

    Monitor *mMonitor;

    CollectionHash mCollectionsById;
    CollectionQueue mCollectionQueue;
    int mProcessedCollectionsCount;

    Status mExplicitFetchStatus;

    Collection mCurrentCollection;
    QString mCurrentFolderId;

  public: // slots
    void collectionAdded( const Collection &collection );
    void fetchResult( KJob *job );
    void processNextCollection();
    void recheckBrokenResource();
    void recheckIdleResource();
    void resourceStatusChanged( const AgentInstance &instance );

  private:
    QStringList folderPathComponentsForCollection( const Collection &collection ) const;
    QString folderIdentifierForCollection( const Collection &collection ) const;
    void processingDone();
};

void AbstractCollectionMigrator::Private::migrateConfig()
{
  // TODO should we have the new settings in a new KMail config?

  if ( mCurrentFolderId.isEmpty() || !mCurrentCollection.isValid() ) {
    return;
  }

//   kDebug( KDE_DEFAULT_DEBUG_AREA ) << "migrating config for folderId" << mCurrentFolderId
//                                    << "to collectionId" << mCurrentCollection.id();

  // general folder specific settings
  const QString folderGroupPattern = QLatin1String( "Folder-%1" );
  if ( mKMailConfig->hasGroup( folderGroupPattern.arg( mCurrentFolderId ) ) ) {
    KConfigGroup oldGroup( mKMailConfig, folderGroupPattern.arg( mCurrentFolderId ) );
    KConfigGroup newGroup( mKMailConfig, folderGroupPattern.arg( mCurrentCollection.id() ) );

    oldGroup.copyTo( &newGroup );
    oldGroup.deleteGroup();
  }

  // check emailidentity
  const QStringList identityGroups = mEmailIdentityConfig->groupList().filter( QRegExp( "Identity #\\d+" ) );
  //kDebug( KDE_DEFAULT_DEBUG_AREA ) << "identityGroups=" << identityGroups;
  Q_FOREACH( const QString &groupName, identityGroups ) {
    KConfigGroup identityGroup( mEmailIdentityConfig, groupName );
    if ( identityGroup.readEntry( "Drafts" ) == mCurrentFolderId )
      identityGroup.writeEntry( "Drafts", mCurrentCollection.id() );
    if ( identityGroup.readEntry( "Templates" ) == mCurrentFolderId )
      identityGroup.writeEntry( "Templates", mCurrentCollection.id() );
    if ( identityGroup.readEntry( "Fcc" ) == mCurrentFolderId )
      identityGroup.writeEntry( "Fcc", mCurrentCollection.id() );

  }


  // Check General/startupFolder
  KConfigGroup general( mKMailConfig, "General" );
  if ( general.readEntry( "startupFolder" ) == mCurrentFolderId )
    general.writeEntry( "startupFolder", mCurrentCollection.id() );

  // check all expire folder
  const QStringList folderGroups = mKMailConfig->groupList().filter( "Folder-" );
  //kDebug( KDE_DEFAULT_DEBUG_AREA ) << "folderGroups=" << folderGroups;
  Q_FOREACH( const QString &groupName, folderGroups ) {
    KConfigGroup filterGroup( mKMailConfig, groupName );
    if ( filterGroup.readEntry( "ExpireToFolder" ) == mCurrentFolderId )
      filterGroup.writeEntry( "ExpireToFolder", mCurrentCollection.id() );
  }

  // check all account folder
  const QStringList accountGroups = mKMailConfig->groupList().filter( QRegExp( "Account \\d+" ) );
  //kDebug( KDE_DEFAULT_DEBUG_AREA ) << "accountGroups=" << accountGroups;
  Q_FOREACH( const QString &groupName, accountGroups ) {
    KConfigGroup filterGroup( mKMailConfig, groupName );

    if ( filterGroup.readEntry( "Folder" ) == mCurrentFolderId )
      filterGroup.writeEntry( "Folder", mCurrentCollection.id() );

    if ( filterGroup.readEntry( "trash" ) == mCurrentFolderId )
      filterGroup.writeEntry( "trash" , mCurrentCollection.id() );
  }

  // check all filters
  const QStringList filterGroups = mKMailConfig->groupList().filter( QRegExp( "Filter #\\d+" ) );
  //kDebug( KDE_DEFAULT_DEBUG_AREA ) << "filterGroups=" << filterGroups;
  Q_FOREACH( const QString &groupName, filterGroups ) {
    KConfigGroup filterGroup( mKMailConfig, groupName );

    const QStringList actionKeys = filterGroup.keyList().filter( QRegExp( "action-args-\\d+" ) );
    //kDebug( KDE_DEFAULT_DEBUG_AREA ) << "actionKeys=" << actionKeys;

    Q_FOREACH( const QString &actionKey, actionKeys ) {
      if ( filterGroup.readEntry( actionKey, QString() ) == mCurrentFolderId ) {
        kDebug( KDE_DEFAULT_DEBUG_AREA ) << "replacing folder id for action" << actionKey
                                         << "in group" << groupName;
        filterGroup.writeEntry( actionKey, mCurrentCollection.id() );
      }
    }
  }

  // check MessageListView::StorageModelSortOrder
  KConfigGroup sortOrderGroup( mKMailConfig, QLatin1String( "MessageListView::StorageModelSortOrder" ) );
  const QString groupSortDirectionPattern = QLatin1String( "%1GroupSortDirection" );
  const QString groupSortingPattern = QLatin1String( "%1GroupSorting" );
  if ( sortOrderGroup.hasKey( groupSortingPattern.arg( mCurrentFolderId ) ) ) {
    const QString value = sortOrderGroup.readEntry( groupSortingPattern.arg( mCurrentFolderId ) );
    sortOrderGroup.writeEntry( groupSortingPattern.arg( mCurrentCollection.id()), value );
    sortOrderGroup.deleteEntry( groupSortingPattern.arg( mCurrentFolderId ) );
  }
  if ( sortOrderGroup.hasKey( groupSortDirectionPattern.arg( mCurrentFolderId ) ) ) {
    const QString value = sortOrderGroup.readEntry( groupSortDirectionPattern.arg( mCurrentFolderId ) );
    sortOrderGroup.writeEntry( groupSortDirectionPattern.arg( mCurrentCollection.id() ), value );
    sortOrderGroup.deleteEntry( groupSortDirectionPattern.arg( mCurrentFolderId ) );
  }


  // check MessageListView::StorageModelSelectedMessages
  KConfigGroup selectedMessagesGroup( mKMailConfig, QLatin1String( "MessageListView::StorageModelSelectedMessages" ) );
  const QString storageModelPattern = QLatin1String( "MessageUniqueIdForStorageModel%1" );
  if ( selectedMessagesGroup.hasKey( storageModelPattern.arg( mCurrentFolderId ) ) ) {
    //kDebug( KDE_DEFAULT_DEBUG_AREA ) << "replacing selected message entry for"
    //                                 << mCurrentFolderId;
    const qulonglong defValue = 0;
    const qulonglong value = selectedMessagesGroup.readEntry( storageModelPattern.arg( mCurrentFolderId ), defValue );
    selectedMessagesGroup.writeEntry( storageModelPattern.arg( mCurrentCollection.id() ), value );
    selectedMessagesGroup.deleteEntry( storageModelPattern.arg( mCurrentFolderId ) );
  }

  // folder specific templates
  const QString templatesGroupPattern = QLatin1String( "Templates #%1" );
  if ( mKMailConfig->hasGroup( templatesGroupPattern.arg( mCurrentFolderId ) ) ) {
    KConfigGroup oldGroup( mKMailConfig, templatesGroupPattern.arg( mCurrentFolderId ) );
    KConfigGroup newGroup( mKMailConfig, templatesGroupPattern.arg( mCurrentCollection.id() ) );

    oldGroup.copyTo( &newGroup );
    oldGroup.deleteGroup();
  }
}

void AbstractCollectionMigrator::Private::collectionAdded( const Collection &collection )
{
  if ( mStatus == Waiting ) {
    mStatus = Idle;
  }

  // don't wait any longer, start explicit fetch right away
  if ( mExplicitFetchStatus == Waiting ) {
    QMetaObject::invokeMethod( q, "recheckIdleResource", Qt::QueuedConnection );
  }

  if ( !mCollectionsById.contains( collection.id() ) ) {
    mCollectionQueue.enqueue( collection );
    mCollectionsById.insert( collection.id(), collection );
  }

  if ( mStatus == Idle ) {
    mStatus = Scheduling;
    QMetaObject::invokeMethod( q, "processNextCollection", Qt::QueuedConnection );
  }
}

void AbstractCollectionMigrator::Private::fetchResult( KJob *job )
{
  mExplicitFetchStatus = Finished;

  CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob*>( job );
  Q_ASSERT( fetchJob != 0 );

  if ( fetchJob->error() != 0 ) {
    q->migrationCancelled( i18nc( "@info:status", "Resource '%1' did not create any folders",
                                  mResource.name() ) );
  } else {
    const Collection::List collections = fetchJob->collections();
    Q_FOREACH( const Collection &collection, collections ) {
      collectionAdded( collection );
    }

    if ( mStatus == Idle ) {
      Q_ASSERT( mCollectionQueue.isEmpty() );
      processingDone();
    }
  }
}

void AbstractCollectionMigrator::Private::processNextCollection()
{
  if ( mStatus == Failed || mStatus == Finished ) {
    return;
  }

  if ( mCollectionQueue.isEmpty() ) {
    if ( mResource.status() == AgentInstance::Idle && mExplicitFetchStatus != Running ) {
      processingDone();
    } else {
      mStatus = Idle;
    }
  } else {
    mStatus = Running;
    ++mProcessedCollectionsCount;

    const Collection collection = mCollectionQueue.dequeue();

    mCurrentFolderId = folderIdentifierForCollection( collection );
    mCurrentCollection = collection;

    q->migrateCollection( collection, mCurrentFolderId );
  }
}

void AbstractCollectionMigrator::Private::recheckBrokenResource()
{
  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "mStatus=" << mStatus << "mResource.status()=" << mResource.status();
  if ( mStatus == Waiting ) {
    q->migrationCancelled( i18nc( "@info:status", "Resource '%1' is not working correctly",
                                  mResource.name() ) );
  }
}

void AbstractCollectionMigrator::Private::recheckIdleResource()
{
  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "mStatus=" << mStatus << "mResource.status()=" << mResource.status();

  if ( mExplicitFetchStatus == Waiting ) {
    mExplicitFetchStatus = Running;

    CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );

    CollectionFetchScope colScope;
    colScope.setResource( mResource.identifier() );
    colScope.setAncestorRetrieval( CollectionFetchScope::All );
    job->setFetchScope( colScope );

    connect( job, SIGNAL( result( KJob* ) ), q, SLOT( fetchResult( KJob* ) ) );
  }
}

void AbstractCollectionMigrator::Private::resourceStatusChanged( const AgentInstance &instance )
{
  if ( instance.identifier() != mResource.identifier() ) {
    return;
  }

  const AgentInstance::Status oldStatus = mResource.status();
  const QString oldMessage = mResource.statusMessage();
  mResource = instance;

  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "oldStatus=" << oldStatus << "message=" << oldMessage
           << "newStatus" << mResource.status() << "message=" << mResource.statusMessage();

  if ( oldStatus != AgentInstance::Idle && mResource.status() == AgentInstance::Idle && mExplicitFetchStatus == Idle ) {
    mExplicitFetchStatus = Waiting;

    // if resource is now "Idle" it might still need time to process until it becomes ready
    // unfortunately this is not a separate state so lets delay the explicit fetch
    // wait for at most one minute
    QTimer::singleShot( 60000, q, SLOT( recheckIdleResource() ) );
  }

  if ( mStatus == Waiting ) {
    mStatus = Idle;
  }
}

QStringList AbstractCollectionMigrator::Private::folderPathComponentsForCollection( const Collection &collection ) const
{
  QStringList result;

  if ( collection.parentCollection() == Collection::root() ) {
    if ( !mTopLevelFolder.isEmpty() ) {
      result << mTopLevelFolder;
    }
  } else {
    result = folderPathComponentsForCollection( collection.parentCollection() );
    result << collection.remoteId();
  }

  return result;
}

QString AbstractCollectionMigrator::Private::folderIdentifierForCollection( const Collection &collection ) const
{
  QStringList components = folderPathComponentsForCollection( collection );

  QString result;
  while ( !components.isEmpty() ) {
    QString component = components.front();
    component.remove( QLatin1Char( '/' ) );

    components.pop_front();
    if ( !components.isEmpty() ) {
      result += Maildir::subDirNameForFolderName( component ) + QLatin1Char( '/' );
    } else {
      result += component;
    }
  }

  return result;
}

void AbstractCollectionMigrator::Private::processingDone()
{
  if ( mCollectionsById.count() == 0 ) {
    q->migrationCancelled( i18nc( "@info:status", "Resource '%1' did not create any folders",
                                  mResource.name() ) );
  } else {
    q->migrationDone();
  }
}

AbstractCollectionMigrator::AbstractCollectionMigrator( const AgentInstance &resource, QObject *parent )
  : QObject( parent ), d( new Private( this, resource ) )
{
  CollectionFetchScope colScope;
  colScope.setResource( d->mResource.identifier() );
  colScope.setAncestorRetrieval( CollectionFetchScope::All );

  d->mMonitor = new Monitor( this );
  d->mMonitor->setResourceMonitored( d->mResource.identifier().toAscii(), true );
  d->mMonitor->fetchCollection( true );
  d->mMonitor->setCollectionFetchScope( colScope );

  connect( d->mMonitor, SIGNAL( collectionAdded( Akonadi::Collection, Akonadi::Collection ) ), SLOT( collectionAdded( Akonadi::Collection ) ) );

  if ( d->mResource.status() == AgentInstance::Idle ) {
    // if resource is "Idle" it might still need time to process until it becomes ready
    // unfortunately this is not a separate state so lets delay the explicit fetch
    // wait for at most one minute
    QTimer::singleShot( 60000, this, SLOT( recheckIdleResource() ) );
  } else if ( d->mResource.status() == AgentInstance::Broken ) {
    // if resource is "Broken", it could still become idle after fully processing its new config
    // wait for at most one minute
    QTimer::singleShot( 60000, this, SLOT( recheckBrokenResource() ) );
  }

  // monitor resource status so we know when to quit waiting
  connect( AgentManager::self(), SIGNAL( instanceStatusChanged( Akonadi::AgentInstance ) ),
           this, SLOT( resourceStatusChanged( Akonadi::AgentInstance  ) ) );
}

AbstractCollectionMigrator::~AbstractCollectionMigrator()
{
  delete d;
}

void AbstractCollectionMigrator::setTopLevelFolder( const QString &topLevelFolder )
{
  d->mTopLevelFolder = topLevelFolder;
}

QString AbstractCollectionMigrator::topLevelFolder() const
{
  return d->mTopLevelFolder;
}

void AbstractCollectionMigrator::setKMailConfig( KConfig *config )
{
  d->mKMailConfig = config;
}

void AbstractCollectionMigrator::setEmailIdentityConfig( KConfig *config )
{
  d->mEmailIdentityConfig = config;
}

void AbstractCollectionMigrator::migrationProgress( int processedCollections, int seenCollections )
{
  emit progress( 0, seenCollections, processedCollections );
}

void AbstractCollectionMigrator::collectionProcessed()
{
  d->migrateConfig();

  d->mCurrentFolderId = QString();
  d->mCurrentCollection = Collection();

  d->mStatus = Private::Scheduling;
  QMetaObject::invokeMethod( this, "processNextCollection", Qt::QueuedConnection );

  migrationProgress( d->mProcessedCollectionsCount, d->mCollectionsById.count() );
}

void AbstractCollectionMigrator::migrationDone()
{
  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "processed" << d->mProcessedCollectionsCount << "collection"
                                   << "seen" << d->mCollectionsById.count();
  d->mStatus = Private::Finished;
  emit migrationFinished( d->mResource, QString() );
}

void AbstractCollectionMigrator::migrationCancelled( const QString &error )
{
  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "processed" << d->mProcessedCollectionsCount << "collection"
                                   << "seen" << d->mCollectionsById.count();
  d->mStatus = Private::Failed;
  emit migrationFinished( d->mResource, error );
}

const AgentInstance AbstractCollectionMigrator::resource() const
{
  return d->mResource;
}

KConfig *AbstractCollectionMigrator::kmailConfig() const
{
  return d->mKMailConfig;
}

KConfig *AbstractCollectionMigrator::emailIdentityConfig() const
{
  return d->mEmailIdentityConfig;
}

#include "abstractcollectionmigrator.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
