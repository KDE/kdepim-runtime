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
#include "resources/kabc/kresourceassistant.h"
#include "kcal/kcalmimetypevisitor.h" // the kcal at the akonadi top-level

#include <kcal/calformat.h>
#include <kcal/resourcecalendar.h>

#include <kresources/configdialog.h>

#include <akonadi/changerecorder.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/itemfetchscope.h>

#include <kconfig.h>
#include <kinputdialog.h>
#include <kwindowsystem.h>

#include <QTimer>

#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

using namespace Akonadi;

static const char progName[]    = "KCalResource";
static const char progVersion[] = "0.9";

KCalResource::KCalResource( const QString &id )
  : ResourceBase( id ),
    mManager( new KCal::CalendarResourceManager( QLatin1String( "calendar" ) ) ),
    mResource( 0 ),
    mMimeVisitor( new KCalMimeTypeVisitor() ),
    mFullItemRetrieve( false ),
    mDelayedSaveTimer( new QTimer( this ) )
{
  // setup for UID generation
  const QString prodId = QLatin1String( "-//Akonadi//NONSGML KDE Compatibility Resource %1//EN" );
  KCal::CalFormat::setApplication( QLatin1String( progName ), prodId.arg( progVersion ) );

  connect( this, SIGNAL(reloadConfiguration()), SLOT(reloadConfig()) );

  connect( mDelayedSaveTimer, SIGNAL( timeout() ), this, SLOT( delayedSaveCalendar() ) );

  changeRecorder()->itemFetchScope().fetchFullPayload();
  changeRecorder()->fetchCollection( true );

  mDelayedSaveTimer->setSingleShot( true );
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
    KRES::ConfigDialog dlg( 0, QLatin1String( "calendar" ), mResource );
    KWindowSystem::setMainWindow( &dlg, windowId );
    if ( dlg.exec() ) {
      setName( mResource->resourceName() );
      mManager->writeConfig( KGlobal::config().data() );
    }

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

  EntityDisplayAttribute* attr =
    topLevelCollection.attribute<EntityDisplayAttribute>( Collection::AddIfMissing );
  attr->setDisplayName( mResource->resourceName() );
  attr->setIconName( QLatin1String( "office-calendar" ) );

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

    attr = childCollection.attribute<EntityDisplayAttribute>( Collection::AddIfMissing );
    attr->setDisplayName( childCollection.name() );

    const QString type = mResource->subresourceType( subResource );

    if ( !type.isEmpty() ) {
      QStringList childMimeTypes( Collection::mimeType() );
      childMimeTypes << QLatin1String( "application/x-vnd.akonadi.calendar." ) + type;
      childCollection.setContentMimeTypes( childMimeTypes );

      if ( type == QLatin1String( "journal" ) )
        attr->setIconName( QLatin1String( "view-pim-journal" ) );
      else if ( type == QLatin1String( "todo" ) )
        attr->setIconName( QLatin1String( "view-pim-tasks" ) );
      else
        attr->setIconName( QLatin1String( "office-calendar" ) );
    } else {
      attr->setIconName( QLatin1String( "office-calendar" ) );
      childCollection.setContentMimeTypes( mimeTypes );
    }

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
    if ( subResource == collection.remoteId() ||
         ( subResource.isEmpty() && collection.parent() == Collection::root().id() ) ) {
      Item item( mMimeVisitor->mimeType( incidence ) );
      item.setRemoteId( incidence->uid() );
      if ( mFullItemRetrieve )
        item.setPayload<IncidencePtr>( IncidencePtr( incidence->clone() ) );
      items << item;
    }
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

  if ( mResource == 0 ) {
    kError() << "Resource not fully operational yet";
    emit status( Broken, i18nc( "@info:status", "No KDE calendar plugin configured yet" ) );
    return;
  }

  Q_UNUSED( collection );

  if ( item.hasPayload<IncidencePtr>() ) {
    IncidencePtr incidencePtr = item.payload<IncidencePtr>();
    if ( incidencePtr->uid().isEmpty() ) {
      const QString uid = KCal::CalFormat::createUniqueId();
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

  if ( mResource == 0 ) {
    kError() << "Resource not fully operational yet";
    emit status( Broken, i18nc( "@info:status", "No KDE calendar plugin configured yet" ) );
    return;
  }

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

  if ( mResource == 0 ) {
    kError() << "Resource not fully operational yet";
    emit status( Broken, i18nc( "@info:status", "No KDE calendar plugin configured yet" ) );
    return;
  }

  KCal::Incidence *incidence = mResource->incidence( item.remoteId() );
  if ( incidence != 0 && mResource->deleteIncidence( incidence ) ) {
    if ( !mResource->save( incidence ) ) {
      kError() << "Failed to save incidence" << incidence->uid();
      // resource error emitted by savingError()
    }
    delete incidence;
    changeCommitted( item );
    return;
  }

  changeProcessed();
}

void KCalResource::collectionAdded( const Akonadi::Collection &collection,
                                    const Akonadi::Collection &parent )
{
  kDebug() << "collection id=" << collection.id() << ", remoteId=" << collection.remoteId() << ", name=" << collection.name()
           << ", parent id="   << parent.id() << ", remoteId="     << parent.remoteId();

  if ( mResource == 0 ) {
    kError() << "Resource not fully operational yet";
    emit status( Broken, i18nc( "@info:status", "No KDE calendar plugin configured yet" ) );
    return;
  }

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

  if ( mResource == 0 ) {
    kError() << "Resource not fully operational yet";
    emit status( Broken, i18nc( "@info:status", "No KDE calendar plugin configured yet" ) );
    return;
  }

  // currently only changing the top level collection's name supported
  if ( collection.parent() == Collection::root().id() ) {
    QString newName = collection.name();
    if ( collection.hasAttribute<EntityDisplayAttribute>() ) {
      EntityDisplayAttribute *attr = collection.attribute<EntityDisplayAttribute>();
      if ( !attr->displayName().isEmpty() )
        newName = attr->displayName();
    }

    if ( newName != mResource->resourceName() ) {
      mResource->setResourceName( newName );
      setName( newName );
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

  if ( mResource == 0 ) {
    kError() << "Resource not fully operational yet";
    emit status( Broken, i18nc( "@info:status", "No KDE calendar plugin configured yet" ) );
    return;
  }

  // currently only removing sub resource supported
  const QStringList subResources = mResource->subresources();
  if ( subResources.contains( collection.remoteId() ) ) {
    if ( mResource->removeSubresource( collection.remoteId() ) ) {
      changeCommitted( collection );
      scheduleSaveCalendar();
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

  setName( mResource->resourceName() );

  return true;
}

void KCalResource::closeConfiguration()
{
  mDelayedSaveTimer->stop();

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

bool KCalResource::saveCalendar()
{
  kDebug();
  mDelayedSaveTimer->stop();

  if ( !mResource || mResource->readOnly() )
    return false;

  if ( !mResource->save() ) {
    kError() << "Saving failed";
    // resource error emitted by savingError()
    return false;
  }

  kDebug() << "Saving succeeded";
  return true;
}

bool KCalResource::scheduleSaveCalendar()
{
  if ( !mResource || mResource->readOnly() )
    return false;

  if ( !mDelayedSaveTimer->isActive() )
    mDelayedSaveTimer->start( 5000 );

  return true;
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
  if ( mDelayedSaveTimer->isActive() ) {
    // TODO should record changes for delayed saving
    kError() << "Delayed saving scheduled when resource changed. We might have lost changes";
    mDelayedSaveTimer->stop();
  }

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

void KCalResource::delayedSaveCalendar()
{
  if ( !saveCalendar() ) {
    kError() << "Saving failed, rescheduling delayed save";
    if ( !scheduleSaveCalendar() ) {
      kError() << "Scheduling failed as well, giving up";
    }
  }
}

AKONADI_RESOURCE_MAIN( KCalResource )

#include "kcalresource.moc"
// kate: space-indent on; indent-width 2; replace-tabs on;
