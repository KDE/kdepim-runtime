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

#include <kcal/calendarlocal.h>
#include <kcal/incidence.h>
#include <kcal/icalformat.h>

#include <klocale.h>

#include <QDebug>

using namespace PIM;
using namespace KCal;

ICalResource::ICalResource( const QString &id )
    :ResourceBase( id ), mCalendar( 0 )
{
  // ### just for testing
  mCalendar = new KCal::CalendarLocal( "UTC" );
  mCalendar->load( "akonadi_ical_test.ics" );

  synchronize();
}

PIM::ICalResource::~ ICalResource()
{
  delete mCalendar;
}

void ICalResource::done( PIM::Job * job )
{
  if ( job->error() ) {
    error( "Error while creating item: " + job->errorText() );
  } else {
    qDebug() << "Done!";
  }
  job->deleteLater();
}

void PIM::ICalResource::setParameters(const QByteArray &path, const QByteArray &filename, const QByteArray &mimetype )
{
}

bool ICalResource::requestItemDelivery( const QString & uid, const QString &remoteId, const QString & collection, int type )
{
  Incidence *incidence = mCalendar->incidence( remoteId );
  if ( !incidence ) {
    error( QString("Incidence with uid '%1' not found!").arg( remoteId ) );
    return false;
  }

  ICalFormat format;
  QByteArray data = format.toString( incidence ).toUtf8();

  ItemStoreJob *job = new ItemStoreJob( DataReference( uid, remoteId ), this );
  job->setData( data );
  connect( job, SIGNAL(done(PIM::Job*)), SLOT(done(PIM::Job*)) );
  job->start();

  return true;
}

void PIM::ICalResource::synchronize()
{
  changeStatus( Syncing, i18n("Syncing with ICal file.") );

  CollectionListJob *ljob = new CollectionListJob( Collection::root(), false );
  ljob->setResource( identifier() );
  ljob->exec();

  if ( ljob->collections().count() != 1 ) {
    changeStatus( Error, i18n("No or more than one collection found!") );
    return;
  }
  QByteArray col = ljob->collections().first()->name().toUtf8();
  delete ljob;

  CollectionModifyJob *modify = new CollectionModifyJob( col, this );
  QList<QByteArray> mimeTypes;
  mimeTypes << "text/calendar";
  modify->setContentTypes( mimeTypes );
  if ( !modify->exec() ) {
    changeStatus( Error, i18n("Unable to set properties of collection '%1': %2", QString::fromUtf8(col), modify->errorText()) );
    return;
  }

  ItemFetchJob *fetch = new ItemFetchJob( col, this );
  if ( !fetch->exec() ) {
    changeStatus( Error, i18n("Unable to fetch listing of collection '%1': %2", QString::fromUtf8(col), fetch->errorText()) );
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
    ItemAppendJob *append = new ItemAppendJob( col, QByteArray(), "text/calendar", this );
    append->setRemoteId( uid );
    if ( !append->exec() ) {
      changeProgress( 0 );
      changeStatus( Error, i18n("Appending new incidence failed: %1", append->errorText()) );
      return;
    }
    delete append;

    counter++;
    int percentage = (counter * 100) / incidences.count();
    changeProgress( percentage );
  }

  changeStatus( Ready, QString() );
}

void PIM::ICalResource::aboutToQuit()
{
  mCalendar->save();
}

#include "icalresource.moc"
