/*
    Copyright (c) 2006 Till Adam <adam@kde.org>

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

#include "icalresource.h"
#include "settings.h"
#include "settingsadaptor.h"

#include <kcal/calendarlocal.h>
#include <kcal/incidence.h>

#include <akonadi/kcal/kcalmimetypevisitor.h>

#include <kdebug.h>
#include <kfiledialog.h>
#include <klocale.h>

#include <boost/shared_ptr.hpp>

using namespace Akonadi;
using namespace KCal;

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

ICalResource::ICalResource( const QString &id )
    :ResourceBase( id ), mCalendar( 0 ), mMimeVisitor( new KCalMimeTypeVisitor() )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );
  loadFile();
}

ICalResource::~ ICalResource()
{
  delete mCalendar;
  delete mMimeVisitor;
}

bool ICalResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );
  kDebug( 5251 ) << "Item:" << item.url();

  if ( !mCalendar ) {
    emit error( i18n("Calendar not loaded!") );
    return false;
  }

  const QString rid = item.remoteId();
  IncidencePtr incidence( mCalendar->incidence( rid )->clone() );
  if ( !incidence ) {
    emit error( i18n("Incidence with uid '%1' not found!", rid ) );
    return false;
  }

  Item i = item;
  i.setMimeType( mMimeVisitor->mimeType( incidence.get() ) );
  i.setPayload<IncidencePtr>( incidence );
  itemRetrieved( i );
  return true;
}

void ICalResource::aboutToQuit()
{
  if ( Settings::self()->readOnly() )
    return;
  const QString fileName = Settings::self()->path();
  if ( fileName.isEmpty() )
    emit error( i18n("No filename specified.") );
  else if ( !mCalendar->save( fileName ) )
    emit error( i18n("Failed to save calendar file to %1", fileName ) );
}

void ICalResource::configure( WId windowId )
{
  const QString oldFile = Settings::self()->path();
  KUrl url;
  if ( !oldFile.isEmpty() )
    url = KUrl::fromPath( oldFile );
  else
    url = KUrl::fromPath( QDir::homePath() );
  const QString newFile = KFileDialog::getOpenFileNameWId( url, "*.ics *.ical|" + i18nc("Filedialog filter for *.ics *.ical", "iCal Calendar File"), windowId, i18n("Select Calendar") );
  if ( newFile.isEmpty() )
    return;
  if ( oldFile == newFile )
    return;
  Settings::self()->setPath( newFile );
  loadFile();
  synchronize();
}

void ICalResource::loadFile()
{
  delete mCalendar;
  mCalendar = 0;
  const QString file = Settings::self()->path();
  if ( file.isEmpty() ) {
    emit status( Broken, i18n( "No iCal file specified." ) );
    return;
  }

  mCalendar = new KCal::CalendarLocal( QLatin1String( "UTC" ) );
  mCalendar->load( file );
}

void ICalResource::itemAdded( const Akonadi::Item & item, const Akonadi::Collection& )
{
  Q_ASSERT( item.hasPayload<IncidencePtr>() );
  IncidencePtr i = item.payload<IncidencePtr>();
  mCalendar->addIncidence( i.get() );
  Item it( item );
  it.setRemoteId( i->uid() );
  changeCommitted( it );
}

void ICalResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  kWarning( 5251 ) << "Implement me!";
  AgentBase::Observer::itemChanged( item, parts );
}

void ICalResource::itemRemoved(const Akonadi::Item & item)
{
  Incidence *i = mCalendar->incidence( item.remoteId() );
  if ( i )
    mCalendar->deleteIncidence( i );
  changeProcessed();
}

void ICalResource::retrieveCollections()
{
  Collection c;
  c.setParent( Collection::root() );
  c.setRemoteId( Settings::self()->path() );
  c.setName( name() );
  QStringList mimeTypes;
  mimeTypes << QLatin1String( "text/calendar" );
  mimeTypes += mMimeVisitor->allMimeTypes();
  c.setContentMimeTypes( mimeTypes );
  Collection::List list;
  list << c;
  collectionsRetrieved( list );
}

void ICalResource::retrieveItems( const Akonadi::Collection & col )
{
  Q_UNUSED( col );
  if ( !mCalendar )
    return;

  Incidence::List incidences = mCalendar->incidences();
  Item::List items;
  foreach ( Incidence *incidence, incidences ) {
    Item item ( mMimeVisitor->mimeType( incidence ) );
    item.setRemoteId( incidence->uid() );
    items << item;
  }
  itemsRetrieved( items );
}

AKONADI_RESOURCE_MAIN( ICalResource )

#include "icalresource.moc"
