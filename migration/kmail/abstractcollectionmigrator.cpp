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

#include <akonadi/kmime/specialmailcollections.h>
#include <akonadi/kmime/messagefolderattribute.h>
#include <akonadi/agentinstance.h>
#include <akonadi/agentmanager.h>
#include <akonadi/collection.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>

#include "collectionannotationsattribute.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocale>

#include <QHash>
#include <QQueue>
#include <QTimer>

#define KOLAB_SHAREDSEEN "/vendor/cmu/cyrus-imapd/sharedseen"
#define KOLAB_INCIDENCESFOR "/vendor/kolab/incidences-for"
#define KOLAB_FOLDERTYPE "/vendor/kolab/folder-type"


using namespace Akonadi;
using KPIM::Maildir;

typedef QHash<Collection::Id, Collection> CollectionHash;
typedef QQueue<Collection> CollectionQueue;

typedef QHash<int, QString> IconNameHash;

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

    Private( AbstractCollectionMigrator *parent, const AgentInstance &resource, MixedMaildirStore *store )
      : q( parent ), mResource( resource ),  mStore( store ), mHiddenSession( 0 ), mStatus( Idle ), mKMailConfig( 0 ), mEmailIdentityConfig( 0 ), mKcmKmailSummaryConfig( 0 ), mTemplatesConfig( 0 ), mMonitor( 0 ),
        mProcessedCollectionsCount( 0 ), mExplicitFetchStatus( Idle ),
        mNeedModifyJob( false )
    {
      mRecheckTimer.setSingleShot( true );
    }

    ~Private()
    {
      delete mHiddenSession;
    }

    void migrateConfig();
    void collectionDone();

  public:
    AgentInstance mResource;
    MixedMaildirStore *mStore;
    Session *mHiddenSession;

    Status mStatus;
    QString mTopLevelFolder;
    KSharedConfigPtr mKMailConfig;
    KSharedConfigPtr mEmailIdentityConfig;
    KSharedConfigPtr mKcmKmailSummaryConfig;
    KSharedConfigPtr mTemplatesConfig;
    Monitor *mMonitor;

    CollectionHash mCollectionsById;
    CollectionQueue mCollectionQueue;
    int mProcessedCollectionsCount;

    Status mExplicitFetchStatus;

    Collection mCurrentCollection;
    QString mCurrentFolderId;

    QTimer mRecheckTimer;

    bool mNeedModifyJob;

    static IconNameHash mIconNamesBySpecialType;

  public: // slots
    void collectionAdded( const Collection &collection );
    void fetchResult( KJob *job );
    void modifyResult( KJob *job );
    void processNextCollection();
    void recheckBrokenResource();
    void recheckIdleResource();
    void resourceStatusChanged( const AgentInstance &instance );

  private:
    QStringList folderPathComponentsForCollection( const Collection &collection ) const;
    QString folderIdentifierForCollection( const Collection &collection ) const;
    void processingDone();
};

IconNameHash AbstractCollectionMigrator::Private::mIconNamesBySpecialType;

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
    if ( newGroup.readEntry( "UseCustomIcons", false ) ) {
      EntityDisplayAttribute *attribute = mCurrentCollection.attribute<EntityDisplayAttribute>( Akonadi::Collection::AddIfMissing );
      //kDebug( KDE_DEFAULT_DEBUG_AREA )<<" NormalIconPath :"<<newGroup.readEntry( "NormalIconPath" )<<" UnreadIconPath :"<<newGroup.readEntry( "UnreadIconPath" );
      attribute->setIconName( newGroup.readEntry( "NormalIconPath" ) );
      attribute->setActiveIconName( newGroup.readEntry( "UnreadIconPath" ) );

      // collection modification needs to be sent to Akonadi
      mNeedModifyJob = true;
    }

    newGroup.deleteEntry( "UseCustomIcons" );
    newGroup.deleteEntry( "UnreadIconPath" );
    newGroup.deleteEntry( "NormalIconPath" );

    Akonadi::CollectionAnnotationsAttribute *annotationsAttribute = mCurrentCollection.attribute<Akonadi::CollectionAnnotationsAttribute>( Entity::AddIfMissing );
    QMap<QByteArray, QByteArray> annotations = annotationsAttribute->annotations();
    if ( newGroup.readEntry( "SharedSeenFlags", false ) ) {
      annotations[ KOLAB_SHAREDSEEN ] = "true";
      mNeedModifyJob = true;
    }
    newGroup.deleteEntry( "SharedSeenFlags" );
    if ( newGroup.hasKey( "IncidencesFor" ) ) {
      const QString incidenceFor = newGroup.readEntry( "IncidencesFor" );
      //kDebug( KDE_DEFAULT_DEBUG_AREA ) << "IncidencesFor=" << incidenceFor;
      if ( incidenceFor == "nobody" ) {
        annotations[ KOLAB_INCIDENCESFOR ] = "nobody";
      } else if ( incidenceFor == "admins" ) {
        annotations[ KOLAB_INCIDENCESFOR ] = "admins";
      } else if ( incidenceFor == "readers" ) {
        annotations[ KOLAB_INCIDENCESFOR ] = "readers";
      } else {
        annotations[ KOLAB_INCIDENCESFOR ] = "admins"; //Default
      }
      mNeedModifyJob = true;
      newGroup.deleteEntry( "IncidencesFor" );
    }
    if ( newGroup.hasKey( "Annotation-FolderType" ) ) {
      const QString annotationFolderType = newGroup.readEntry( "Annotation-FolderType" );
      EntityDisplayAttribute *attribute = mCurrentCollection.attribute<EntityDisplayAttribute>( Akonadi::Collection::AddIfMissing );
      if ( annotationFolderType == "mail" ) {
        //????
      } else if ( annotationFolderType == "event" ) {
        annotations[ KOLAB_FOLDERTYPE ] = "event";
        attribute->setIconName("view-calendar");
      } else if ( annotationFolderType == "task" ) {
        annotations[ KOLAB_FOLDERTYPE ] = "task";
        attribute->setIconName("view-pim-tasks");
      } else if ( annotationFolderType == "contact" ) {
        annotations[ KOLAB_FOLDERTYPE ] = "contact";
        attribute->setIconName("view-pim-contacts");
      } else if ( annotationFolderType == "note" ) {
        annotations[ KOLAB_FOLDERTYPE ] = "note";
        attribute->setIconName("view-pim-notes");
      } else if ( annotationFolderType == "journal" ) {
        annotations[ KOLAB_FOLDERTYPE ] = "journal";
        attribute->setIconName("view-pim-journal");
      } else {
        //????
      }
      mNeedModifyJob = true;
      newGroup.deleteEntry( "Annotation-FolderType" );
    }
    const QString whofield = newGroup.readEntry( "WhoField" );
    if ( !whofield.isEmpty() ) {
      Akonadi::MessageFolderAttribute *messageFolder  = mCurrentCollection.attribute<Akonadi::MessageFolderAttribute>( Akonadi::Entity::AddIfMissing );

      if ( whofield ==  QLatin1String( "To" ) )
        messageFolder->setOutboundFolder( true );
      else if( whofield == QLatin1String( "From" ) )
        messageFolder->setOutboundFolder( false );
      mNeedModifyJob = true;
    }
    newGroup.deleteEntry( "WhoField" );

    //Delete old entry
    newGroup.deleteEntry( "TotalMsgs" );
    newGroup.deleteEntry( "FolderSize" );
    newGroup.deleteEntry( "UnreadMsgs" );
    newGroup.deleteEntry( "Compactable" );
    newGroup.deleteEntry( "ContentsType" );
    newGroup.deleteEntry( "NoContent" );
    newGroup.deleteEntry( "ReadOnly" );
    newGroup.deleteEntry( "UploadAllFlags" );
    newGroup.deleteEntry( "PermanentFlags" );
    newGroup.deleteEntry( "StorageQuotaUsage" );
    newGroup.deleteEntry( "StorageQuotaLimit" );
    newGroup.deleteEntry( "StorageQuotaRoot" );
    newGroup.deleteEntry( "FolderAttributes" );
    newGroup.deleteEntry( "AlarmsBlocked" );
    newGroup.deleteEntry( "IncidencesForChanged" );
    newGroup.deleteEntry( "UserRights" );
    newGroup.deleteEntry( "StatusChangedLocally" );

    newGroup.deleteEntry( "MainFolderViewItemDnDSortingKey" );
    newGroup.deleteEntry( "MainFolderViewItemIsExpanded" );
    newGroup.deleteEntry( "MainFolderViewItemIsHidden" );
    newGroup.deleteEntry( "MainFolderViewItemIsSelected" );

    newGroup.deleteEntry( "Annotation-FolderType" );
    newGroup.deleteEntry( "AnnotationFolderTypeChanged" );
    newGroup.deleteEntry( "SystemLabel" );
    newGroup.deleteEntry( "ImapPath" );

    //Migrate favorite folder
    if ( newGroup.hasKey( "Id" ) ) {
      uint value = newGroup.readEntry( "Id", 0 );
      bool favoriteCollectionsMigrated = mKMailConfig->hasGroup( "FavoriteCollections" );

      KConfigGroup newFavoriteGroup( mKMailConfig, "FavoriteCollections" );
      if ( mKMailConfig->hasGroup( "FavoriteFolderView" ) && !favoriteCollectionsMigrated ) {
        KConfigGroup oldFavoriteGroup( mKMailConfig, "FavoriteFolderView" );
        const QList<int> lIds = oldFavoriteGroup.readEntry( "FavoriteFolderIds", QList<int>() );
        const QStringList lNames = oldFavoriteGroup.readEntry( "FavoriteFolderNames", QStringList() );
        oldFavoriteGroup.copyTo( &newFavoriteGroup );
        oldFavoriteGroup.deleteGroup();

        //kDebug( KDE_DEFAULT_DEBUG_AREA ) << "FAVORITE COLLECTION lIds=" << lIds<<" lNames="<<lNames;
        newFavoriteGroup.writeEntry( "FavoriteCollectionIds", lIds );
        newFavoriteGroup.writeEntry( "FavoriteCollectionLabels", lNames );

        newFavoriteGroup.deleteEntry( "FavoriteFolderNames" );
        newFavoriteGroup.deleteEntry( "FavoriteFolderIds" );

        newFavoriteGroup.deleteEntry( "IconSize" );
        newFavoriteGroup.deleteEntry( "SortingPolicy" );
        newFavoriteGroup.deleteEntry( "ToolTipDisplayPolicy" );
        newFavoriteGroup.deleteEntry( "FavoriteFolderViewSeenInboxes" );

        KConfigGroup favoriteCollectionViewGroup( mKMailConfig, "FavoriteCollectionView" );
        if ( newFavoriteGroup.hasKey( "FavoriteFolderViewHeight" ) ) {
          const int value = newFavoriteGroup.readEntry( "FavoriteFolderViewHeight", 100 );
          favoriteCollectionViewGroup.writeEntry( "FavoriteCollectionViewHeight", value );
        }

        if ( newFavoriteGroup.hasKey( "EnableFavoriteFolderView" ) ) {
          const bool value = newFavoriteGroup.readEntry( "EnableFavoriteFolderView", true );
          favoriteCollectionViewGroup.writeEntry( "EnableFavoriteCollectionView", value );
        }
      }

      if ( newFavoriteGroup.hasKey( "FavoriteCollectionIds" ) ) {
        QList<qint64> lIds = newFavoriteGroup.readEntry( "FavoriteCollectionIds", QList<qint64>() );
        if ( lIds.contains( value ) ) {
          const int pos = lIds.indexOf( value );
          lIds.replace( pos, mCurrentCollection.id() );
          newFavoriteGroup.writeEntry( "FavoriteCollectionIds", lIds );
        }
      }
      newGroup.deleteEntry( "Id" );
    }
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

  // Check kcmkmailsummaryrc General/ActiveFolders
  KConfigGroup kcmkmailsummary( mKcmKmailSummaryConfig, "General" );
  if ( kcmkmailsummary.hasKey( "ActiveFolders" ) ) {
    if ( !kcmkmailsummary.hasKey( "Role_CheckState" ) ) {
      kcmkmailsummary.writeEntry( "Role_CheckState", kcmkmailsummary.readEntry( "ActiveFolders", QStringList() ) );
      kcmkmailsummary.sync();
    }

    QStringList lstCollection = kcmkmailsummary.readEntry( "Role_CheckState", QStringList() );
    QString visualPath( mCurrentFolderId );
    visualPath.replace( ".directory", "" );
    visualPath.replace( "/.", "/" );
    if ( !visualPath.isEmpty() && ( visualPath.at( 0 ) == '.' ) )
      visualPath.remove( 0, 1 ); //remove first "."

    const QString localFolderPattern = QLatin1String( "/Local/%1" );
    const QString imapFolderPattern = QLatin1String( "/%1" );
    const QString newCollectionPattern = QLatin1String( "c%1" );
    if ( lstCollection.contains( localFolderPattern.arg( visualPath ) ) ) {
      const int pos = lstCollection.indexOf( localFolderPattern.arg( visualPath ) );
      lstCollection.replace( pos, newCollectionPattern.arg( mCurrentCollection.id() ) );
      lstCollection.insert( pos+1, "2" );
      kcmkmailsummary.writeEntry( "Role_CheckState", lstCollection );
    } else if ( lstCollection.contains( imapFolderPattern.arg( visualPath ) ) ) {
      const int pos = lstCollection.indexOf( imapFolderPattern.arg( visualPath ) );
      lstCollection.replace( pos, newCollectionPattern.arg( mCurrentCollection.id() ) );
      lstCollection.insert( pos+1, "2" );

      kcmkmailsummary.writeEntry( "Role_CheckState", lstCollection );
    }
  }

  // Check Composer/previous-fcc
  KConfigGroup composer( mKMailConfig, "Composer" );
  if ( composer.readEntry( "previous-fcc" ) == mCurrentFolderId )
    composer.writeEntry( "previous-fcc", mCurrentCollection.id() );

  // Check FolderSelectionDialog/LastSelectedFolder
  KConfigGroup folderSelection( mKMailConfig, "FolderSelectionDialog" );
  if ( folderSelection.readEntry( "LastSelectedFolder", mCurrentFolderId ) == mCurrentFolderId )
    folderSelection.writeEntry( "LastSelectedFolder", mCurrentCollection.id() );
  folderSelection.deleteEntry( "TreeWidgetLayout" );


  // Check General/startupFolder
  KConfigGroup general( mKMailConfig, "General" );
  if ( general.readEntry( "startupFolder" ) == mCurrentFolderId )
    general.writeEntry( "startupFolder", mCurrentCollection.id() );
  general.deleteEntry( "default-mailbox-format" );

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
    KConfigGroup accountGroup( mKMailConfig, groupName );

    if ( accountGroup.readEntry( "Folder" ) == mCurrentFolderId ){
      accountGroup.writeEntry( "Folder", mCurrentCollection.id() );
    }

    if ( accountGroup.readEntry( "trash" ) == mCurrentFolderId ) {
      accountGroup.writeEntry( "trash" , mCurrentCollection.id() );

      // this is configured as a trash collection, so make sure it gets the appropriate attribute
      // unless there is already a trash collection for this resource
      // (unfortunately special collections enforce uniqueness per resource)
      if ( !SpecialMailCollections::self()->hasCollection( SpecialMailCollections::Trash, mResource ) ) {
        q->registerAsSpecialCollection( SpecialMailCollections::Trash );
      }
    }
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

  // check MessageListView::StorageModelAggregations
  KConfigGroup storageModelAggregationGroup( mKMailConfig, QLatin1String( "MessageListView::StorageModelAggregations" ) );
  const QString setForStorageAggregationModelPattern = QLatin1String( "SetForStorageModel%1" );
  if ( storageModelAggregationGroup.hasKey(setForStorageAggregationModelPattern.arg( mCurrentFolderId ) ) ) {
    const QString value = storageModelAggregationGroup.readEntry( setForStorageAggregationModelPattern.arg( mCurrentFolderId ) );
    storageModelAggregationGroup.writeEntry( setForStorageAggregationModelPattern.arg( mCurrentCollection.id() ),value );
    storageModelAggregationGroup.deleteEntry( setForStorageAggregationModelPattern.arg( mCurrentFolderId ) );
  }

  // check MessageListView::StorageModelThemes
  KConfigGroup storageModelThemesGroup( mKMailConfig, QLatin1String( "MessageListView::StorageModelThemes" ) );
  const QString setForStorageModelPattern = QLatin1String( "SetForStorageModel%1" );
  if ( storageModelThemesGroup.hasKey(setForStorageModelPattern.arg( mCurrentFolderId ) ) ) {
    const QString value = storageModelThemesGroup.readEntry( setForStorageModelPattern.arg( mCurrentFolderId ) );
    storageModelThemesGroup.writeEntry( setForStorageModelPattern.arg( mCurrentCollection.id() ),value );
    storageModelThemesGroup.deleteEntry( setForStorageModelPattern.arg( mCurrentFolderId ) );
  }

  // check MessageListView::StorageModelSortOrder
  KConfigGroup sortOrderGroup( mKMailConfig, QLatin1String( "MessageListView::StorageModelSortOrder" ) );
  const QString groupSortDirectionPattern = QLatin1String( "%1GroupSortDirection" );
  const QString groupSortingPattern = QLatin1String( "%1GroupSorting" );
  const QString messageSortDirectionPattern = QLatin1String( "%1MessageSortDirection" );
  const QString messageSortingPattern = QLatin1String( "%1MessageSorting" );

  if ( sortOrderGroup.hasKey( groupSortDirectionPattern.arg( mCurrentFolderId ) ) ) {
    const QString value = sortOrderGroup.readEntry( groupSortDirectionPattern.arg( mCurrentFolderId ) );
    sortOrderGroup.writeEntry( groupSortDirectionPattern.arg( mCurrentCollection.id()), value );
    sortOrderGroup.deleteEntry( groupSortDirectionPattern.arg( mCurrentFolderId ) );
  }

  if ( sortOrderGroup.hasKey( groupSortingPattern.arg( mCurrentFolderId ) ) ) {
    const QString value = sortOrderGroup.readEntry( groupSortingPattern.arg( mCurrentFolderId ) );
    sortOrderGroup.writeEntry( groupSortingPattern.arg( mCurrentCollection.id()), value );
    sortOrderGroup.deleteEntry( groupSortingPattern.arg( mCurrentFolderId ) );
  }

  if ( sortOrderGroup.hasKey( messageSortingPattern.arg( mCurrentFolderId ) ) ) {
    const QString value = sortOrderGroup.readEntry( messageSortingPattern.arg( mCurrentFolderId ) );
    sortOrderGroup.writeEntry( messageSortingPattern.arg( mCurrentCollection.id() ), value );
    sortOrderGroup.deleteEntry( messageSortingPattern.arg( mCurrentFolderId ) );
  }

  if ( sortOrderGroup.hasKey( messageSortDirectionPattern.arg( mCurrentFolderId ) ) ) {
    const QString value = sortOrderGroup.readEntry( messageSortDirectionPattern.arg( mCurrentFolderId ) );
    sortOrderGroup.writeEntry( messageSortDirectionPattern.arg( mCurrentCollection.id() ), value );
    sortOrderGroup.deleteEntry( messageSortDirectionPattern.arg( mCurrentFolderId ) );
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
    KConfigGroup newGroup( mTemplatesConfig, templatesGroupPattern.arg( mCurrentCollection.id() ) );

    oldGroup.copyTo( &newGroup );
    oldGroup.deleteGroup();
  }

}

void AbstractCollectionMigrator::Private::collectionDone()
{
  mCurrentFolderId = QString();
  mCurrentCollection = Collection();

  mStatus = Private::Scheduling;
  QMetaObject::invokeMethod( q, "processNextCollection", Qt::QueuedConnection );

  q->migrationProgress( mProcessedCollectionsCount, mCollectionsById.count() );
}

void AbstractCollectionMigrator::Private::collectionAdded( const Collection &collection )
{
  if ( mStatus == Waiting ) {
    mRecheckTimer.stop();
    mRecheckTimer.disconnect();
    mStatus = Idle;
  }

  // don't wait any longer, start explicit fetch right away
  if ( mExplicitFetchStatus == Waiting ) {
    mRecheckTimer.stop();
    mRecheckTimer.disconnect();
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

void AbstractCollectionMigrator::Private::modifyResult( KJob *job )
{
  if ( job->error() ) {
    kError() << job->error();
  }

  collectionDone();
}

void AbstractCollectionMigrator::Private::processNextCollection()
{
  if ( mStatus == Failed || mStatus == Finished ) {
    return;
  }

  if ( mCollectionQueue.isEmpty() ) {
    mStatus = Idle;

    if ( mExplicitFetchStatus == Finished ) {
      processingDone();
      return;
    }

    if ( mExplicitFetchStatus == Idle ) {
      // TODO should explicitly create fetch job instead of tricking recheckIdleResource()
      // into doing it
      mExplicitFetchStatus = Waiting;
      recheckIdleResource();
      return;
    }

    if ( mResource.status() == AgentInstance::Idle && mExplicitFetchStatus != Running ) {
      processingDone();
      return;
    }
  } else {
    mStatus = Running;
    ++mProcessedCollectionsCount;

    const Collection collection = mCollectionQueue.dequeue();

    mCurrentFolderId = folderIdentifierForCollection( collection );
    mCurrentCollection = collection;
    mNeedModifyJob = false;

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

  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "resource=" << mResource.identifier()
           << "oldStatus=" << oldStatus << "message=" << oldMessage
           << "newStatus" << mResource.status() << "message=" << mResource.statusMessage();

  if ( oldStatus == AgentInstance::Broken && mResource.status() == AgentInstance::Broken ) {
    if ( oldMessage != mResource.statusMessage() ) {
      // changing to a new broken state. if still waiting, wait for at most 10 seconds before
      // giving up
      if ( mStatus == Waiting ) {
        kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Restaring recheck timer for last 10 seconds";
        mRecheckTimer.start( 10000 );
      }
    }
  }

  if ( mStatus == Waiting && mResource.status() != AgentInstance::Broken ) {
    mRecheckTimer.stop();
    mRecheckTimer.disconnect();
    mStatus = Idle;
  }

  if ( oldStatus != AgentInstance::Idle && mResource.status() == AgentInstance::Idle && mExplicitFetchStatus == Idle ) {
    mExplicitFetchStatus = Waiting;

    // if resource is now "Idle" it might still need time to process until it becomes ready
    // unfortunately this is not a separate state so lets delay the explicit fetch
    // wait for at most one minute
    mRecheckTimer.stop();
    mRecheckTimer.disconnect();
    QObject::connect( &mRecheckTimer, SIGNAL( timeout() ), q, SLOT( recheckIdleResource() ) );
    mRecheckTimer.start( 60000 );
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

AbstractCollectionMigrator::AbstractCollectionMigrator( const AgentInstance &resource, MixedMaildirStore *store, QObject *parent )
  : QObject( parent ), d( new Private( this, resource, store ) )
{
  Q_ASSERT( store != 0 );

  d->mHiddenSession = new Session( resource.identifier().toAscii() );

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
    connect( &(d->mRecheckTimer), SIGNAL( timeout() ), SLOT( recheckIdleResource() ) );
    d->mRecheckTimer.start( 60000 );
  } else if ( d->mResource.status() == AgentInstance::Broken ) {
    // if resource is "Broken", it could still become idle after fully processing its new config
    // wait for at most one minute
    d->mStatus = Private::Waiting;
    connect( &(d->mRecheckTimer), SIGNAL( timeout() ), SLOT( recheckBrokenResource() ) );
    d->mRecheckTimer.start( 60000 );
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

void AbstractCollectionMigrator::setKMailConfig( const KSharedConfigPtr &config )
{
  d->mKMailConfig = config;
}

void AbstractCollectionMigrator::setEmailIdentityConfig( const KSharedConfigPtr &config )
{
  d->mEmailIdentityConfig = config;
}

void AbstractCollectionMigrator::setKcmKmailSummaryConfig( const KSharedConfigPtr& config )
{
  d->mKcmKmailSummaryConfig = config;
}

void AbstractCollectionMigrator::setTemplatesConfig( const KSharedConfigPtr& config )
{
  d->mTemplatesConfig = config;
}

void AbstractCollectionMigrator::migrationProgress( int processedCollections, int seenCollections )
{
  emit progress( 0, seenCollections, processedCollections );
}

void AbstractCollectionMigrator::collectionProcessed()
{
  d->migrateConfig();

  if ( d->mNeedModifyJob ) {
    CollectionModifyJob *job = new CollectionModifyJob( d->mCurrentCollection );
    connect( job, SIGNAL( result( KJob*) ), SLOT( modifyResult( KJob* ) ) );
  } else {
    d->collectionDone();
  }
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

KSharedConfigPtr AbstractCollectionMigrator::kmailConfig() const
{
  return d->mKMailConfig;
}

KSharedConfigPtr AbstractCollectionMigrator::emailIdentityConfig() const
{
  return d->mEmailIdentityConfig;
}

void AbstractCollectionMigrator::registerAsSpecialCollection( int type )
{
  Q_ASSERT( type > SpecialMailCollections::Invalid  && type < SpecialMailCollections::LastType );

  SpecialMailCollections::Type colType = (SpecialMailCollections::Type)type;

  kDebug( KDE_DEFAULT_DEBUG_AREA ) << "Registering collection" << d->mCurrentCollection.name() << "for type" << type;
  SpecialMailCollections::self()->registerCollection( colType, d->mCurrentCollection );

  if ( Private::mIconNamesBySpecialType.isEmpty() )
  {
    Private::mIconNamesBySpecialType.insert( SpecialMailCollections::Root,
                                             QLatin1String( "folder" ) );
    Private::mIconNamesBySpecialType.insert( SpecialMailCollections::Inbox,
                                             QLatin1String( "mail-folder-inbox" ) );
    Private::mIconNamesBySpecialType.insert( SpecialMailCollections::Outbox,
                                             QLatin1String( "mail-folder-outbox" ) );
    Private::mIconNamesBySpecialType.insert( SpecialMailCollections::SentMail,
                                             QLatin1String( "mail-folder-sent" ) );
    Private::mIconNamesBySpecialType.insert( SpecialMailCollections::Trash,
                                             QLatin1String( "user-trash" ) );
    Private::mIconNamesBySpecialType.insert( SpecialMailCollections::Drafts,
                                             QLatin1String( "document-properties" ) );
    Private::mIconNamesBySpecialType.insert( SpecialMailCollections::Templates,
                                             QLatin1String( "document-new" ) );
  }

  IconNameHash::const_iterator findIt = Private::mIconNamesBySpecialType.constFind( colType );
  if ( findIt != Private::mIconNamesBySpecialType.constEnd() ) {
    EntityDisplayAttribute *attribute = d->mCurrentCollection.attribute<EntityDisplayAttribute>( Entity::AddIfMissing );
    Q_ASSERT( attribute != 0 );

    attribute->setIconName( findIt.value() );
    d->mNeedModifyJob = true;
  }
}

MixedMaildirStore *AbstractCollectionMigrator::store()
{
  return d->mStore;
}

Session *AbstractCollectionMigrator::hiddenSession()
{
  return d->mHiddenSession;
}

#include "abstractcollectionmigrator.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
