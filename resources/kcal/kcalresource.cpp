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

#include <kcal/calendarresources.h>
#include <kcal/resourcecalendar.h>

#include <kresources/factory.h>
#include <kresources/configdialog.h>

#include <akonadi/kcal/kcalmimetypevisitor.h>

#include <kconfig.h>
#include <kinputdialog.h>

#include <QTimer>

#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

using namespace Akonadi;

KCalResource::KCalResource( const QString &id )
  : ResourceBase( id ),
    mCalendar( new KCal::CalendarResources( QLatin1String( "UTC" ) ) ),
    mLoaded( false ),
    mDelayedUpdateTimer( new QTimer( this ) ),
    mMimeVisitor( new KCalMimeTypeVisitor() )
{
  mDelayedUpdateTimer->setInterval( 10 );
  mDelayedUpdateTimer->setSingleShot( true );

  connect( mCalendar, SIGNAL( signalErrorMessage( const QString& ) ),
           this, SLOT( calendarError( const QString& ) ) );

  connect( mCalendar, SIGNAL( calendarChanged() ),
           this, SLOT( calendarChanged() ) );

  connect( mDelayedUpdateTimer, SIGNAL( timeout() ),
           this, SLOT( delayedUpdate() ) );

  KSharedConfig::Ptr config = KGlobal::config();
  Q_ASSERT( !config.isNull() );

  mCalendar->readConfig( config.data() );
  mLoaded = loadCalendar();
}

KCalResource::~KCalResource()
{
  delete mMimeVisitor;
}

bool KCalResource::retrieveItem( const Akonadi::Item &item, const QStringList &parts )
{
  kDebug() << "item(" << item.id() << "," << item.remoteId() << "),"
           << parts;
  Q_UNUSED( parts );
  const QString rid = item.remoteId();

  KCal::Incidence *incidence = mCalendar->incidence( item.remoteId() );
  if ( incidence == 0 ) {
    error( i18n( "Incidence with uid '%1' not found!", rid ) );
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
  mLastError.clear();

  if ( !mCalendar->save() ) {
    kError() << "Saving failed: " << mLastError;
    return;
  }

  kDebug() << "Saving succeeded";
}

void KCalResource::configure( WId windowId )
{
  Q_UNUSED( windowId );

  QWidget *window = 0; // should use windowId somehow

  KCal::CalendarResourceManager *manager = mCalendar->resourceManager();
  KCal::ResourceCalendar *resource = manager->standardResource();

  if ( resource != 0 ) {
    mCalendar->resourceManager()->remove( resource );
    resource = 0;
    mLoaded = false;
  }

  QStringList types = manager->resourceTypeNames();
  QStringList descs = manager->resourceTypeDescriptions();

  int index = types.indexOf( QLatin1String( "akonadi" ) );
  if ( index != -1 ) {
    types.removeAt( index );
    descs.removeAt( index );
  }

  bool ok = false;
  QString desc = KInputDialog::getItem( i18n( "Create KCal resource" ),
                                        i18n( "Please select type of the new resource:" ),
                                        descs, 0, false, &ok, window );
  if ( !ok )
    return;

  QString type = types[ descs.indexOf( desc ) ];

  // Create new resource
  resource = manager->createResource( type );
  if ( resource == 0 ) {
    error( i18n( "Unable to create a KCal resource of type %1", type ) );
    return;
  }

  resource->setResourceName( i18n( "%1 calendar", type ) );

  KRES::ConfigDialog dlg( window, QLatin1String( "calendar" ), resource );

  if ( !dlg.exec() ) {
    delete resource;
    resource = 0;
    return;
  }

  manager->add( resource );
  manager->writeConfig( KGlobal::config().data() );

  connect( resource, SIGNAL( resourceLoaded( ResourceCalendar* ) ),
           this, SLOT(loadingFinished( ResourceCalendar* ) ) );
  connect( resource, SIGNAL( resourceLoadError( ResourceCalendar*, const QString& ) ),
           this, SLOT(loadingError( ResourceCalendar*, const QString& ) ) );

  mLoaded = loadCalendar();
  synchronize();
}

void KCalResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection& col )
{
  Q_UNUSED( col );

  IncidencePtr incidence = item.payload<IncidencePtr>();
  if ( incidence.get() != 0 ) {
    mCalendar->addIncidence( incidence->clone() );

    Item i( item );
    i.setRemoteId( incidence->uid() );
    changesCommitted( i );
  } else {
    changeProcessed();
  }
}

void KCalResource::itemChanged( const Akonadi::Item &item, const QStringList& parts )
{
  Q_UNUSED( parts );

  IncidencePtr incidence = item.payload<IncidencePtr>();
  if ( incidence.get() != 0 ) {
    KCal::Incidence *oldIncidence = mCalendar->incidence( incidence->uid() );
    if ( oldIncidence != 0)
      mCalendar->deleteIncidence( oldIncidence );

    mCalendar->addIncidence( incidence->clone() );

    Item i( item );
    i.setRemoteId( incidence->uid() );
    changesCommitted( i );
  } else {
    changeProcessed();
  }
}

void KCalResource::itemRemoved( const Akonadi::Item &item )
{
  KCal::Incidence *incidence = mCalendar->incidence( item.remoteId() );
  if ( incidence != 0 )
    mCalendar->deleteIncidence( incidence );

  changeProcessed();
}

void KCalResource::retrieveCollections()
{
  kDebug();
  KCal::ResourceCalendar *resource = mCalendar->resourceManager()->standardResource();
  if ( resource == 0 ) {
    kError() << "No KCal resource";

    error( i18n( "No KCal resource plugin configured" ) );

    emit status( Broken, i18n( "No KCal resource plugin configured" ) );
    return;
  }

  Collection c;
  c.setParent( Collection::root() );
  c.setRemoteId( resource->identifier() );
  c.setName( name() );

  QStringList mimeTypes;
  mimeTypes << QLatin1String( "text/calendar" );
  mimeTypes += mMimeVisitor->allMimeTypes();
  c.setContentMimeTypes( mimeTypes );

  Collection::List list;
  list << c;
  collectionsRetrieved( list );
}

void KCalResource::retrieveItems( const Akonadi::Collection &col, const QStringList &parts )
{
  Q_UNUSED( col );
  Q_UNUSED( parts );

  if ( mDelayedUpdateTimer->isActive() )
    return;

  mDelayedUpdateTimer->start();
}

bool KCalResource::loadCalendar()
{
  if ( !mLoaded ) {
    mLastError.clear();

    KCal::CalendarResourceManager *manager = mCalendar->resourceManager();
    KCal::ResourceCalendar *resource = manager->standardResource();
    if ( resource != 0 && !resource->isOpen() ) {
      if ( !resource->open() ) {
        kError() << "Opening resource" << resource->identifier() << "failed";
        return false;
      }
    }

    mCalendar->load();
    if ( !mLastError.isEmpty() ) {
      kError() << "Loading failed:" << mLastError;
      return false;
    }
  }

  return true;
}

void KCalResource::calendarError( const QString& message )
{
  kDebug() << message;

  mLastError = message;
}

void KCalResource::calendarChanged()
{
  if ( mDelayedUpdateTimer->isActive() )
    return;

  synchronize();
}

void KCalResource::delayedUpdate()
{
  Item::List items;

  KCal::Incidence::List incidences = mCalendar->rawIncidences();

  KCal::Incidence::List::const_iterator it    = incidences.begin();
  KCal::Incidence::List::const_iterator endIt = incidences.end();
  for ( ; it != endIt; ++it ) {
    Item item( mMimeVisitor->mimeType( *it ) );
    item.setRemoteId( (*it)->uid() );
    items.append( item );
  }

  itemsRetrieved( items );
}

AKONADI_RESOURCE_MAIN( KCalResource )

#include "kcalresource.moc"
