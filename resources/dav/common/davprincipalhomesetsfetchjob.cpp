/*
    Copyright (c) 2010 Grégory Oestreicher <greg@kamago.net>

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

#include "davprincipalhomesetsfetchjob.h"

#include "davmanager.h"
#include "davprotocolbase.h"

#include <kio/davjob.h>

DavPrincipalHomeSetsFetchJob::DavPrincipalHomeSetsFetchJob( const DavUtils::DavUrl &url, QObject *parent )
  : KJob( parent ), mUrl( url )
{
}

void DavPrincipalHomeSetsFetchJob::start()
{
  QDomDocument document;

  QDomElement propfindElement = document.createElementNS( "DAV:", "propfind" );
  document.appendChild( propfindElement );

  QDomElement propElement = document.createElementNS( "DAV:", "prop" );
  propfindElement.appendChild( propElement );

  const QString homeSet = DavManager::self()->davProtocol( mUrl.protocol() )->principalHomeSet();
  const QString homeSetNS = DavManager::self()->davProtocol( mUrl.protocol() )->principalHomeSetNS();
  propElement.appendChild( document.createElementNS( homeSetNS, homeSet ) );

  KIO::DavJob *job = DavManager::self()->createPropFindJob( mUrl.url(), document );
  job->addMetaData( "PropagateHttpHeader", "true" );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( davJobFinished( KJob* ) ) );
}

QStringList DavPrincipalHomeSetsFetchJob::homeSets() const
{
  return mHomeSets;
}

void DavPrincipalHomeSetsFetchJob::davJobFinished( KJob *job )
{
  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  KIO::DavJob *davJob = qobject_cast<KIO::DavJob*>( job );

  const QString httpStatus = davJob->queryMetaData( "HTTP-Headers" ).split( "\n" ).at( 0 );

  if ( httpStatus.contains( "HTTP/1.1 5" ) ) {
    // Server-side error, unrecoverable
    setError( UserDefinedError );
    setErrorText( httpStatus );
    emitResult();
    return;
  }

  /*
   * Extract information from a document like the following :
   *
   *  <?xml version="1.0" encoding="utf-8" ?>
   *  <multistatus xmlns="DAV:" xmlns:C="urn:ietf:params:xml:ns:caldav">
   *    <response>
   *      <href>/principals/users/greg%40kamago.net/</href>
   *      <propstat>
   *        <prop>
   *          <C:calendar-home-set>
   *            <href>/greg%40kamago.net/</href>
   *          </C:calendar-home-set>
   *        </prop>
   *        <status>HTTP/1.1 200 OK</status>
   *      </propstat>
   *    </response>
   *  </multistatus>
   */

  const QString homeSet = DavManager::self()->davProtocol( mUrl.protocol() )->principalHomeSet();
  const QString homeSetNS = DavManager::self()->davProtocol( mUrl.protocol() )->principalHomeSetNS();

  const QDomDocument document = davJob->response();
  const QDomElement multistatusElement = document.documentElement();

  QDomElement responseElement = DavUtils::firstChildElementNS( multistatusElement, "DAV:", "response" );
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

    // extract home sets
    const QDomElement propElement = DavUtils::firstChildElementNS( propstatElement, "DAV:", "prop" );
    const QDomElement homeSetElement = DavUtils::firstChildElementNS( propElement, homeSetNS, homeSet );
    QDomElement hrefElement = DavUtils::firstChildElementNS( homeSetElement, "DAV:", "href" );

    while ( !hrefElement.isNull() ) {
      const QString href = hrefElement.text();
      if ( !mHomeSets.contains( href ) )
        mHomeSets << href;

      hrefElement = DavUtils::nextSiblingElementNS( hrefElement, "DAV:", "href" );
    }

    responseElement = DavUtils::nextSiblingElementNS( responseElement, "DAV:", "response" );
  }

  emitResult();
}
