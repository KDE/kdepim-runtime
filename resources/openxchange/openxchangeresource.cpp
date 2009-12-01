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

#include <kabc/addressee.h>
#include <klocale.h>

#include <oxa/davmanager.h>
#include <oxa/foldercreatejob.h>
#include <oxa/folderdeletejob.h>
#include <oxa/foldersrequestjob.h>
#include <oxa/useridrequestjob.h>

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

  if ( !userPermissions.contains( Settings::self()->userId() ) ) {
    // There are no rights given for us explicitly, so it is read-only
    qDebug() << "no rights found for user" << Settings::self()->userId() << "on folder" << folder.title();
    return Collection::ReadOnly;
  } else {
    const OXA::Folder::Permissions permissions = userPermissions.value( Settings::self()->userId() );
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
  changeRecorder()->itemFetchScope().setAncestorRetrieval( ItemFetchScope::All );
  changeRecorder()->collectionFetchScope().setAncestorRetrieval( CollectionFetchScope::All );

  setHierarchicalRemoteIdentifiersEnabled( true );

  setName( i18n( "Open-Xchange" ) );

  KUrl baseUrl = Settings::self()->baseUrl();
  baseUrl.setUserName( Settings::self()->username() );
  baseUrl.setPassword( Settings::self()->password() );
  OXA::DavManager::self()->setBaseUrl( baseUrl );
}

OpenXchangeResource::~OpenXchangeResource()
{
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

    // find out the uid of the user before we continue
    OXA::UserIdRequestJob *job = new OXA::UserIdRequestJob( this );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( onUserIdRequestJobFinished( KJob* ) ) );
    job->start();
  } else {
    emit configurationDialogRejected();
  }
}

void OpenXchangeResource::onUserIdRequestJobFinished( KJob *job )
{
  if ( job->error() ) {
    //TODO: proper error handling
    return;
  }

  OXA::UserIdRequestJob *requestJob = qobject_cast<OXA::UserIdRequestJob*>( job );
  Q_ASSERT( requestJob );

  Settings::self()->setUserId( requestJob->userId() );

  // now we have the user id, so continue synchronization
  synchronize();
  emit configurationDialogAccepted();
}

void OpenXchangeResource::retrieveCollections()
{
  OXA::FoldersRequestJob *job = new OXA::FoldersRequestJob;
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onFoldersRequestJobFinished( KJob* ) ) );
  job->start();
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
  const OXA::Folder::List folders = requestJob->folders();
  foreach ( const OXA::Folder &folder, folders ) {
    Collection collection;
    if ( !remoteIdMap.contains( folder.folderId() ) ) {
      qDebug() << "Error: parent folder id" << folder.folderId() << "of folder" << folder.title() << "is unkown";
    }

    collection.setParentCollection( remoteIdMap.value( folder.folderId() ) );
    collection.setRemoteId( RemoteIdentifier( folder.objectId(), folder.module(), folder.lastModified() ).toString() );

    // set a unique name to make Akonadi happy
    collection.setName( folder.title() + "_" + QUuid::createUuid().toString() );

    EntityDisplayAttribute *attribute = collection.attribute<EntityDisplayAttribute>( Collection::AddIfMissing );
    attribute->setDisplayName( folder.title() );

    QStringList mimeTypes;
    mimeTypes.append( Collection::mimeType() );
    switch ( folder.module() ) {
      case OXA::Folder::Calendar:
        mimeTypes.append( QLatin1String( "application/x-vnd.akonadi.calendar.event" ) );
        attribute->setIconName( QString::fromLatin1( "view-calendar" ) );
        break;
      case OXA::Folder::Contacts:
        mimeTypes.append( KABC::Addressee::mimeType() );
        attribute->setIconName( QString::fromLatin1( "view-pim-contacts" ) );
        break;
      case OXA::Folder::Tasks:
        mimeTypes.append( QLatin1String( "application/x-vnd.akonadi.calendar.todo" ) );
        attribute->setIconName( QString::fromLatin1( "view-pim-tasks" ) );
        break;
      case OXA::Folder::Unbound:
        break;
    }

    collection.setContentMimeTypes( mimeTypes );
    collection.setRights( folderPermissionsToCollectionRights( folder ) );

    remoteIdMap.insert( folder.objectId(), collection );
    collections.append( collection );
  }

  collectionsRetrieved( collections );
}

void OpenXchangeResource::retrieveItems( const Akonadi::Collection &collection )
{
  Item::List items;

  itemsRetrieved( items );
}

bool OpenXchangeResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  cancelTask( i18n( "No items available" ) );
  return false;
}

void OpenXchangeResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  cancelTask( i18n( "Trying to write to a read-only directory" ) );
  return;
}

void OpenXchangeResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  cancelTask( i18n( "Trying to write to a read-only directory" ) );
  return;
}

void OpenXchangeResource::itemRemoved( const Akonadi::Item &item )
{
  cancelTask( i18n( "Trying to write to a read-only directory" ) );
  return;
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
  userPermissions.insert( Settings::self()->userId(), permissions );

  // set user permissions of folder
  folder.setUserPermissions( userPermissions );

  OXA::FolderCreateJob *job = new OXA::FolderCreateJob( folder, this );
  job->setProperty( "collection", QVariant::fromValue( collection ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onFolderCreateJobFinished( KJob* ) ) );
  job->start();
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

void OpenXchangeResource::collectionChanged( const Akonadi::Collection &collection )
{
  cancelTask( i18n( "Trying to write to a read-only directory" ) );
  return;
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
