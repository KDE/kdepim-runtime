/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "davitemslistjob.h"

#include "davmanager.h"
#include "davprotocolbase.h"
#include "davutils.h"

#include <kio/davjob.h>
#include <klocale.h>

#include <QtCore/QBuffer>
#include <QtXmlPatterns/QXmlQuery>

DavItemsListJob::DavItemsListJob( const DavUtils::DavUrl &url, QObject *parent )
  : KJob( parent ), mUrl( url ), mSubJobCount( 0 ), mSubJobSuccessful( false )
{
}

void DavItemsListJob::start()
{
  const DavProtocolBase *protocol = DavManager::self()->davProtocol( mUrl.protocol() );
  Q_ASSERT( protocol );
  QListIterator<QDomDocument> it( protocol->itemsQueries() );

  while ( it.hasNext() ) {
    const QDomDocument props = it.next();

    if ( DavManager::self()->davProtocol( mUrl.protocol() )->useReport() ) {
      KIO::DavJob *job = DavManager::self()->createReportJob( mUrl.url(), props );
      job->addMetaData( "PropagateHttpHeader", "true" );
      job->setProperty( "davType", "report" );
      connect( job, SIGNAL( result( KJob* ) ), this, SLOT( davJobFinished( KJob* ) ) );
    } else {
      KIO::DavJob *job = DavManager::self()->createPropFindJob( mUrl.url(), props );
      job->addMetaData( "PropagateHttpHeader", "true" );
      job->setProperty( "davType", "propFind" );
      connect( job, SIGNAL( result( KJob* ) ), this, SLOT( davJobFinished( KJob* ) ) );
    }

    ++mSubJobCount;
  }
}

DavItem::List DavItemsListJob::items() const
{
  return mItems;
}

void DavItemsListJob::davJobFinished( KJob *job )
{
  --mSubJobCount;

  if ( job->error() && !mSubJobSuccessful ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    if ( mSubJobCount == 0 )
      emitResult();
    return;
  }

  KIO::DavJob *davJob = qobject_cast<KIO::DavJob*>( job );

  const int responseCode = davJob->queryMetaData( "responsecode" ).toInt();

  if ( responseCode > 499 && responseCode < 600 && !mSubJobSuccessful ) {
    // Server-side error, unrecoverable
    setError( UserDefinedError );
    setErrorText( i18n( "The server encountered an error that prevented it from completing your request" ) );
    if ( mSubJobCount == 0 )
      emitResult();
    return;
  } else if ( responseCode > 399 && responseCode < 500 && !mSubJobSuccessful ) {
    // User-side error
    setError( UserDefinedError );
    setErrorText( i18n( "There was a problem with the request : error %1.", responseCode ) );
    if ( mSubJobCount == 0 )
      emitResult();
    return;
  }

  if ( !mSubJobSuccessful ) {
    setError( 0 ); // nope, everything went fine
    mSubJobSuccessful = true;
  }

  /*
   * Extract data from a document like the following:
   *
   * <multistatus xmlns="DAV:">
   *   <response xmlns="DAV:">
   *     <href xmlns="DAV:">/caldav.php/test1.user/home/KOrganizer-166749289.780.ics</href>
   *     <propstat xmlns="DAV:">
   *       <prop xmlns="DAV:">
   *         <getetag xmlns="DAV:">"b4bbea0278f4f63854c4167a7656024a"</getetag>
   *       </prop>
   *       <status xmlns="DAV:">HTTP/1.1 200 OK</status>
   *     </propstat>
   *   </response>
   *   <response xmlns="DAV:">
   *     <href xmlns="DAV:">/caldav.php/test1.user/home/KOrganizer-399416366.464.ics</href>
   *     <propstat xmlns="DAV:">
   *       <prop xmlns="DAV:">
   *         <getetag xmlns="DAV:">"52eb129018398a7da4f435b2bc4c6cd5"</getetag>
   *       </prop>
   *       <status xmlns="DAV:">HTTP/1.1 200 OK</status>
   *     </propstat>
   *   </response>
   * </multistatus>
   */

  const QDomDocument document = davJob->response();
  const QDomElement documentElement = document.documentElement();

  QDomElement responseElement = DavUtils::firstChildElementNS( documentElement, "DAV:", "response" );
  while ( !responseElement.isNull() ) {

    QDomElement propstatElement;

    // check for the valid propstat, without giving up on first error
    {
      const QDomNodeList propstats = responseElement.elementsByTagNameNS( "DAV:", "propstat" );
      for ( uint i = 0; i < propstats.length(); ++i ) {
        const QDomElement propstatCandidate = propstats.item( i ).toElement();
        const QDomElement statusElement = DavUtils::firstChildElementNS( propstatCandidate, "DAV:", "status" );
        if ( statusElement.text().contains( "200" ) ) {
          propstatElement = propstatCandidate;
        }
      }
    }

    if ( propstatElement.isNull() ) {
      responseElement = DavUtils::nextSiblingElementNS( responseElement, "DAV:", "response" );
      continue;
    }

    const QDomElement propElement = DavUtils::firstChildElementNS( propstatElement, "DAV:", "prop" );

    // check whether it is a dav collection ...
    const QDomElement resourcetypeElement = DavUtils::firstChildElementNS( propElement, "DAV:", "resourcetype" );
    if ( !responseElement.isNull() ) {
      const QDomElement collectionElement = DavUtils::firstChildElementNS( resourcetypeElement, "DAV:", "collection" );
      if ( !collectionElement.isNull() ) {
        responseElement = DavUtils::nextSiblingElementNS( responseElement, "DAV:", "response" );
        continue;
      }
    }

    // ... if not it is an item
    DavItem item;

    // extract path
    const QDomElement hrefElement = DavUtils::firstChildElementNS( responseElement, "DAV:", "href" );
    const QString href = hrefElement.text();

    KUrl url = davJob->url();
    url.setUser( QString() );
    if ( href.startsWith( '/' ) ) {
      // href is only a path, use request url to complete
      url.setEncodedPath( href.toAscii() );
    } else {
      // href is a complete url
      KUrl tmpUrl( href );
      url = tmpUrl;
    }

    QString itemUrl = url.prettyUrl();
    if ( mSeenUrls.contains( itemUrl ) ) {
      responseElement = DavUtils::nextSiblingElementNS( responseElement, "DAV:", "response" );
      continue;
    }

    mSeenUrls << itemUrl;
    item.setUrl( itemUrl );

    // extract etag
    const QDomElement getetagElement = DavUtils::firstChildElementNS( propElement, "DAV:", "getetag" );

    item.setEtag( getetagElement.text() );

    mItems << item;

    responseElement = DavUtils::nextSiblingElementNS( responseElement, "DAV:", "response" );
  }

  if ( mSubJobCount == 0 )
    emitResult();
}

#include "davitemslistjob.moc"
