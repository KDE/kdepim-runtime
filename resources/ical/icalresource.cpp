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
#include <libakonadi/collectionlistjob.h>
#include <libakonadi/collectionmodifyjob.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/session.h>

#include <kcal/calendarlocal.h>
#include <kcal/incidence.h>

#include <kdebug.h>
#include <kfiledialog.h>
#include <klocale.h>

#include <boost/shared_ptr.hpp>

using namespace Akonadi;
using namespace KCal;

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

ICalResource::ICalResource( const QString &id )
    :ResourceBase( id ), mCalendar( 0 )
{
  loadFile();
//   synchronize();
}

ICalResource::~ ICalResource()
{
  delete mCalendar;
}

bool ICalResource::retrieveItem( const Akonadi::Item &item, const QStringList &parts )
{
  Q_UNUSED( parts );
  kDebug( 5251 ) << "Item:" << item.url();
  const QString rid = item.reference().remoteId();
  IncidencePtr incidence( mCalendar->incidence( rid )->clone() );
  if ( !incidence ) {
    error( i18n("Incidence with uid '%1' not found!", rid ) );
    return false;
  }
  Item i = item;
  i.setMimeType( "text/calendar" );
  i.setPayload<IncidencePtr>( incidence );
  itemRetrieved( i );
  return true;
}

void ICalResource::aboutToQuit()
{
  QString fileName = settings()->value( "General/Path" ).toString();
  if ( fileName.isEmpty() )
    error( i18n("No filename specified.") );
  else if ( !mCalendar->save( fileName ) )
    error( i18n("Failed to save calendar file to %1", fileName ) );
}

void ICalResource::configure( WId windowId )
{
  QString oldFile = settings()->value( "General/Path" ).toString();
  KUrl url;
  if ( !oldFile.isEmpty() )
    url = KUrl::fromPath( oldFile );
  else
    url = KUrl::fromPath( QDir::homePath() );
  QString newFile = KFileDialog::getOpenFileNameWId( url, "*.ics *.ical|" + i18nc("Filedialog filter for *.ics *.ical", "iCal Calendar File"), windowId, i18n("Select Calendar") );
  if ( newFile.isEmpty() )
    return;
  if ( oldFile == newFile )
    return;
  settings()->setValue( "General/Path", newFile );
  loadFile();
  synchronize();
}

void ICalResource::loadFile()
{
  delete mCalendar;
  mCalendar = 0;
  QString file = settings()->value( "General/Path" ).toString();
  if ( file.isEmpty() ) {
    changeStatus( Error, i18n( "No iCal file specified." ) );
    return;
  }

  mCalendar = new KCal::CalendarLocal( "UTC" );
  mCalendar->load( file );
}

void ICalResource::itemAdded( const Akonadi::Item & item, const Akonadi::Collection& )
{
  Q_ASSERT( item.hasPayload<IncidencePtr>() );
  IncidencePtr i = item.payload<IncidencePtr>();
  mCalendar->addIncidence( i.get() );
  Item it( item );
  it.setRemoteId( i->uid() );
  changesCommitted( it );
}

void ICalResource::itemChanged( const Akonadi::Item &item, const QStringList &parts )
{
  kWarning( 5251 ) << "Implement me!";
  ResourceBase::itemChanged( item, parts );
}

void ICalResource::itemRemoved(const Akonadi::DataReference & ref)
{
  Incidence *i = mCalendar->incidence( ref.remoteId() );
  if ( i )
    mCalendar->deleteIncidence( i );
  changeProcessed();
}

void ICalResource::retrieveCollections()
{
  Collection c;
  c.setParent( Collection::root() );
  c.setRemoteId( settings()->value( "General/Path" ).toString() );
  c.setName( name() );
  QStringList mimeTypes;
  mimeTypes << "text/calendar";
  c.setContentTypes( mimeTypes );
  Collection::List list;
  list << c;
  collectionsRetrieved( list );
}

void ICalResource::retrieveItems(const Akonadi::Collection & col, const QStringList &parts)
{
  Q_UNUSED( col );
  Q_UNUSED( parts );
  if ( !mCalendar )
    return;

  Incidence::List incidences = mCalendar->incidences();
  Item::List items;
  foreach ( Incidence *incidence, incidences ) {
    Item item( DataReference( -1, incidence->uid() ) );
    item.setMimeType( "text/calendar" );
    items << item;
  }
  itemsRetrieved( items );
}

#include "icalresource.moc"
