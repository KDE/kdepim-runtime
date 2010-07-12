/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "openxchangeresource.h"

#include "configdialog.h"
#include "settingsadaptor.h"

#include <akonadi/cachepolicy.h>
#include <akonadi/changerecorder.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/kcal/incidencemimetypevisitor.h>

#include <kabc/addressee.h>
#include <kcal/event.h>
#include <kcal/todo.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include <oxa/davmanager.h>
#include <oxa/foldercreatejob.h>
#include <oxa/folderdeletejob.h>
#include <oxa/foldermodifyjob.h>
#include <oxa/foldermovejob.h>
#include <oxa/foldersrequestdeltajob.h>
#include <oxa/foldersrequestjob.h>
#include <oxa/objectcreatejob.h>
#include <oxa/objectdeletejob.h>
#include <oxa/objectmodifyjob.h>
#include <oxa/objectmovejob.h>
#include <oxa/objectrequestjob.h>
#include <oxa/objectsrequestdeltajob.h>
#include <oxa/objectsrequestjob.h>
#include <oxa/updateusersjob.h>
#include <oxa/users.h>

using namespace Akonadi;

class RemoteInformation
{
  public:
    RemoteInformation( qlonglong objectId, OXA::Folder::Module module, const QString &lastModified )
      : mObjectId( objectId ),
        mModule( module ),
        mLastModified( lastModified )
    {
    }

    inline qlonglong objectId() const
    {
      return mObjectId;
    }

    inline OXA::Folder::Module module() const
    {
      return mModule;
    }

    inline QString lastModified() const
    {
      return mLastModified;
    }

    inline void setLastModified( const QString &lastModified )
    {
      mLastModified = lastModified;
    }

    static RemoteInformation load( const Entity &entity )
    {
      const QStringList parts = entity.remoteRevision().split( QLatin1Char( ':' ), QString::KeepEmptyParts );

      OXA::Folder::Module module = OXA::Folder::Unbound;

      if ( parts.count() > 0 ) {
        if ( parts.at( 0 ) == QLatin1String( "calendar" ) )
          module = OXA::Folder::Calendar;
        else if ( parts.at( 0 ) == QLatin1String( "contacts" ) )
          module = OXA::Folder::Contacts;
        else if ( parts.at( 0 ) == QLatin1String( "tasks" ) )
          module = OXA::Folder::Tasks;
        else
          module = OXA::Folder::Unbound;
      }

      QString lastModified = QLatin1String( "0" );
      if ( parts.count() > 1 )
        lastModified = parts.at( 1 );

      return RemoteInformation( entity.remoteId().toLongLong(), module, lastModified );
    }

    void store( Entity &entity ) const
    {
      QString module;
      switch ( mModule ) {
        case OXA::Folder::Calendar: module = QLatin1String( "calendar" ); break;
        case OXA::Folder::Contacts: module = QLatin1String( "contacts" ); break;
        case OXA::Folder::Tasks: module = QLatin1String( "tasks" ); break;
        case OXA::Folder::Unbound: break;
      }

      QStringList parts;
      parts.append( module );
      parts.append( mLastModified );

      entity.setRemoteId( QString::number( mObjectId ) );
      entity.setRemoteRevision( parts.join( QLatin1String( ":" ) ) );
    }

  private:
    qlonglong mObjectId;
    OXA::Folder::Module mModule;
    QString mLastModified;
};

class ObjectsLastSync
{
  public:
    ObjectsLastSync()
    {
      if ( !Settings::self()->objectsLastSync().isEmpty() ) {
        const QStringList pairs = Settings::self()->objectsLastSync().split( QLatin1Char( ':' ), QString::KeepEmptyParts );
        foreach ( const QString &pair, pairs ) {
          const QStringList entry = pair.split( QLatin1Char( '=' ), QString::KeepEmptyParts );
          mObjectsMap.insert( entry.at( 0 ).toLongLong(), entry.at( 1 ).toULongLong() );
        }
      }
    }

    void save()
    {
      QStringList pairs;

      QMapIterator<qlonglong, qulonglong> it( mObjectsMap );
      while ( it.hasNext() ) {
        it.next();
        pairs.append( QString::number( it.key() ) + QLatin1Char( '=' ) + QString::number( it.value() ) );
      }

      Settings::self()->setObjectsLastSync( pairs.join( QLatin1String( ":" ) ) );
      Settings::self()->writeConfig();
    }

    qulonglong lastSync( qlonglong collectionId ) const
    {
      if ( !mObjectsMap.contains( collectionId ) )
        return 0;

      return mObjectsMap.value( collectionId );
    }

    void setLastSync( qlonglong collectionId, qulonglong timeStamp )
    {
      mObjectsMap.insert( collectionId, timeStamp );
    }

  private:
    QMap<qlonglong, qulonglong> mObjectsMap;
};

static Collection::Rights folderPermissionsToCollectionRights( const OXA::Folder &folder )
{
  const OXA::Folder::UserPermissions userPermissions = folder.userPermissions();

  if ( !userPermissions.contains( OXA::Users::self()->currentUserId() ) ) {
    // There are no rights given for us explicitly, so it is read-only
    return Collection::ReadOnly;
  } else {
    const OXA::Folder::Permissions permissions = userPermissions.value( OXA::Users::self()->currentUserId() );
    Collection::Rights rights = Collection::ReadOnly;
    switch ( permissions.folderPermission() ) {
      case OXA::Folder::Permissions::FolderIsVisible: rights |= Collection::ReadOnly; break;
      case OXA::Folder::Permissions::CreateObjects: rights |= Collection::CanCreateItem; break;
      case OXA::Folder::Permissions::CreateSubfolders: // fallthrough
      case OXA::Folder::Permissions::AdminPermission: rights |= (Collection::CanCreateItem | Collection::CanCreateCollection); break;
      default: break;
    }

    if ( permissions.objectWritePermission() != OXA::Folder::Permissions::NoWritePermission ) {
      rights |= Collection::CanChangeItem;
      rights |= Collection::CanChangeCollection;
    }

    if ( permissions.objectDeletePermission() != OXA::Folder::Permissions::NoDeletePermission ) {
      rights |= Collection::CanDeleteItem;
      rights |= Collection::CanDeleteCollection;
    }

    return rights;
  }
}

OpenXchangeResource::OpenXchangeResource( const QString &id )
  : ResourceBase( id )
{
  // setup the resource
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

  changeRecorder()->fetchCollection( true );
  changeRecorder()->itemFetchScope().fetchFullPayload( true );
  changeRecorder()->itemFetchScope().setAncestorRetrieval( ItemFetchScope::Parent );
  changeRecorder()->collectionFetchScope().setAncestorRetrieval( CollectionFetchScope::Parent );

  setName( i18n( "Open-Xchange" ) );

  OXA::Users::self()->init( identifier() );

  KUrl baseUrl = Settings::self()->baseUrl();
  baseUrl.setUserName( Settings::self()->username() );
  baseUrl.setPassword( Settings::self()->password() );
  OXA::DavManager::self()->setBaseUrl( baseUrl );

  // Create the standard collections.
  //
  // There exists special OX folders (e.g. private, public, shared) that are not
  // returned by a normal webdav listing, therefor we create them manually here.
  // This is possible because the remote ids of these folders are fixed values from 1
  // till 4.
  mResourceCollection.setParentCollection( Collection::root() );
  const RemoteInformation resourceInformation( 0, OXA::Folder::Unbound, QString() );
  resourceInformation.store( mResourceCollection );
  mResourceCollection.setName( name() );
  mResourceCollection.setContentMimeTypes( QStringList() << Collection::mimeType() );
  mResourceCollection.setRights( Collection::ReadOnly );
  EntityDisplayAttribute *attribute = mResourceCollection.attribute<EntityDisplayAttribute>( Collection::AddIfMissing );
  attribute->setIconName( QLatin1String( "ox" ) );

  Collection privateFolder;
  privateFolder.setParentCollection( mResourceCollection );
  const RemoteInformation privateFolderInformation( 1, OXA::Folder::Unbound, QString() );
  privateFolderInformation.store( privateFolder );
  privateFolder.setName( i18n( "Private Folder" ) );
  privateFolder.setContentMimeTypes( QStringList() << Collection::mimeType() );
  privateFolder.setRights( Collection::ReadOnly );

  Collection publicFolder;
  publicFolder.setParentCollection( mResourceCollection );
  const RemoteInformation publicFolderInformation( 2, OXA::Folder::Unbound, QString() );
  publicFolderInformation.store( publicFolder );
  publicFolder.setName( i18n( "Public Folder" ) );
  publicFolder.setContentMimeTypes( QStringList() << Collection::mimeType() );
  publicFolder.setRights( Collection::ReadOnly );

  Collection sharedFolder;
  sharedFolder.setParentCollection( mResourceCollection );
  const RemoteInformation sharedFolderInformation( 3, OXA::Folder::Unbound, QString() );
  sharedFolderInformation.store( sharedFolder );
  sharedFolder.setName( i18n( "Shared Folder" ) );
  sharedFolder.setContentMimeTypes( QStringList() << Collection::mimeType() );
  sharedFolder.setRights( Collection::ReadOnly );

  Collection systemFolder;
  systemFolder.setParentCollection( mResourceCollection );
  const RemoteInformation systemFolderInformation( 4, OXA::Folder::Unbound, QString() );
  systemFolderInformation.store( systemFolder );
  systemFolder.setName( i18n( "System Folder" ) );
  systemFolder.setContentMimeTypes( QStringList() << Collection::mimeType() );
  systemFolder.setRights( Collection::ReadOnly );

  // TODO: set cache policy depending on sync behaviour
  Akonadi::CachePolicy cachePolicy;
  cachePolicy.setInheritFromParent( false );
  cachePolicy.setSyncOnDemand( false );
  cachePolicy.setCacheTimeout( -1 );
  cachePolicy.setIntervalCheckTime( 5 );
  mResourceCollection.setCachePolicy( cachePolicy );

  mStandardCollectionsMap.insert( 0, mResourceCollection );
  mStandardCollectionsMap.insert( 1, privateFolder );
  mStandardCollectionsMap.insert( 2, publicFolder );
  mStandardCollectionsMap.insert( 3, sharedFolder );
  mStandardCollectionsMap.insert( 4, systemFolder );

  mCollectionsMap = mStandardCollectionsMap;

  if ( Settings::self()->useIncrementalUpdates() )
    syncCollectionsRemoteIdCache();
}

OpenXchangeResource::~OpenXchangeResource()
{
}

void OpenXchangeResource::cleanup()
{
  // be nice and remove cache file when resource is removed
  QFile::remove( OXA::Users::self()->cacheFilePath() );

  QFile::remove( KStandardDirs::locateLocal( "config", Settings::self()->config()->name() ) );

  ResourceBase::cleanup();
}

void OpenXchangeResource::aboutToQuit()
{
}

void OpenXchangeResource::configure( WId windowId )
{
  const bool useIncrementalUpdates = Settings::self()->useIncrementalUpdates();

  ConfigDialog dlg( windowId );
  if ( dlg.exec() ) { //krazy:exclude=crashy

    // if the user has changed the incremental update settings we have to do
    // some additional initialization work
    if ( useIncrementalUpdates != Settings::self()->useIncrementalUpdates() ) {
      Settings::self()->setFoldersLastSync( 0 );
      Settings::self()->setObjectsLastSync( QString() );
    }

    Settings::self()->writeConfig();

    clearCache();

    KUrl baseUrl = Settings::self()->baseUrl();
    baseUrl.setUserName( Settings::self()->username() );
    baseUrl.setPassword( Settings::self()->password() );
    OXA::DavManager::self()->setBaseUrl( baseUrl );

    // To find out the correct ACLs we need the uid of the user that
    // logs in. For loading events and tasks we need a complete mapping of
    // user id to name as well, so the mapping must be loaded as well.
    // Both is done by UpdateUsersJob, so trigger it here before we continue
    // with synchronization in onUpdateUsersJobFinished.
    OXA::UpdateUsersJob *job = new OXA::UpdateUsersJob( this );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( onUpdateUsersJobFinished( KJob* ) ) );
    job->start();
  } else {
    emit configurationDialogRejected();
  }
}

void OpenXchangeResource::retrieveCollections()
{
  //qDebug("tokoe: retrieve collections called");
  if ( Settings::self()->useIncrementalUpdates() ) {
    //qDebug("lastSync=%llu", Settings::self()->foldersLastSync());
    OXA::FoldersRequestDeltaJob *job = new OXA::FoldersRequestDeltaJob( Settings::self()->foldersLastSync(), this );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( onFoldersRequestDeltaJobFinished( KJob* ) ) );
    job->start();
  } else {
    OXA::FoldersRequestJob *job = new OXA::FoldersRequestJob( 0, OXA::FoldersRequestJob::Modified, this );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( onFoldersRequestJobFinished( KJob* ) ) );
    job->start();
  }
}

void OpenXchangeResource::retrieveItems( const Akonadi::Collection &collection )
{
  //qDebug("tokoe: retrieveItems on %s called", qPrintable(collection.name()));
  const RemoteInformation remoteInformation = RemoteInformation::load( collection );

  OXA::Folder folder;
  folder.setObjectId( remoteInformation.objectId() );
  folder.setModule( remoteInformation.module() );

  if ( Settings::self()->useIncrementalUpdates() ) {
    ObjectsLastSync lastSyncInfo;
    //qDebug("lastSync=%llu", lastSyncInfo.lastSync( collection.id() ));
    OXA::ObjectsRequestDeltaJob *job = new OXA::ObjectsRequestDeltaJob( folder, lastSyncInfo.lastSync( collection.id() ), this );
    job->setProperty( "collection", QVariant::fromValue( collection ) );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( onObjectsRequestDeltaJobFinished( KJob* ) ) );
    job->start();
  } else {
    OXA::ObjectsRequestJob *job = new OXA::ObjectsRequestJob( folder, 0, OXA::ObjectsRequestJob::Modified, this );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( onObjectsRequestJobFinished( KJob* ) ) );
    job->start();
  }
}

bool OpenXchangeResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  //qDebug("tokoe: retrieveItem %lld called", item.id());
  const RemoteInformation remoteInformation = RemoteInformation::load( item );

  OXA::Object object;
  object.setObjectId( remoteInformation.objectId() );
  object.setModule( remoteInformation.module() );

  OXA::ObjectRequestJob *job = new OXA::ObjectRequestJob( object, this );
  job->setProperty( "item", QVariant::fromValue( item ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onObjectRequestJobFinished( KJob* ) ) );
  job->start();

  return true;
}

void OpenXchangeResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  const RemoteInformation remoteInformation = RemoteInformation::load( collection );

  OXA::Object object;
  object.setFolderId( remoteInformation.objectId() );
  object.setModule( remoteInformation.module() );

  if ( item.hasPayload<KABC::Addressee>() ) {
    object.setContact( item.payload<KABC::Addressee>() );
  } else if ( item.hasPayload<KABC::ContactGroup>() ) {
    object.setContactGroup( item.payload<KABC::ContactGroup>() );
  } else if ( item.hasPayload<KCal::Event::Ptr>() ) {
    object.setEvent( item.payload<KCal::Incidence::Ptr>() );
  } else if ( item.hasPayload<KCal::Todo::Ptr>() ) {
    object.setTask( item.payload<KCal::Incidence::Ptr>() );
  } else {
    Q_ASSERT( false );
  }

  OXA::ObjectCreateJob *job = new OXA::ObjectCreateJob( object, this );
  job->setProperty( "item", QVariant::fromValue( item ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onObjectCreateJobFinished( KJob* ) ) );
  job->start();
}

void OpenXchangeResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  const RemoteInformation remoteInformation = RemoteInformation::load( item );
  const RemoteInformation parentRemoteInformation = RemoteInformation::load( item.parentCollection() );

  OXA::Object object;
  object.setObjectId( remoteInformation.objectId() );
  object.setModule( remoteInformation.module() );
  object.setFolderId( parentRemoteInformation.objectId() );
  object.setLastModified( remoteInformation.lastModified() );

  if ( item.hasPayload<KABC::Addressee>() ) {
    object.setContact( item.payload<KABC::Addressee>() );
  } else if ( item.hasPayload<KABC::ContactGroup>() ) {
    object.setContactGroup( item.payload<KABC::ContactGroup>() );
  } else if ( item.hasPayload<KCal::Event::Ptr>() ) {
    object.setEvent( item.payload<KCal::Incidence::Ptr>() );
  } else if ( item.hasPayload<KCal::Todo::Ptr>() ) {
    object.setTask( item.payload<KCal::Incidence::Ptr>() );
  } else {
    Q_ASSERT( false );
  }

  OXA::ObjectModifyJob *job = new OXA::ObjectModifyJob( object, this );
  job->setProperty( "item", QVariant::fromValue( item ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onObjectModifyJobFinished( KJob* ) ) );
  job->start();
}

void OpenXchangeResource::itemRemoved( const Akonadi::Item &item )
{
  const RemoteInformation remoteInformation = RemoteInformation::load( item );
  const RemoteInformation parentRemoteInformation = RemoteInformation::load( item.parentCollection() );

  OXA::Object object;
  object.setObjectId( remoteInformation.objectId() );
  object.setFolderId( parentRemoteInformation.objectId() );
  object.setModule( remoteInformation.module() );
  object.setLastModified( remoteInformation.lastModified() );

  OXA::ObjectDeleteJob *job = new OXA::ObjectDeleteJob( object, this );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onObjectDeleteJobFinished( KJob* ) ) );

  job->start();
}

void OpenXchangeResource::itemMoved( const Akonadi::Item &item, const Akonadi::Collection &collectionSource,
                                     const Akonadi::Collection &collectionDestination )
{
  const RemoteInformation remoteInformation = RemoteInformation::load( item );
  const RemoteInformation parentRemoteInformation = RemoteInformation::load( collectionSource );
  const RemoteInformation newParentRemoteInformation = RemoteInformation::load( collectionDestination );

  OXA::Object object;
  object.setObjectId( remoteInformation.objectId() );
  object.setModule( remoteInformation.module() );
  object.setFolderId( parentRemoteInformation.objectId() );
  object.setLastModified( remoteInformation.lastModified() );

  OXA::Folder destinationFolder;
  destinationFolder.setObjectId( newParentRemoteInformation.objectId() );

  OXA::ObjectMoveJob *job = new OXA::ObjectMoveJob( object, destinationFolder, this );
  job->setProperty( "item", QVariant::fromValue( item ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onObjectMoveJobFinished( KJob* ) ) );

  job->start();
}

void OpenXchangeResource::collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent )
{
  const RemoteInformation parentRemoteInformation = RemoteInformation::load( parent );

  OXA::Folder folder;
  folder.setTitle( collection.name() );
  folder.setFolderId( parentRemoteInformation.objectId() );
  folder.setType( OXA::Folder::Private );

  // the folder 'inherits' the module type of its parent collection
  folder.setModule( parentRemoteInformation.module() );

  // fill permissions
  OXA::Folder::Permissions permissions;
  permissions.setFolderPermission( OXA::Folder::Permissions::CreateSubfolders );
  permissions.setObjectReadPermission( OXA::Folder::Permissions::ReadOwnObjects );
  permissions.setObjectWritePermission( OXA::Folder::Permissions::WriteOwnObjects );
  permissions.setObjectDeletePermission( OXA::Folder::Permissions::DeleteOwnObjects );
  permissions.setAdminFlag( true );

  // assign permissions to user
  OXA::Folder::UserPermissions userPermissions;
  userPermissions.insert( OXA::Users::self()->currentUserId(), permissions );

  // set user permissions of folder
  folder.setUserPermissions( userPermissions );

  OXA::FolderCreateJob *job = new OXA::FolderCreateJob( folder, this );
  job->setProperty( "collection", QVariant::fromValue( collection ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onFolderCreateJobFinished( KJob* ) ) );
  job->start();
}

void OpenXchangeResource::collectionChanged( const Akonadi::Collection &collection )
{
  const RemoteInformation remoteInformation = RemoteInformation::load( collection );

  // do not try to change the standard collections
  if ( remoteInformation.objectId() >= 0 && remoteInformation.objectId() <= 4 ) {
    changeCommitted( collection );
    return;
  }

  const RemoteInformation parentRemoteInformation = RemoteInformation::load( collection.parentCollection() );

  OXA::Folder folder;
  folder.setObjectId( remoteInformation.objectId() );
  folder.setFolderId( parentRemoteInformation.objectId() );
  folder.setTitle( collection.name() );
  folder.setLastModified( remoteInformation.lastModified() );

  OXA::FolderModifyJob *job = new OXA::FolderModifyJob( folder, this );
  job->setProperty( "collection", QVariant::fromValue( collection ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onFolderModifyJobFinished( KJob* ) ) );
}

void OpenXchangeResource::collectionRemoved( const Akonadi::Collection &collection )
{
  const RemoteInformation remoteInformation = RemoteInformation::load( collection );

  OXA::Folder folder;
  folder.setObjectId( remoteInformation.objectId() );
  folder.setLastModified( remoteInformation.lastModified() );

  OXA::FolderDeleteJob *job = new OXA::FolderDeleteJob( folder, this );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onFolderDeleteJobFinished( KJob* ) ) );

  job->start();
}

void OpenXchangeResource::collectionMoved( const Akonadi::Collection &collection, const Akonadi::Collection &collectionSource,
                                           const Akonadi::Collection &collectionDestination )
{
  const RemoteInformation remoteInformation = RemoteInformation::load( collection );
  const RemoteInformation parentRemoteInformation = RemoteInformation::load( collectionSource );
  const RemoteInformation newParentRemoteInformation = RemoteInformation::load( collectionDestination );

  OXA::Folder folder;
  folder.setObjectId( remoteInformation.objectId() );
  folder.setFolderId( parentRemoteInformation.objectId() );

  OXA::Folder destinationFolder;
  destinationFolder.setObjectId( newParentRemoteInformation.objectId() );

  OXA::FolderMoveJob *job = new OXA::FolderMoveJob( folder, destinationFolder, this );
  job->setProperty( "collection", QVariant::fromValue( collection ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onFolderMoveJobFinished( KJob* ) ) );

  job->start();
}

//// job result slots

void OpenXchangeResource::onUpdateUsersJobFinished( KJob *job )
{
  if ( job->error() ) {
    // This might be an indication that we can not connect to the server...
    emit status( Broken, i18n( "Unable to connect to server" ) );
    return;
  }

  if ( Settings::self()->useIncrementalUpdates() )
    syncCollectionsRemoteIdCache();

  // now we have all user information, so continue synchronization
  synchronize();
  emit configurationDialogAccepted();
}

void OpenXchangeResource::onObjectsRequestJobFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }

  OXA::ObjectsRequestJob *requestJob = qobject_cast<OXA::ObjectsRequestJob*>( job );
  Q_ASSERT( requestJob );

  Item::List items;

  const OXA::Object::List objects = requestJob->objects();
  foreach ( const OXA::Object &object, objects ) {
    Item item;
    switch ( object.module() ) {
      case OXA::Folder::Contacts:
        if ( !object.contact().isEmpty() ) {
          item.setMimeType( KABC::Addressee::mimeType() );
          item.setPayload<KABC::Addressee>( object.contact() );
        } else {
          item.setMimeType( KABC::ContactGroup::mimeType() );
          item.setPayload<KABC::ContactGroup>( object.contactGroup() );
        }
        break;
      case OXA::Folder::Calendar:
        item.setMimeType( IncidenceMimeTypeVisitor::eventMimeType() );
        item.setPayload<KCal::Incidence::Ptr>( object.event() );
        break;
      case OXA::Folder::Tasks:
        item.setMimeType( IncidenceMimeTypeVisitor::todoMimeType() );
        item.setPayload<KCal::Incidence::Ptr>( object.task() );
        break;
      case OXA::Folder::Unbound:
        Q_ASSERT( false );
        break;
    }
    const RemoteInformation remoteInformation( object.objectId(), object.module(), object.lastModified() );
    remoteInformation.store( item );

    items.append( item );
  }

  itemsRetrieved( items );
}

void OpenXchangeResource::onObjectsRequestDeltaJobFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }

  OXA::ObjectsRequestDeltaJob *requestJob = qobject_cast<OXA::ObjectsRequestDeltaJob*>( job );
  Q_ASSERT( requestJob );

  const Akonadi::Collection collection = requestJob->property( "collection" ).value<Collection>();

  ObjectsLastSync lastSyncInfo;

  qulonglong objectsLastSync = lastSyncInfo.lastSync( collection.id() );

  Item::List changedItems;

  const OXA::Object::List modifiedObjects = requestJob->modifiedObjects();
  foreach ( const OXA::Object &object, modifiedObjects ) {
    Item item;
    switch ( object.module() ) {
      case OXA::Folder::Contacts:
        if ( !object.contact().isEmpty() ) {
          item.setMimeType( KABC::Addressee::mimeType() );
          item.setPayload<KABC::Addressee>( object.contact() );
        } else {
          item.setMimeType( KABC::ContactGroup::mimeType() );
          item.setPayload<KABC::ContactGroup>( object.contactGroup() );
        }
        break;
      case OXA::Folder::Calendar:
        item.setMimeType( IncidenceMimeTypeVisitor::eventMimeType() );
        item.setPayload<KCal::Incidence::Ptr>( object.event() );
        break;
      case OXA::Folder::Tasks:
        item.setMimeType( IncidenceMimeTypeVisitor::todoMimeType() );
        item.setPayload<KCal::Incidence::Ptr>( object.task() );
        break;
      case OXA::Folder::Unbound:
        Q_ASSERT( false );
        break;
    }
    const RemoteInformation remoteInformation( object.objectId(), object.module(), object.lastModified() );
    remoteInformation.store( item );

    // the value of objectsLastSync is determined by the maximum last modified value
    // of the added or changed objects
    objectsLastSync = qMax( objectsLastSync, object.lastModified().toULongLong() );

    changedItems.append( item );
  }

  Item::List removedItems;

  const OXA::Object::List deletedObjects = requestJob->deletedObjects();
  foreach ( const OXA::Object &object, deletedObjects ) {
    Item item;

    const RemoteInformation remoteInformation( object.objectId(), object.module(), object.lastModified() );
    remoteInformation.store( item );

    removedItems.append( item );
  }

  if ( objectsLastSync != lastSyncInfo.lastSync( collection.id() ) ) {
    // according to the OX developers we should subtract one millisecond from the
    // maximum last modified value to cover multiple changes that might have been
    // done in the same millisecond to the data on the server
    lastSyncInfo.setLastSync( collection.id(), objectsLastSync - 1 );
    lastSyncInfo.save();
  }

  //qDebug("changedObjects=%d removedObjects=%d", modifiedObjects.count(), deletedObjects.count());
  //qDebug("changedItems=%d removedItems=%d", changedItems.count(), removedItems.count());
  itemsRetrievedIncremental( changedItems, removedItems );
}

void OpenXchangeResource::onObjectRequestJobFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }

  OXA::ObjectRequestJob *requestJob = qobject_cast<OXA::ObjectRequestJob*>( job );
  Q_ASSERT( requestJob );

  const OXA::Object object = requestJob->object();

  Item item = job->property( "item" ).value<Item>();

  const RemoteInformation remoteInformation( object.objectId(), object.module(), object.lastModified() );
  remoteInformation.store( item );

  switch ( object.module() ) {
    case OXA::Folder::Contacts:
      if ( !object.contact().isEmpty() ) {
        item.setMimeType( KABC::Addressee::mimeType() );
        item.setPayload<KABC::Addressee>( object.contact() );
      } else {
        item.setMimeType( KABC::ContactGroup::mimeType() );
        item.setPayload<KABC::ContactGroup>( object.contactGroup() );
      }
      break;
    case OXA::Folder::Calendar:
      item.setMimeType( IncidenceMimeTypeVisitor::eventMimeType() );
      item.setPayload<KCal::Incidence::Ptr>( object.event() );
      break;
    case OXA::Folder::Tasks:
      item.setMimeType( IncidenceMimeTypeVisitor::todoMimeType() );
      item.setPayload<KCal::Incidence::Ptr>( object.task() );
      break;
    case OXA::Folder::Unbound:
      Q_ASSERT( false );
      break;
  }

  itemRetrieved( item );
}

void OpenXchangeResource::onObjectCreateJobFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }

  OXA::ObjectCreateJob *createJob = qobject_cast<OXA::ObjectCreateJob*>( job );
  Q_ASSERT( createJob );

  const OXA::Object object = createJob->object();

  Item item = job->property( "item" ).value<Item>();

  const RemoteInformation remoteInformation( object.objectId(), object.module(), object.lastModified() );
  remoteInformation.store( item );

  changeCommitted( item );
}

void OpenXchangeResource::onObjectModifyJobFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }

  OXA::ObjectModifyJob *modifyJob = qobject_cast<OXA::ObjectModifyJob*>( job );
  Q_ASSERT( modifyJob );

  const OXA::Object object = modifyJob->object();

  Item item = job->property( "item" ).value<Item>();

  // update last_modified property
  RemoteInformation remoteInformation = RemoteInformation::load( item );
  remoteInformation.setLastModified( object.lastModified() );
  remoteInformation.store( item );

  changeCommitted( item );
}

void OpenXchangeResource::onObjectMoveJobFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }

  OXA::ObjectMoveJob *moveJob = qobject_cast<OXA::ObjectMoveJob*>( job );
  Q_ASSERT( moveJob );

  const OXA::Object object = moveJob->object();

  Item item = job->property( "item" ).value<Item>();

  // update last_modified property
  RemoteInformation remoteInformation = RemoteInformation::load( item );
  remoteInformation.setLastModified( object.lastModified() );
  remoteInformation.store( item );

  changeCommitted( item );
}

void OpenXchangeResource::onObjectDeleteJobFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }

  changeProcessed();
}

static Collection folderToCollection( const OXA::Folder &folder, const Collection &parentCollection )
{
  Collection collection;

  collection.setParentCollection( parentCollection );

  const RemoteInformation remoteInformation( folder.objectId(), folder.module(), folder.lastModified() );
  remoteInformation.store( collection );

  // set a unique name to make Akonadi happy
  collection.setName( folder.title() + '_' + QUuid::createUuid().toString() );

  EntityDisplayAttribute *attribute = collection.attribute<EntityDisplayAttribute>( Collection::AddIfMissing );
  attribute->setDisplayName( folder.title() );

  QStringList mimeTypes;
  mimeTypes.append( Collection::mimeType() );
  switch ( folder.module() ) {
    case OXA::Folder::Calendar:
      mimeTypes.append( IncidenceMimeTypeVisitor::eventMimeType() );
      attribute->setIconName( QString::fromLatin1( "view-calendar" ) );
      break;
    case OXA::Folder::Contacts:
      mimeTypes.append( KABC::Addressee::mimeType() );
      mimeTypes.append( KABC::ContactGroup::mimeType() );
      attribute->setIconName( QString::fromLatin1( "view-pim-contacts" ) );
      break;
    case OXA::Folder::Tasks:
      mimeTypes.append( IncidenceMimeTypeVisitor::todoMimeType() );
      attribute->setIconName( QString::fromLatin1( "view-pim-tasks" ) );
      break;
    case OXA::Folder::Unbound:
      break;
  }

  collection.setContentMimeTypes( mimeTypes );
  collection.setRights( folderPermissionsToCollectionRights( folder ) );

  return collection;
}

void OpenXchangeResource::onFoldersRequestJobFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }

  OXA::FoldersRequestJob *requestJob = qobject_cast<OXA::FoldersRequestJob*>( job );
  Q_ASSERT( requestJob );

  Collection::List collections;

  // add the standard collections
  collections << mStandardCollectionsMap.values();

  QMap<qlonglong, Collection> remoteIdMap( mStandardCollectionsMap );

  // add the folders from the server
  OXA::Folder::List folders = requestJob->folders();
  while ( !folders.isEmpty() ) {
    const OXA::Folder folder = folders.takeFirst();
    if ( remoteIdMap.contains( folder.folderId() ) ) {
      // we have the parent collection created already
      const Collection collection = folderToCollection( folder, remoteIdMap.value( folder.folderId() ) );
      remoteIdMap.insert( folder.objectId(), collection );
      collections.append( collection );
    } else {
      // we have to wait until the parent folder has been created
      folders.append( folder );
      qDebug() << "Error: parent folder id" << folder.folderId() << "of folder" << folder.title() << "is unknown";
    }
  }

  collectionsRetrieved( collections );
}

void OpenXchangeResource::onFoldersRequestDeltaJobFinished( KJob *job )
{
  //qDebug("onFoldersRequestDeltaJobFinished mCollectionsMap.count() = %d", mCollectionsMap.count());

  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }

  OXA::FoldersRequestDeltaJob *requestJob = qobject_cast<OXA::FoldersRequestDeltaJob*>( job );
  Q_ASSERT( requestJob );

  Collection::List changedCollections;

  // add the standard collections
  changedCollections << mStandardCollectionsMap.values();

  qulonglong foldersLastSync = Settings::self()->foldersLastSync();

  // add the new or modified folders from the server
  OXA::Folder::List modifiedFolders = requestJob->modifiedFolders();
  while ( !modifiedFolders.isEmpty() ) {
    const OXA::Folder folder = modifiedFolders.takeFirst();
    if ( mCollectionsMap.contains( folder.folderId() ) ) {
      // we have the parent collection created already
      const Collection collection = folderToCollection( folder, mCollectionsMap.value( folder.folderId() ) );
      mCollectionsMap.insert( folder.objectId(), collection );
      changedCollections.append( collection );

      // the value of foldersLastSync is determined by the maximum last modified value
      // of the added or changed folders
      foldersLastSync = qMax( foldersLastSync, folder.lastModified().toULongLong() );

    } else {
      // we have to wait until the parent folder has been created
      modifiedFolders.append( folder );
      qDebug() << "Error: parent folder id" << folder.folderId() << "of folder" << folder.title() << "is unknown";
    }
  }

  Collection::List removedCollections;

  // add the deleted folders from the server
  OXA::Folder::List deletedFolders = requestJob->deletedFolders();
  foreach ( const OXA::Folder &folder, deletedFolders ) {
    Collection collection;
    collection.setRemoteId( QString::number( folder.objectId() ) );

    removedCollections.append( collection );
  }

  if ( foldersLastSync != Settings::self()->foldersLastSync() ) {
    // according to the OX developers we should subtract one millisecond from the
    // maximum last modified value to cover multiple changes that might have been
    // done in the same millisecond to the data on the server
    Settings::self()->setFoldersLastSync( foldersLastSync - 1 );
    Settings::self()->writeConfig();
  }

  //qDebug("changedFolders=%d removedFolders=%d", modifiedFolders.count(), deletedFolders.count());
  //qDebug("changedCollections=%d removedCollections=%d", changedCollections.count(), removedCollections.count());
  collectionsRetrievedIncremental( changedCollections, removedCollections );
}

void OpenXchangeResource::onFolderCreateJobFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }

  OXA::FolderCreateJob *createJob = qobject_cast<OXA::FolderCreateJob*>( job );
  Q_ASSERT( createJob );

  const OXA::Folder folder = createJob->folder();

  Collection collection = job->property( "collection" ).value<Collection>();

  const RemoteInformation remoteInformation( folder.objectId(), folder.module(), folder.lastModified() );
  remoteInformation.store( collection );

  // set matching icon
  EntityDisplayAttribute *attribute = collection.attribute<EntityDisplayAttribute>( Collection::AddIfMissing );
  switch ( folder.module() ) {
    case OXA::Folder::Calendar:
      attribute->setIconName( QString::fromLatin1( "view-calendar" ) );
      break;
    case OXA::Folder::Contacts:
      attribute->setIconName( QString::fromLatin1( "view-pim-contacts" ) );
      break;
    case OXA::Folder::Tasks:
      attribute->setIconName( QString::fromLatin1( "view-pim-tasks" ) );
      break;
    case OXA::Folder::Unbound:
      break;
  }

  changeCommitted( collection );
}

void OpenXchangeResource::onFolderModifyJobFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }

  OXA::FolderModifyJob *modifyJob = qobject_cast<OXA::FolderModifyJob*>( job );
  Q_ASSERT( modifyJob );

  const OXA::Folder folder = modifyJob->folder();

  Collection collection = job->property( "collection" ).value<Collection>();

  // update last_modified property
  RemoteInformation remoteInformation = RemoteInformation::load( collection );
  remoteInformation.setLastModified( folder.lastModified() );
  remoteInformation.store( collection );

  changeCommitted( collection );
}

void OpenXchangeResource::onFolderMoveJobFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }

  OXA::FolderMoveJob *moveJob = qobject_cast<OXA::FolderMoveJob*>( job );
  Q_ASSERT( moveJob );

  const OXA::Folder folder = moveJob->folder();

  Collection collection = job->property( "collection" ).value<Collection>();

  // update last_modified property
  RemoteInformation remoteInformation = RemoteInformation::load( collection );
  remoteInformation.setLastModified( folder.lastModified() );
  remoteInformation.store( collection );

  changeCommitted( collection );
}

void OpenXchangeResource::onFolderDeleteJobFinished( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }

  changeProcessed();
}

/**
 * For incremental updates we need a mapping between the folder id
 * and the collection object for all collections of this resource,
 * so that we can find out the right parent collection in
 * onFoldersRequestDeltaJob().
 *
 * Therefor we trigger this method when the resource is started and
 * configured to use incremental sync.
 */
void OpenXchangeResource::syncCollectionsRemoteIdCache()
{
  mCollectionsMap.clear();

  // copy the standard collections
  mCollectionsMap = mStandardCollectionsMap;

  CollectionFetchJob *job = new CollectionFetchJob( mResourceCollection, CollectionFetchJob::Recursive, this );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onFetchResourceCollectionsFinished( KJob* ) ) );
}

void OpenXchangeResource::onFetchResourceCollectionsFinished( KJob *job )
{
  if ( job->error() ) {
    qDebug() << "Error: Unable to fetch resource collections:" << job->errorText();
    return;
  }

  const CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob*>( job );

  // copy the remaining collections of the resource
  const Collection::List collections = fetchJob->collections();
  foreach ( const Collection &collection, collections )
    mCollectionsMap.insert( collection.remoteId().toULongLong(), collection );
}

AKONADI_RESOURCE_MAIN( OpenXchangeResource )

#include "openxchangeresource.moc"
