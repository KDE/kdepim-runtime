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

#include <kdebug.h>
#include <kio/davjob.h>
#include <klocale.h>

#include <QtCore/QBuffer>
#include <QtXmlPatterns/QXmlQuery>

DavItemsListJob::DavItemsListJob( const DavCollection &collection, QObject *parent )
  : KJob( parent ), mCollection( collection ), mSubJobCount( 0 )
{
}

void DavItemsListJob::start()
{
  QListIterator<QDomDocument> it( DavManager::self()->davProtocol()->itemsQueries() );

  while ( it.hasNext() ) {
    const QDomDocument props = it.next();

    if ( DavManager::self()->davProtocol()->useReport() ) {
      KIO::DavJob *job = DavManager::self()->createReportJob( mCollection.url(), props );
      job->setProperty( "davType", "report" );
      connect( job, SIGNAL( result( KJob* ) ), this, SLOT( davJobFinished( KJob* ) ) );
    } else {
      KIO::DavJob *job = DavManager::self()->createPropFindJob( mCollection.url(), props );
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

  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  KIO::DavJob *davJob = qobject_cast<KIO::DavJob*>( job );

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

  QDomElement responseElement = documentElement.firstChildElement( "response" );
  while ( !responseElement.isNull() ) {

    const QDomElement propstatElement = responseElement.firstChildElement( "propstat" );
    const QDomElement propElement = propstatElement.firstChildElement( "prop" );

    // check for error
    const QDomElement statusElement = propstatElement.firstChildElement( "status" );
    if ( !statusElement.text().contains( "200" ) ) {
      responseElement = responseElement.nextSiblingElement( "response" );
      continue;
    }

    // check whether it is a dav collection ...
    const QDomElement resourcetypeElement = propElement.firstChildElement( "resourcetype" );
    if ( !responseElement.isNull() ) {
      const QDomElement collectionElement = resourcetypeElement.firstChildElement( "collection" );
      if ( !collectionElement.isNull() ) {
        responseElement = responseElement.nextSiblingElement( "response" );
        continue;
      }
    }

    // ... if not it is an item
    DavItem item;

    // extract path
    const QDomElement hrefElement = responseElement.firstChildElement( "href" );
    const QString href = hrefElement.text();

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

    item.setUrl( url.prettyUrl() );

    // extract etag
    const QDomElement getetagElement = propElement.firstChildElement( "getetag" );

    item.setEtag( getetagElement.text() );

    mItems << item;

    responseElement = responseElement.nextSiblingElement( "response" );
  }

  if ( mSubJobCount == 0 )
    emitResult();
}

#include "davitemslistjob.moc"
