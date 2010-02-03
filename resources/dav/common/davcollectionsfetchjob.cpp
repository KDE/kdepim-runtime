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

#include "davcollectionsfetchjob.h"

#include "davmanager.h"
#include "davprotocolbase.h"

#include <kdebug.h>
#include <kio/davjob.h>
#include <klocale.h>

#include <QtCore/QBuffer>
#include <QtXmlPatterns/QXmlQuery>

DavCollectionsFetchJob::DavCollectionsFetchJob( const KUrl &url, QObject *parent )
  : KJob( parent ), mUrl( url )
{
}

void DavCollectionsFetchJob::start()
{
  const QDomDocument collectionQuery = DavManager::self()->davProtocol()->collectionsQuery();

  KIO::DavJob *job = DavManager::self()->createPropFindJob( mUrl, collectionQuery );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( davJobFinished( KJob* ) ) );
}

DavCollection::List DavCollectionsFetchJob::collections() const
{
  return mCollections;
}

void DavCollectionsFetchJob::davJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  KIO::DavJob *davJob = qobject_cast<KIO::DavJob*>( job );

  QByteArray resp( davJob->response().toByteArray() );
  QBuffer buffer( &resp );
  buffer.open( QIODevice::ReadOnly );

  QXmlQuery xquery;
  if ( !xquery.setFocus( &buffer ) ) {
    setError( UserDefinedError );
    setErrorText( i18n( "Error setting focus for XQuery" ) );
    emitResult();
    return;
  }

  xquery.setQuery( DavManager::self()->davProtocol()->collectionsXQuery() );
  if ( !xquery.isValid() ) {
    setError( UserDefinedError );
    setErrorText( i18n( "Invalid XQuery submitted by DAV implementation" ) );
    emitResult();
    return;
  }

  QString responsesStr;
  xquery.evaluateTo( &responsesStr );
  responsesStr.prepend( "<responses>" );
  responsesStr.append( "</responses>" );

  QDomDocument document;
  if ( !document.setContent( responsesStr ) ) {
    setError( UserDefinedError );
    setErrorText( i18n( "Invalid responses from backend" ) );
    emitResult();
    return;
  }

  /*
   * Extract information from a document like the following:
   *
   * <responses>
   *   <response xmlns="DAV:">
   *     <href xmlns="DAV:">/caldav.php/test1.user/home/</href>
   *     <propstat xmlns="DAV:">
   *       <prop xmlns="DAV:">
   *         <C:supported-calendar-component-set xmlns:C="urn:ietf:params:xml:ns:caldav">
   *           <C:comp xmlns:C="urn:ietf:params:xml:ns:caldav" name="VEVENT"/>
   *           <C:comp xmlns:C="urn:ietf:params:xml:ns:caldav" name="VTODO"/>
   *           <C:comp xmlns:C="urn:ietf:params:xml:ns:caldav" name="VJOURNAL"/>
   *           <C:comp xmlns:C="urn:ietf:params:xml:ns:caldav" name="VTIMEZONE"/>
   *           <C:comp xmlns:C="urn:ietf:params:xml:ns:caldav" name="VFREEBUSY"/>
   *         </C:supported-calendar-component-set>
   *         <resourcetype xmlns="DAV:">
   *           <collection xmlns="DAV:"/>
   *           <C:calendar xmlns:C="urn:ietf:params:xml:ns:caldav"/>
   *           <C:schedule-calendar xmlns:C="urn:ietf:params:xml:ns:caldav"/>
   *         </resourcetype>
   *         <displayname xmlns="DAV:">Test1 User</displayname>
   *       </prop>
   *       <status xmlns="DAV:">HTTP/1.1 200 OK</status>
   *     </propstat>
   *   </response>
   * </responses>
   */

  const QDomElement responsesElement = document.documentElement();

  QDomElement responseElement = responsesElement.firstChildElement( "response" );
  while ( !responseElement.isNull() ) {

    const QDomElement propstatElement;

    // check for the valid propstat, without giving up on first error
    {
      const QDomNodeList propstats = responseElement.elementsByTagNameNS( "DAV:", "propstat" );
      for( int i = 0; i < propstats.length(); ++i ) {
        const QDomElement propstatCandidate = propstats.item( i ).toElement();
        const QDomElement statusElement = propstatCandidate.firstChildElement( "status" );
        if ( statusElement.text().contains( "200" ) ) {
          propstatElement = propstatCandidate;
        }
      }
    }
    
    if( propstatElement.isNull() ) {
      responseElement = responseElement.nextSiblingElement( "response" );
      continue;
    }

    // extract url
    const QDomElement hrefElement = responseElement.firstChildElement( "href" );
    if ( hrefElement.isNull() ) {
      responseElement = responseElement.nextSiblingElement( "response" );
      continue;
    }

    QString href = hrefElement.text();
    if ( !href.endsWith( '/' ) )
      href.append( '/' );

    KUrl url = davJob->url();
    if ( href.startsWith( '/' ) ) {
      // href is only a path, use request url to complete
      url.setEncodedPath( href.toAscii() );
    } else {
      // href is a complete url, so just preserve the user information
      KUrl tmpUrl( href );
      tmpUrl.setUser( url.user() );
      url = tmpUrl;
    }

    // extract display name
    const QDomElement propElement = propstatElement.firstChildElement( "prop" );
    const QDomElement displaynameElement = propElement.firstChildElement( "displayname" );

    const QString displayName = displaynameElement.text();

    mCollections << DavCollection( url.prettyUrl(), displayName );

    responseElement = responseElement.nextSiblingElement( "response" );
  }

  emitResult();
}

#include "davcollectionsfetchjob.moc"
