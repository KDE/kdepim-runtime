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
#include <libakonadi/itemappendjob.h>

#include <kcal/calendarlocal.h>
#include <kcal/incidence.h>
#include <kcal/icalformat.h>

#include <QDebug>

using namespace PIM;
using namespace KCal;

ICalResource::ICalResource( const QString &id )
    :ResourceBase( id )
{
  // ### just for testing
  mCalendar = new KCal::CalendarLocal( "UTC" );
  mCalendar->load( "akonadi_ical_test.ics" );
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

bool ICalResource::requestItemDelivery( const QString & uid, const QString & collection, int type )
{
  Incidence *incidence = mCalendar->incidence( uid );
  if ( !incidence ) {
    error( QString("Incidence with uid '%1' not found!").arg( uid ) );
    return false;
  }

  ICalFormat format;
  QByteArray data = format.toString( incidence ).toUtf8();

  ItemAppendJob *job = new ItemAppendJob( collection.toUtf8(), data, "text/calendar", this );
  connect( job, SIGNAL(done(PIM::Job*)), SLOT(done(PIM::Job*)) );
  job->start();

  return true;
}

#include "icalresource.moc"
