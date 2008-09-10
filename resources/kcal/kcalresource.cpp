/*
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "kcalresource.h"

#include "../kabc/kresourceassistant.h"

#include <kcal/resourcecalendar.h>

#include <kresources/configdialog.h>

#include <akonadi/changerecorder.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/kcal/kcalmimetypevisitor.h>

#include <kconfig.h>
#include <kinputdialog.h>
#include <krandom.h>
#include <kwindowsystem.h>

#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

using namespace Akonadi;

KCalResource::KCalResource( const QString &id )
  : ResourceBase( id ),
    mManager( new KCal::CalendarResourceManager( QLatin1String( "calendar" ) ) ),
    mResource( 0 ),
    mMimeVisitor( new KCalMimeTypeVisitor() ),
    mFullItemRetrieve( false )
{
  connect( this, SIGNAL(reloadConfiguration()), SLOT(reloadConfig()) );

  changeRecorder()->itemFetchScope().fetchFullPayload();
  changeRecorder()->fetchCollection( true );
}

KCalResource::~KCalResource()
{
  delete mMimeVisitor;
}

void KCalResource::configure( WId windowId )
{
  if ( mResource != 0 ) {
    emit status( Running,
                 i18nc( "@info:status", "Changing calendar plugin configuration" ) );
    KRES::ConfigDialog dlg( 0, QLatin1String( "calendat" ), mResource );
    KWindowSystem::setMainWindow( &dlg, windowId );
    if ( dlg.exec() )
      mManager->writeConfig( KGlobal::config().data() );

    emit status( Idle, QString() );
    // TODO: need to react on name changes, but do we need to react on data changes?
    // as a workaround lets sync the collection tree
    synchronizeCollectionTree();
    return;
  }

  emit status( Running,
               i18nc( "@info:status", "Acquiring calendar plugin configuration" ) );

  KResourceAssistant kresAssistant( QLatin1String( "Calendar" ) );
  KWindowSystem::setMainWindow( &kresAssistant, windowId );

  connect( &kresAssistant, SIGNAL( error( const QString& ) ),
           this, SIGNAL( error( const QString& ) ) );

  if ( kresAssistant.exec() != QDialog::Accepted ) {
    emit status( Broken, i18nc( "@info:status", "No KDE calendar plugin configured yet" ) );
    return;
  }

  mResource = dynamic_cast<KCal::ResourceCalendar*>( kresAssistant.resource() );
  Q_ASSERT( mResource != 0 );

  mManager->add( mResource );
  mManager->writeConfig( KGlobal::config().data() );

  if ( !openConfiguration() ) {
    const QString message = i18nc( "@info:status", "Initialization based on newly created configuration failed." );
    emit error( message );
    emit status( Broken, message );
    return;
  }

  emit status( Running, i18nc( "@info:status", "Loading calendar" ) );

  // signals emitted by initialLoadingFinished() or loadingError()
  if ( !mResource->load() ) {
    kError() << "Resource::load() failed";
  }
}

void KCalResource::retrieveCollections()
{
  kDebug();
  if ( mResource == 0 ) {
    kError() << "No KCal resource";

    const QString message = i18nc( "@info:status", "No KDE calendar plugin configured yet" );
    emit error( message );

    emit status( Broken, message );
    return;
  }

  Collection topLevelCollection;
  topLevelCollection.setParent( Collection::root() );
  topLevelCollection.setRemoteId( mResource->identifier() );
  topLevelCollection.setName( mResource->resourceName() );

  QStringList mimeTypes;
  mimeTypes << QLatin1String( "text/calendar" );
  mimeTypes += mMimeVisitor->allMimeTypes();

  Collection::Rights readOnlyRights;

  Collection::Rights readWriteRights;
  readWriteRights |= Collection::CanCreateItem;
  readWriteRights |= Collection::CanChangeItem;
  readWriteRights |= Collection::CanDeleteItem;

  if ( mResource->canHaveSubresources() ) {
    mimeTypes << Collection::mimeType();

    readWriteRights |= Collection::CanCreateCollection;
    readWriteRights |= Collection::CanChangeCollection;
    readWriteRights |= Collection::CanDeleteCollection;
  }

  topLevelCollection.setContentMimeTypes( mimeTypes );
  topLevelCollection.setRights( mResource->readOnly() ? readOnlyRights : readWriteRights );

  Collection::List list;
  list << topLevelCollection;

  const QStringList subResources = mResource->subresources();
  kDebug() << "subResources" << subResources;
  foreach ( const QString &subResource, subResources ) {
    // TODO check with KOrganizer how it handles subresources
    Collection childCollection;
    childCollection.setParent( topLevelCollection );
    childCollection.setRemoteId( subResource );
    childCollection.setName( mResource->labelForSubresource( subResource ) );

    const QString type = mResource->subresourceType( subResource );

    if ( !type.isEmpty() ) {
      QStringList childMimeTypes( Collection::mimeType() );
      childMimeTypes << QLatin1String( "application/x-vnd.akonadi.calendar." ) + type;
      childCollection.setContentMimeTypes( childMimeTypes );
    } else
      childCollection.setContentMimeTypes( mimeTypes );

    // TODO we have no API for adding incidences to specific sub resources, so we should
    // not tell Akonadi adding that items is allowed
    childCollection.setRights( mResource->readOnly() ? readOnlyRights : readWriteRights );

    list << childCollection;
  }

  collectionsRetrieved( list );
}

void KCalResource::retrieveItems( const Akonadi::Collection &collection )
{
  kDebug();
  if ( mResource == 0 ) {
    kError() << "No KCal resource";

    const QString message = i18nc( "@info:status", "No KDE calendar plugin configured yet" );
    emit error( message );

    emit status( Broken, message );
    return;
  }

  Item::List items;

  const KCal::Incidence::List incidenceList = mResource->rawIncidences();
  foreach ( KCal::Incidence *incidence, incidenceList ) {
    const QString subResource = mResource->subresourceIdentifier( incidence );
    if ( subResource != collection.remoteId() ) {
      if ( collection.parent() != Collection::root().id() )
        continue;
    }

    Item item( mMimeVisitor->mimeType( incidence ) );
    item.setRemoteId( incidence->uid() );
    if ( mFullItemRetrieve )
      item.setPayload<IncidencePtr>( IncidencePtr( incidence->clone() ) );
    items << item;
  }

  itemsRetrieved( items );
}

bool KCalResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  kDebug() << "item id="  << item.id() << ", remoteId=" << item.remoteId()
           << "mimeType=" << item.mimeType() << "parts=" << parts;

  if ( mResource == 0 ) {
    kError() << "No KCal resource";

    const QString message = i18nc( "@info:status", "No KDE calendar plugin configured yet" );
    emit error( message );

    emit status( Broken, message );
    return false;
  }

  const QString rid = item.remoteId();

  KCal::Incidence *incidence = mResource->incidence( rid );
  if ( incidence == 0 ) {
    kError() << "No incidence with uid" << rid;
    emit error( i18nc( "@info:status",
                        "Request for data of a specific calendar entry failed "
                        "because there is no such entry" ) );
    return false;
  }

  Item i( item );
  i.setMimeType( mMimeVisitor->mimeType( incidence ) );
  i.setPayload<IncidencePtr>( IncidencePtr( incidence->clone() ) );
  itemRetrieved( i );

  return true;
}

void KCalResource::aboutToQuit()
{
  mManager->writeConfig( KGlobal::config().data() );

  mResource->save();
}

void KCalResource::doSetOnline( bool online )
{
  kDebug() << "online=" << online << ", isOnline=" << isOnline();
  if ( online )
    reloadConfig();
  else
    closeConfiguration();

  ResourceBase::doSetOnline( online );
}

void KCalResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& collection )
{
  kDebug() << "item id="  << item.id() << ", remoteId=" << item.remoteId()
           << "mimeType=" << item.mimeType();

  Q_UNUSED( collection );

  if ( item.hasPayload<IncidencePtr>() ) {
    IncidencePtr incidencePtr = item.payload<IncidencePtr>();
    if ( incidencePtr->uid().isEmpty() ) {
      const QString uid = KRandom::randomString( 10 );
      incidencePtr->setUid( uid );
    }

    KCal::Incidence *incidence = incidencePtr->clone();
    if ( mResource->addIncidence( incidence ) ) {
      Item newItem( item );
      newItem.setRemoteId( incidencePtr->uid() );
      newItem.setPayload<IncidencePtr>( incidencePtr );
      if ( !mResource->save( incidence ) ) {
        kError() << "Failed to save incidence" << incidence->uid();
        // resource error emitted by savingError()
      }
      changeCommitted( newItem );
      return;
    }

    kError() << "Failed to add incidence to resource";
  }

  changeProcessed();
}

void KCalResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& parts )
{
  kDebug() << "item id="  << item.id() << ", remoteId=" << item.remoteId()
           << "mimeType=" << item.mimeType() << "parts=" << parts;

  if ( item.hasPayload<IncidencePtr>() ) {
    IncidencePtr incidencePtr = item.payload<IncidencePtr>();

    // TODO check if there is a way to update an incidence instead of delete/add
    KCal::Incidence *incidence = mResource->incidence( incidencePtr->uid() );
    if ( incidence == 0 || mResource->deleteIncidence( incidence ) ) {
      incidence = incidencePtr->clone();
      if ( mResource->addIncidence( incidence ) ) {
        if ( !mResource->save( incidence ) ) {
          kError() << "Failed to save incidence" << incidence->uid();
          // resource error emitted by savingError()
        }
        changeCommitted( item );
        return;
      }

      kError() << "Failed to add incidence to resource";
    } else
      kError() << "Failed to delete old incidence from resource";
  }

  changeProcessed();
}

void KCalResource::itemRemoved( const Akonadi::Item &item )
{
  kDebug() << "item id=" << item.id() << ", remoteId=" << item.remoteId();

  KCal::Incidence *incidence = mResource->incidence( item.remoteId() );
  if ( incidence != 0 && mResource->deleteIncidence( incidence ) ) {
    changeCommitted( item );
    // TODO save calendar
    return;
  }

  changeProcessed();
}

void KCalResource::collectionAdded( const Akonadi::Collection &collection,
                                    const Akonadi::Collection &parent )
{
  kDebug() << "collection id=" << collection.id() << ", remoteId=" << collection.remoteId() << ", name=" << collection.name()
           << ", parent id="   << parent.id() << ", remoteId="     << parent.remoteId();

  if ( mResource->addSubresource( collection.name(), parent.remoteId() ) ) {
    // result delivered by signalSubresourceAdded(). how do we best relate this?
    changeCommitted( collection );
    return;
  }

  changeProcessed();
}

void KCalResource::collectionChanged( const Akonadi::Collection &collection )
{
  kDebug() << "collection id=" << collection.id() << ", remoteId=" << collection.remoteId();

  // currently only changing the top level collection's name supported
  if ( collection.parent() == Collection::root().id() ) {
    if ( collection.name() != mResource->resourceName() ) {
      mResource->setResourceName( collection.name() );
      changeCommitted( collection );
      // TODO save config
      return;
    }
  }

  changeProcessed();
}

void KCalResource::collectionRemoved( const Akonadi::Collection &collection )
{
  kDebug() << "collection id=" << collection.id() << ", remoteId=" << collection.remoteId();

  // currently only removing sub resource supported
  const QStringList subResources = mResource->subresources();
  if ( subResources.contains( collection.remoteId() ) ) {
    if ( mResource->removeSubresource( collection.remoteId() ) ) {
      changeCommitted( collection );
      // TODO save calendar
      return;
    }
  }

  changeProcessed();
}

bool KCalResource::openConfiguration()
{
  if ( mResource != 0 ) {
    if ( !mResource->isOpen() ) {
      if ( !mResource->open() ) {
        kError() << "Opening resource" << mResource->identifier() << "failed";
        return false;
      }
    }

    connect( mResource, SIGNAL( resourceLoaded( ResourceCalendar* ) ),
             this, SLOT( initialLoadingFinished( ResourceCalendar* ) ) );
    connect( mResource, SIGNAL( resourceLoadError( ResourceCalendar*, const QString& ) ),
             this, SLOT( loadingError( ResourceCalendar*, const QString& ) ) );
  }

  return true;
}

void KCalResource::closeConfiguration()
{
  if ( mResource != 0 ) {
    if ( mResource->isOpen() )
      mResource->close();

    disconnect( mResource, SIGNAL( resourceLoadError( ResourceCalendar*, const QString& ) ),
                this, SLOT( loadingError( ResourceCalendar*, const QString& ) ) );
    disconnect( mResource, SIGNAL( resourceSaveError( ResourceCalendar*, const QString& ) ),
                this, SLOT( savingError( ResourceCalendar*, const QString& ) ) );
    disconnect( mResource, SIGNAL( resourceChanged( ResourceCalendar* ) ),
                this, SLOT( resourceChanged( ResourceCalendar* ) ) );
  }
}

void KCalResource::reloadConfig()
{
  kDebug() << "resource=" << (void*) mResource;
  if ( mResource != 0 ) {
    if ( !mResource->save() ) {
      kError() << "Saving of calendar failed";
    }
  }
  closeConfiguration();

  KGlobal::config()->reparseConfiguration();

  if ( KGlobal::config()->groupList().isEmpty() ) {
    emit status( Broken, i18nc( "@info:status", "No KDE calendar plugin configured yet" ) );
    return;
  }

  Q_ASSERT( KGlobal::config().data() != 0);

  mManager->readConfig( KGlobal::config().data() );

  mResource = mManager->standardResource();
  if ( mResource != 0 ) {
    if ( mResource->type().toLower() == QLatin1String( "akonadi" ) ) {
      kError() << "Resource config points to an Akonadi bridge resource";
      emit status( Broken, i18nc( "@info:status", "No KDE calendar plugin configured yet" ) );
      return;
    }
  }

  if ( mResource == 0 ) {
    emit status( Broken, i18nc( "@info:status", "No KDE calendar plugin configured yet" ) );
    return;
  }

  if ( !isOnline() )
    return;

  if ( !openConfiguration() ) {
    kError() << "openConfiguration() failed";

    const QString message = i18nc( "@info:status", "Initialization based on stored configuration failed." );
    emit error( message );
    emit status( Broken, message );
    return;
  }

  emit status( Running, i18nc( "@info:status", "Loading calendar" ) );

  // signals emitted by initialLoadingFinished() or loadingError()
  if ( !mResource->load() ) {
    kError() << "Resource::load() failed";
  }
}

void KCalResource::initialLoadingFinished( ResourceCalendar *resource )
{
  Q_ASSERT( resource == mResource );

  kDebug();

  disconnect( mResource, SIGNAL( resourceLoaded( ResourceCalendar* ) ),
              this, SLOT( initialLoadingFinished( ResourceCalendar* ) ) );

  connect( mResource, SIGNAL( resourceChanged( ResourceCalendar* ) ),
           this, SLOT( resourceChanged( ResourceCalendar* ) ) );

  emit status( Idle, QString() );
  mFullItemRetrieve = false;
  synchronize();
}

void KCalResource::resourceChanged( ResourceCalendar *resource )
{
  Q_ASSERT( resource == mResource );

  kDebug();

  mFullItemRetrieve = true;
  synchronize();
}

void KCalResource::loadingError( ResourceCalendar *resource, const QString &message )
{
  Q_ASSERT( resource == mResource );

  kError() << "Loading error: " << message;
  const QString statusMessage =
    i18nc( "@info:status", "Loading of calendar failed: %1", message );

  emit error( statusMessage );
  emit status( Broken, statusMessage );
}

void KCalResource::savingError( ResourceCalendar *resource, const QString &message )
{
  Q_ASSERT( resource == mResource );

  kError() << "Saving error: " << message;
  const QString statusMessage =
    i18nc( "@info:status", "Saving of calendar failed: %1", message );

  emit error( statusMessage );
  emit status( Broken, statusMessage );
}

AKONADI_RESOURCE_MAIN( KCalResource )

#include "kcalresource.moc"
// kate: space-indent on; indent-width 2; replace-tabs on;
