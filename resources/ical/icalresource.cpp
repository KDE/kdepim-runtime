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
#include <libakonadi/itemserializer.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/session.h>

#include <kcal/calendarlocal.h>
#include <kcal/incidence.h>

#include <kfiledialog.h>
#include <klocale.h>

#include <QtCore/QDebug>
#include <QtDBus/QDBusConnection>

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

bool ICalResource::requestItemDelivery( const Akonadi::DataReference &ref, const QStringList &parts, const QDBusMessage &msg )
{
  Q_UNUSED( parts );
  qDebug() << "ICalResource::requestItemDelivery()";
  IncidencePtr incidence( mCalendar->incidence( ref.remoteId() )->clone() );
  if ( !incidence ) {
    error( QString("Incidence with uid '%1' not found!").arg( ref.remoteId() ) );
    return false;
  } else {
    Item item( ref );
    item.setMimeType( "text/calendar" );
    item.setPayload<IncidencePtr>( incidence );

    ItemStoreJob *job = new ItemStoreJob( item, session() );
    job->storePayload();
    return deliverItem( job, msg );
  }
}

void ICalResource::aboutToQuit()
{
  QString fileName = settings()->value( "General/Path" ).toString();
  if ( fileName.isEmpty() )
    error( i18n("No filename specified.") );
  else if ( !mCalendar->save( fileName ) )
    error( i18n("Failed to save calendar file to %1", fileName ) );
}

void ICalResource::configure()
{
  QString oldFile = settings()->value( "General/Path" ).toString();
  KUrl url;
  if ( !oldFile.isEmpty() )
    url = KUrl::fromPath( oldFile );
  else
    url = KUrl::fromPath( QDir::homePath() );
  QString newFile = KFileDialog::getOpenFileName( url, "*.ics *.ical|iCal Calendar File", 0, i18n("Select Calendar") );
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
  DataReference r( item.reference().id(), i->uid() );
  changesCommitted( r );
}

void ICalResource::itemChanged( const Akonadi::Item&, const QStringList& )
{
  qWarning() << "Implement me!";
}

void ICalResource::itemRemoved(const Akonadi::DataReference & ref)
{
  Incidence *i = mCalendar->incidence( ref.remoteId() );
  if ( i )
    mCalendar->deleteIncidence( i );
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
  c.setCachePolicyId( 1 ); // ### just for testing
  Collection::List list;
  list << c;
  collectionsRetrieved( list );
}

void ICalResource::synchronizeCollection(const Akonadi::Collection & col)
{
  if ( !mCalendar )
    return;

  ItemFetchJob *fetch = new ItemFetchJob( col, session() );
  if ( !fetch->exec() ) {
    changeStatus( Error, i18n("Unable to fetch listing of collection '%1': %2", col.name(), fetch->errorString()) );
    return;
  }

  changeProgress( 0 );

  Item::List items = fetch->items();
  Incidence::List incidences = mCalendar->incidences();

  int counter = 0;
  foreach ( Incidence *incidence, incidences ) {
    QString uid = incidence->uid();
    bool found = false;
    foreach ( Item item, items ) {
      if ( item.reference().remoteId() == uid ) {
        found = true;
        break;
      }
    }
    if ( found )
      continue;
    Item item( DataReference( -1, uid ) );
    item.setMimeType( "text/calendar" );
    ItemAppendJob *append = new ItemAppendJob( item, col, session() );
    if ( !append->exec() ) {
      changeProgress( 0 );
      changeStatus( Error, i18n("Appending new incidence failed: %1", append->errorString()) );
      return;
    }

    counter++;
    int percentage = (counter * 100) / incidences.count();
    changeProgress( percentage );
  }

  collectionSynchronized();
}

#include "icalresource.moc"
