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

#include <akonadi/changerecorder.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/kcal/incidencemimetypevisitor.h>

#include <kabc/addressee.h>
#include <kcal/event.h>
#include <kcal/todo.h>
#include <klocale.h>

#include <oxa/davmanager.h>
#include <oxa/foldercreatejob.h>
#include <oxa/folderdeletejob.h>
#include <oxa/foldermodifyjob.h>
#include <oxa/foldermovejob.h>
#include <oxa/foldersrequestjob.h>
#include <oxa/objectcreatejob.h>
#include <oxa/objectdeletejob.h>
#include <oxa/objectmodifyjob.h>
#include <oxa/objectmovejob.h>
#include <oxa/objectrequestjob.h>
#include <oxa/objectsrequestjob.h>
#include <oxa/updateusersjob.h>
#include <oxa/users.h>

using namespace Akonadi;

class RemoteIdentifier
{
  public:
    RemoteIdentifier( qlonglong objectId, OXA::Folder::Module module, const QString &lastModified )
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

    static RemoteIdentifier fromString( const QString &data )
    {
      const QStringList parts = data.split( QLatin1Char( ':' ), QString::KeepEmptyParts );
      Q_ASSERT( parts.count() == 3 );


      OXA::Folder::Module module;
      if ( parts.at( 1 ) == QLatin1String( "calendar" ) )
        module = OXA::Folder::Calendar;
      else if ( parts.at( 1 ) == QLatin1String( "contacts" ) )
        module = OXA::Folder::Contacts;
      else if ( parts.at( 1 ) == QLatin1String( "tasks" ) )
        module = OXA::Folder::Tasks;
      else
        module = OXA::Folder::Unbound;

      return RemoteIdentifier( parts.at( 0 ).toLongLong(), module, parts.at( 2 ) );
    }

    QString toString() const
    {
      QString module;
      switch ( mModule ) {
        case OXA::Folder::Calendar: module = QLatin1String( "calendar" ); break;
        case OXA::Folder::Contacts: module = QLatin1String( "contacts" ); break;
        case OXA::Folder::Tasks: module = QLatin1String( "tasks" ); break;
        case OXA::Folder::Unbound: break;
      }

      QStringList parts;
      parts.append( QString::number( mObjectId ) );
      parts.append( module );
      parts.append( mLastModified );

      return parts.join( QLatin1String( ":" ) );
    }

  private:
    qlonglong mObjectId;
    OXA::Folder::Module mModule;
    QString mLastModified;
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
}

OpenXchangeResource::~OpenXchangeResource()
{
}

void OpenXchangeResource::cleanup()
{
  // be nice and remove cache file when resource is removed
  QFile::remove( OXA::Users::self()->cacheFilePath() );

  ResourceBase::cleanup();
}

void OpenXchangeResource::aboutToQuit()
{
}

void OpenXchangeResource::configure( WId windowId )
{
  ConfigDialog dlg( windowId );
  if ( dlg.exec() ) {
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
  OXA::FoldersRequestJob *job = new OXA::FoldersRequestJob( this );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onFoldersRequestJobFinished( KJob* ) ) );
  job->start();
}

void OpenXchangeResource::retrieveItems( const Akonadi::Collection &collection )
{
  const RemoteIdentifier remoteIdentifier = RemoteIdentifier::fromString( collection.remoteId() );

  OXA::Folder folder;
  folder.setObjectId( remoteIdentifier.objectId() );
  folder.setModule( remoteIdentifier.module() );

  OXA::ObjectsRequestJob *job = new OXA::ObjectsRequestJob( folder, this );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onObjectsRequestJobFinished( KJob* ) ) );
  job->start();
}

bool OpenXchangeResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  const RemoteIdentifier remoteIdentifier = RemoteIdentifier::fromString( item.remoteId() );

  OXA::Object object;
  object.setObjectId( remoteIdentifier.objectId() );
  object.setModule( remoteIdentifier.module() );

  OXA::ObjectRequestJob *job = new OXA::ObjectRequestJob( object, this );
  job->setProperty( "item", QVariant::fromValue( item ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onObjectRequestJobFinished( KJob* ) ) );
  job->start();

  return true;
}

void OpenXchangeResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  const RemoteIdentifier remoteIdentifier = RemoteIdentifier::fromString( collection.remoteId() );

  OXA::Object object;
  object.setFolderId( remoteIdentifier.objectId() );
  object.setModule( remoteIdentifier.module() );

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
  const RemoteIdentifier remoteIdentifier = RemoteIdentifier::fromString( item.remoteId() );
  const RemoteIdentifier parentRemoteIdentifier = RemoteIdentifier::fromString( item.parentCollection().remoteId() );

  OXA::Object object;
  object.setObjectId( remoteIdentifier.objectId() );
  object.setModule( remoteIdentifier.module() );
  object.setFolderId( parentRemoteIdentifier.objectId() );
  object.setLastModified( remoteIdentifier.lastModified() );

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
  const RemoteIdentifier remoteIdentifier = RemoteIdentifier::fromString( item.remoteId() );
  const RemoteIdentifier parentRemoteIdentifier = RemoteIdentifier::fromString( item.parentCollection().remoteId() );

  OXA::Object object;
  object.setObjectId( remoteIdentifier.objectId() );
  object.setFolderId( parentRemoteIdentifier.objectId() );
  object.setModule( remoteIdentifier.module() );
  object.setLastModified( remoteIdentifier.lastModified() );

  OXA::ObjectDeleteJob *job = new OXA::ObjectDeleteJob( object, this );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onObjectDeleteJobFinished( KJob* ) ) );

  job->start();
}

void OpenXchangeResource::itemMoved( const Akonadi::Item &item, const Akonadi::Collection &collectionSource,
                                     const Akonadi::Collection &collectionDestination )
{
  const RemoteIdentifier remoteIdentifier = RemoteIdentifier::fromString( item.remoteId() );
  const RemoteIdentifier parentRemoteIdentifier = RemoteIdentifier::fromString( collectionSource.remoteId() );
  const RemoteIdentifier newParentRemoteIdentifier = RemoteIdentifier::fromString( collectionDestination.remoteId() );

  OXA::Object object;
  object.setObjectId( remoteIdentifier.objectId() );
  object.setModule( remoteIdentifier.module() );
  object.setFolderId( parentRemoteIdentifier.objectId() );
  object.setLastModified( remoteIdentifier.lastModified() );

  OXA::Folder destinationFolder;
  destinationFolder.setObjectId( newParentRemoteIdentifier.objectId() );

  OXA::ObjectMoveJob *job = new OXA::ObjectMoveJob( object, destinationFolder, this );
  job->setProperty( "item", QVariant::fromValue( item ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onObjectMoveJobFinished( KJob* ) ) );

  job->start();
}

void OpenXchangeResource::collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent )
{
  const RemoteIdentifier parentRemoteIdentifier = RemoteIdentifier::fromString( parent.remoteId() );

  OXA::Folder folder;
  folder.setTitle( collection.name() );
  folder.setFolderId( parentRemoteIdentifier.objectId() );
  folder.setType( OXA::Folder::Private );

  // the folder 'inherits' the module type of its parent collection
  folder.setModule( parentRemoteIdentifier.module() );

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
  const RemoteIdentifier remoteIdentifier = RemoteIdentifier::fromString( collection.remoteId() );

  // do not try to change the standard collections
  if ( remoteIdentifier.objectId() >= 0 && remoteIdentifier.objectId() <= 4 ) {
    changeCommitted( collection );
    return;
  }

  const RemoteIdentifier parentRemoteIdentifier = RemoteIdentifier::fromString( collection.parentCollection().remoteId() );

  OXA::Folder folder;
  folder.setObjectId( remoteIdentifier.objectId() );
  folder.setFolderId( parentRemoteIdentifier.objectId() );
  folder.setTitle( collection.name() );
  folder.setLastModified( remoteIdentifier.lastModified() );

  OXA::FolderModifyJob *job = new OXA::FolderModifyJob( folder, this );
  job->setProperty( "collection", QVariant::fromValue( collection ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onFolderModifyJobFinished( KJob* ) ) );
}

void OpenXchangeResource::collectionRemoved( const Akonadi::Collection &collection )
{
  const RemoteIdentifier remoteIdentifier = RemoteIdentifier::fromString( collection.remoteId() );

  OXA::Folder folder;
  folder.setObjectId( remoteIdentifier.objectId() );
  folder.setLastModified( remoteIdentifier.lastModified() );

  OXA::FolderDeleteJob *job = new OXA::FolderDeleteJob( folder, this );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onFolderDeleteJobFinished( KJob* ) ) );

  job->start();
}

void OpenXchangeResource::collectionMoved( const Akonadi::Collection &collection, const Akonadi::Collection &collectionSource,
                                           const Akonadi::Collection &collectionDestination )
{
  const RemoteIdentifier remoteIdentifier = RemoteIdentifier::fromString( collection.remoteId() );
  const RemoteIdentifier parentRemoteIdentifier = RemoteIdentifier::fromString( collectionSource.remoteId() );
  const RemoteIdentifier newParentRemoteIdentifier = RemoteIdentifier::fromString( collectionDestination.remoteId() );

  OXA::Folder folder;
  folder.setObjectId( remoteIdentifier.objectId() );
  folder.setFolderId( parentRemoteIdentifier.objectId() );

  OXA::Folder destinationFolder;
  destinationFolder.setObjectId( newParentRemoteIdentifier.objectId() );

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
    item.setRemoteId( RemoteIdentifier( object.objectId(), object.module(), object.lastModified() ).toString() );

    items.append( item );
  }

  itemsRetrieved( items );
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
  item.setRemoteId( RemoteIdentifier( object.objectId(), object.module(), object.lastModified() ).toString() );

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
  item.setRemoteId( RemoteIdentifier( object.objectId(), object.module(), object.lastModified() ).toString() );

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
  RemoteIdentifier remoteIdentifier = RemoteIdentifier::fromString( item.remoteId() );
  remoteIdentifier.setLastModified( object.lastModified() );
  item.setRemoteId( remoteIdentifier.toString() );

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
  RemoteIdentifier remoteIdentifier = RemoteIdentifier::fromString( item.remoteId() );
  remoteIdentifier.setLastModified( object.lastModified() );
  item.setRemoteId( remoteIdentifier.toString() );

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
  collection.setRemoteId( RemoteIdentifier( folder.objectId(), folder.module(), folder.lastModified() ).toString() );

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
  QMap<qlonglong, Collection> remoteIdMap;

  // create the standard collections
  Collection resourceCollection;
  resourceCollection.setParentCollection( Collection::root() );
  resourceCollection.setRemoteId( RemoteIdentifier( 0, OXA::Folder::Unbound, QString() ).toString() );
  resourceCollection.setName( name() );
  resourceCollection.setContentMimeTypes( QStringList() << Collection::mimeType() );
  resourceCollection.setRights( Collection::ReadOnly );
  EntityDisplayAttribute *attribute = resourceCollection.attribute<EntityDisplayAttribute>( Collection::AddIfMissing );
  attribute->setIconName( QLatin1String( "ox" ) );

  Collection privateFolder;
  privateFolder.setParentCollection( resourceCollection );
  privateFolder.setRemoteId( RemoteIdentifier( 1, OXA::Folder::Unbound, QString() ).toString() );
  privateFolder.setName( i18n( "Private Folder" ) );
  privateFolder.setContentMimeTypes( QStringList() << Collection::mimeType() );
  privateFolder.setRights( Collection::ReadOnly );

  Collection publicFolder;
  publicFolder.setParentCollection( resourceCollection );
  publicFolder.setRemoteId( RemoteIdentifier( 2, OXA::Folder::Unbound, QString() ).toString() );
  publicFolder.setName( i18n( "Public Folder" ) );
  publicFolder.setContentMimeTypes( QStringList() << Collection::mimeType() );
  publicFolder.setRights( Collection::ReadOnly );

  Collection sharedFolder;
  sharedFolder.setParentCollection( resourceCollection );
  sharedFolder.setRemoteId( RemoteIdentifier( 3, OXA::Folder::Unbound, QString() ).toString() );
  sharedFolder.setName( i18n( "Shared Folder" ) );
  sharedFolder.setContentMimeTypes( QStringList() << Collection::mimeType() );
  sharedFolder.setRights( Collection::ReadOnly );

  Collection systemFolder;
  systemFolder.setParentCollection( resourceCollection );
  systemFolder.setRemoteId( RemoteIdentifier( 4, OXA::Folder::Unbound, QString() ).toString() );
  systemFolder.setName( i18n( "System Folder" ) );
  systemFolder.setContentMimeTypes( QStringList() << Collection::mimeType() );
  systemFolder.setRights( Collection::ReadOnly );

  collections.append( resourceCollection );
  collections.append( privateFolder );
  collections.append( publicFolder );
  collections.append( sharedFolder );
  collections.append( systemFolder );

  remoteIdMap.insert( 0, resourceCollection );
  remoteIdMap.insert( 1, privateFolder );
  remoteIdMap.insert( 2, publicFolder );
  remoteIdMap.insert( 3, sharedFolder );
  remoteIdMap.insert( 4, systemFolder );

  // add the folders from the server
  OXA::Folder::List folders = requestJob->folders();
  while ( !folders.isEmpty() ) {
    const OXA::Folder folder = folders.takeFirst();
    qDebug("handle folder %s", qPrintable( folder.title() ));
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
  collection.setRemoteId( RemoteIdentifier( folder.objectId(), folder.module(), folder.lastModified() ).toString() );

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
  RemoteIdentifier remoteIdentifier = RemoteIdentifier::fromString( collection.remoteId() );
  remoteIdentifier.setLastModified( folder.lastModified() );
  collection.setRemoteId( remoteIdentifier.toString() );

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
  RemoteIdentifier remoteIdentifier = RemoteIdentifier::fromString( collection.remoteId() );
  remoteIdentifier.setLastModified( folder.lastModified() );
  collection.setRemoteId( remoteIdentifier.toString() );

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

AKONADI_RESOURCE_MAIN( OpenXchangeResource )

#include "openxchangeresource.moc"
