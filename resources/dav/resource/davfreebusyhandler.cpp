/*
    Copyright (c) 2011 Grégory Oestreicher <greg@kamago.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "davfreebusyhandler.h"

#include "davcollectionsfetchjob.h"
#include "davmanager.h"
#include "davprincipalsearchjob.h"
#include "settings.h"

#include <KCalCore/ICalFormat>
#include <KDateTime>
#include <klocale.h>
#include <kio/davjob.h>
#include <kio/job.h>
#include <kcal/alarm.h>

DavFreeBusyHandler::DavFreeBusyHandler( QObject* parent )
  : QObject( parent ), mNextRequestId( 0 )
{
}

void DavFreeBusyHandler::canHandleFreeBusy( const QString& email )
{
  DavUtils::DavUrl::List urls = Settings::self()->configuredDavUrls();
  foreach ( const DavUtils::DavUrl &url, urls ) {
    if ( url.protocol() == DavUtils::CalDav ) {
      ++mRequestsTracker[email].handlingJobCount;
      DavPrincipalSearchJob *job = new DavPrincipalSearchJob( url, DavPrincipalSearchJob::EmailAddress, email );
      job->setProperty( "email", QVariant::fromValue( email ) );
      job->setProperty( "url", QVariant::fromValue( url.url().url() ) );
      job->fetchProperty( "schedule-inbox-URL", "urn:ietf:params:xml:ns:caldav" );
      connect( job, SIGNAL( result(KJob*) ), this, SLOT( onPrincipalSearchJobFinished(KJob*) ) );
      job->start();
    }
  }
}

void DavFreeBusyHandler::retrieveFreeBusy( const QString& email, const KDateTime& start, const KDateTime& end )
{
  if ( !mPrincipalScheduleOutbox.contains( email ) ) {
    emit freeBusyRetrieved( email, QString(), false, i18n( "No schedule-outbox found for %1" ).arg( email ) );
    return;
  }

  KCalCore::FreeBusy::Ptr fb( new KCalCore::FreeBusy( start, end ) );
  KCalCore::Attendee::Ptr att( new KCalCore::Attendee( QString(), email ) );
  fb->addAttendee( att );

  KCalCore::ICalFormat formatter;
  QByteArray fbData = formatter.createScheduleMessage( fb, KCalCore::iTIPRequest ).toUtf8();

  foreach ( const QString &outbox, mPrincipalScheduleOutbox[email] ) {
    ++mRequestsTracker[email].retrievalJobCount;
    uint requestId = mNextRequestId++;

    KUrl url( outbox );
    KIO::StoredTransferJob *job = KIO::storedHttpPost( fbData, url );
    job->addMetaData( "content-type", "text/calendar" );
    job->setProperty( "email", QVariant::fromValue( email ) );
    job->setProperty( "request-id", QVariant::fromValue( requestId ) );
    connect( job, SIGNAL( result(KJob*) ), this, SLOT( onRetrieveFreeBusyJobFinished(KJob*) ) );
    job->start();
  }
}

void DavFreeBusyHandler::onPrincipalSearchJobFinished( KJob* job )
{
  QString email = job->property( "email" ).toString();
  int handlingJobCount = --mRequestsTracker[email].handlingJobCount;

  if ( job->error() ) {
    if ( handlingJobCount == 0 && !mRequestsTracker[email].handlingJobSuccessful )
      emit handlesFreeBusy( email, false );
    return;
  }

  DavPrincipalSearchJob *davJob = qobject_cast<DavPrincipalSearchJob*>( job );
  QList<DavPrincipalSearchJob::Result> results = davJob->results();

  if ( results.isEmpty() ) {
    if ( handlingJobCount == 0 && !mRequestsTracker[email].handlingJobSuccessful )
      emit handlesFreeBusy( email, false );
    return;
  }

  mRequestsTracker[email].handlingJobSuccessful = true;

  foreach ( const DavPrincipalSearchJob::Result &result, results ) {
    kDebug() << result.value;
    KUrl url( davJob->property( "url" ).toString() );
    if ( result.value.startsWith( '/' ) ) {
      // href is only a path, use request url to complete
      url.setEncodedPath( result.value.toAscii() );
    } else {
      // href is a complete url
      KUrl tmpUrl( result.value );
      url = tmpUrl;
    }

    if ( !mPrincipalScheduleOutbox[email].contains( url.url() ) )
      mPrincipalScheduleOutbox[email] << url.url();
  }

  if ( handlingJobCount == 0 )
    emit handlesFreeBusy( email, true );
}

void DavFreeBusyHandler::onRetrieveFreeBusyJobFinished( KJob* job )
{
  QString email = job->property( "email" ).toString();
  uint requestId = job->property( "request-id" ).toUInt();
  int retrievalJobCount = --mRequestsTracker[email].retrievalJobCount;

  if ( job->error() ) {
    if ( retrievalJobCount == 0 && !mRequestsTracker[email].retrievalJobSuccessful )
      emit( freeBusyRetrieved( email, QString(), false, job->errorString() ) );
    return;
  }

  /*
   * Extract info from a document like the following:
   * <?xml version="1.0" encoding="utf-8" ?>
   * <C:schedule-response xmlns:D="DAV:" xmlns:C="urn:ietf:params:xml:ns:caldav">
   *   <C:response>
   *     <C:recipient>
   *       <D:href>mailto:wilfredo@example.com<D:href>
   *     </C:recipient>
   *     <C:request-status>2.0;Success</C:request-status>
   *     <C:calendar-data>BEGIN:VCALENDAR
   * VERSION:2.0
   * PRODID:-//Example Corp.//CalDAV Server//EN
   * METHOD:REPLY
   * BEGIN:VFREEBUSY
   * UID:4FD3AD926350
   * DTSTAMP:20090602T200733Z
   * DTSTART:20090602T000000Z
   * DTEND:20090604T000000Z
   * ORGANIZER;CN="Cyrus Daboo":mailto:cyrus@example.com
   * ATTENDEE;CN="Wilfredo Sanchez Vega":mailto:wilfredo@example.com
   * FREEBUSY;FBTYPE=BUSY:20090602T110000Z/20090602T120000Z
   * FREEBUSY;FBTYPE=BUSY:20090603T170000Z/20090603T180000Z
   * END:VFREEBUSY
   * END:VCALENDAR
   *     </C:calendar-data>
   *   </C:response>
   *   <C:response>
   *     <C:recipient>
   *       <D:href>mailto:mike@example.org<D:href>
   *     </C:recipient>
   *     <C:request-status>3.7;Invalid calendar user</C:request-status>
   *   </C:response>
   * </C:schedule-response>
   */

  KIO::StoredTransferJob *postJob = qobject_cast<KIO::StoredTransferJob*>( job );
  QDomDocument response;
  response.setContent( postJob->data(), true );

  QDomElement scheduleResponse = response.documentElement();

  // We are only expecting one response tag
  QDomElement responseElement = DavUtils::firstChildElementNS( scheduleResponse, "urn:ietf:params:xml:ns:caldav", "response" );
  if ( responseElement.isNull() ) {
    if ( retrievalJobCount == 0 && !mRequestsTracker[email].retrievalJobSuccessful )
      emit( freeBusyRetrieved( email, QString(), false, i18n( "Invalid response from the server" ) ) );
    return;
  }

  // We can load directly the calendar-data and use its content to create
  // an incidence base that will give us everything we need to test
  // the success
  QDomElement calendarDataElement = DavUtils::firstChildElementNS( responseElement, "urn:ietf:params:xml:ns:caldav", "calendar-data" );
  if ( calendarDataElement.isNull() ) {
    if ( retrievalJobCount == 0 && !mRequestsTracker[email].retrievalJobSuccessful )
      emit( freeBusyRetrieved( email, QString(), false, i18n( "Invalid response from the server" ) ) );
    return;
  }

  QString rawData = calendarDataElement.text();

  KCalCore::ICalFormat format;
  KCalCore::FreeBusy::Ptr fb = format.parseFreeBusy( rawData );
  if ( fb.isNull() ) {
    if ( retrievalJobCount == 0 && !mRequestsTracker[email].retrievalJobSuccessful )
      emit( freeBusyRetrieved( email, QString(), false, i18n( "Unable to parse free-busy data received" ) ) );
    return;
  }

  // We're safe now
  mRequestsTracker[email].retrievalJobSuccessful = true;

//   fb->clearAttendees();
  if ( mRequestsTracker[email].resultingFreeBusy[requestId].isNull() )
    mRequestsTracker[email].resultingFreeBusy[requestId] = fb;
  else
    mRequestsTracker[email].resultingFreeBusy[requestId]->merge( fb );

  if ( retrievalJobCount == 0 ) {
    QString fbStr = format.createScheduleMessage( mRequestsTracker[email].resultingFreeBusy[requestId],
                                                  KCalCore::iTIPRequest );
    emit freeBusyRetrieved( email, fbStr, true, QString() );
  }
}
