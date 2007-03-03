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
#include <kcal/icalformat.h>

#include <kfiledialog.h>
#include <klocale.h>

#include <QtCore/QDebug>
#include <QtDBus/QDBusConnection>

using namespace Akonadi;
using namespace KCal;

ICalResource::ICalResource( const QString &id )
    :ResourceBase( id ), mCalendar( 0 )
{
  loadFile();
  synchronize();
}

ICalResource::~ ICalResource()
{
  delete mCalendar;
}

bool ICalResource::requestItemDelivery( const Akonadi::DataReference &ref, int type, const QDBusMessage &msg )
{
  qDebug() << "ICalResource::requestItemDelivery()";
  Incidence *incidence = mCalendar->incidence( ref.externalUrl().toString() );
  if ( !incidence ) {
    error( QString("Incidence with uid '%1' not found!").arg( ref.externalUrl().toString() ) );
    return false;
  } else {
    ICalFormat format;
    QByteArray data = format.toString( incidence ).toUtf8();

    ItemStoreJob *job = new ItemStoreJob( ref, session() );
    job->setData( data );
    return deliverItem( job, msg );
  }
}

void ICalResource::synchronize()
{
  if ( !mCalendar )
    return;

  changeStatus( Syncing, i18n("Syncing with ICal file.") );

  CollectionListJob *ljob = new CollectionListJob( Collection::root(), false, session() );
  ljob->setResource( identifier() );
  ljob->exec();

  if ( ljob->collections().count() != 1 ) {
    changeStatus( Error, i18n("No or more than one collection found!") );
    return;
  }
  QString col = ljob->collections().first()->name();
  delete ljob;

  CollectionModifyJob *modify = new CollectionModifyJob( col, session() );
  QList<QByteArray> mimeTypes;
  mimeTypes << "text/calendar";
  modify->setContentTypes( mimeTypes );
  modify->setCachePolicy( 1 ); // ### just for testing
  if ( !modify->exec() ) {
    changeStatus( Error, i18n("Unable to set properties of collection '%1': %2", col, modify->errorString()) );
    return;
  }

  ItemFetchJob *fetch = new ItemFetchJob( col, session() );
  if ( !fetch->exec() ) {
    changeStatus( Error, i18n("Unable to fetch listing of collection '%1': %2", col, fetch->errorString()) );
    return;
  }

  changeProgress( 0 );

  Item::List items = fetch->items();
  delete fetch;
  Incidence::List incidences = mCalendar->incidences();

  int counter = 0;
  foreach ( Incidence *incidence, incidences ) {
    QString uid = incidence->uid();
    bool found = false;
    foreach ( Item* item, items ) {
      if ( item->reference().externalUrl().toString() == uid ) {
        found = true;
        break;
      }
    }
    if ( found )
      continue;
    ItemAppendJob *append = new ItemAppendJob( col, QByteArray(), "text/calendar", session() );
    append->setRemoteId( uid );
    if ( !append->exec() ) {
      changeProgress( 0 );
      changeStatus( Error, i18n("Appending new incidence failed: %1", append->errorString()) );
      return;
    }
    delete append;

    counter++;
    int percentage = (counter * 100) / incidences.count();
    changeProgress( percentage );
  }

  changeStatus( Ready, QString() );
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
  // TODO: delete existing items
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

void ICalResource::itemAdded(const Akonadi::DataReference & ref)
{
  ItemFetchJob* fetch = new ItemFetchJob( ref, session() );
  // TODO: error handling
  if ( fetch->exec() && !fetch->items().isEmpty() ) {
    ICalFormat format;
    Item *item = fetch->items().first();
    Incidence* i = format.fromString( QString::fromUtf8( item->data() ) );
    if ( i ) {
      mCalendar->addIncidence( i );
      DataReference r( ref.persistanceID(), i->uid() );
      changesCommitted( r );
    }
  }
}

void ICalResource::itemChanged(const Akonadi::DataReference & ref)
{
  qWarning() << "Implement me!";
}

void ICalResource::itemRemoved(const Akonadi::DataReference & ref)
{
  Incidence *i = mCalendar->incidence( ref.externalUrl().toString() );
  if ( i )
    mCalendar->deleteIncidence( i );
}

#include "icalresource.moc"
