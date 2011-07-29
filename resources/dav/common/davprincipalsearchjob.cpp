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

#include "davprincipalsearchjob.h"

#include "davmanager.h"

#include <kio/davjob.h>
#include <klocale.h>
#include <kdebug.h>

DavPrincipalSearchJob::DavPrincipalSearchJob( const DavUtils::DavUrl& url, DavPrincipalSearchJob::FilterType type,
                                              const QString& filter, QObject* parent )
  : KJob( parent ), mUrl( url ), mType( type), mFilter( filter ), mPrincipalPropertySearchSubJobCount( 0 ),
    mPrincipalPropertySearchSubJobSuccessful( false )
{
}

void DavPrincipalSearchJob::fetchProperty( const QString& name, const QString& ns )
{
  QString propNamespace = ns;
  if ( propNamespace.isEmpty() )
    propNamespace = "DAV:";

  mFetchProperties << QPair<QString, QString>( propNamespace, name );
}

void DavPrincipalSearchJob::start()
{
  /*
   * The first step is to try to locate the URL that contains the principals.
   * This is done with a PROPFIND request and a XML like this:
   * <?xml version="1.0" encoding="utf-8" ?>
   * <D:propfind xmlns:D="DAV:">
   *   <D:prop>
   *     <D:principal-collection-set/>
   *   </D:prop>
   * </D:propfind>
   */
  QDomDocument query;

  QDomElement propfind = query.createElementNS( "DAV:", "propfind" );
  query.appendChild( propfind );

  QDomElement prop = query.createElementNS( "DAV:", "prop" );
  propfind.appendChild( prop );

  QDomElement principalCollectionSet = query.createElementNS( "DAV:", "principal-collection-set" );
  prop.appendChild( principalCollectionSet );

  KIO::DavJob *job = DavManager::self()->createPropFindJob( mUrl.url(), query );
  job->addMetaData( "PropagateHttpHeader", "true" );
  connect( job, SIGNAL(result(KJob*)), this, SLOT(principalCollectionSetSearchFinished(KJob*)) );
  job->start();
}

void DavPrincipalSearchJob::principalCollectionSetSearchFinished( KJob* job )
{

  if ( job->error() ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    emitResult();
    return;
  }

  KIO::DavJob *davJob = qobject_cast<KIO::DavJob*>( job );

  const int responseCode = davJob->queryMetaData( "responsecode" ).toInt();

  if ( responseCode > 499 && responseCode < 600 ) {
    // Server-side error, unrecoverable
    setError( UserDefinedError );
    setErrorText( i18n( "The server encountered an error that prevented it from completing your request" ) );
    emitResult();
    return;
  } else if ( responseCode > 399 && responseCode < 500 ) {
    // User-side error
    setError( UserDefinedError );
    setErrorText( i18n( "There was a problem with the request : error %1.", responseCode ) );
    emitResult();
    return;
  }

  /*
   * Extract information from a document like the following:
   *
   * <?xml version="1.0" encoding="utf-8" ?>
   * <D:multistatus xmlns:D="DAV:">
   *   <D:response>
   *     <D:href>http://www.example.com/papers/</D:href>
   *     <D:propstat>
   *       <D:prop>
   *         <D:principal-collection-set>
   *           <D:href>http://www.example.com/acl/users/</D:href>
   *           <D:href>http://www.example.com/acl/groups/</D:href>
   *         </D:principal-collection-set>
   *       </D:prop>
   *       <D:status>HTTP/1.1 200 OK</D:status>
   *     </D:propstat>
   *   </D:response>
   * </D:multistatus>
   */

  QDomDocument document = davJob->response();
  QDomElement documentElement = document.documentElement();

  QDomElement responseElement = DavUtils::firstChildElementNS( documentElement, "DAV:", "response" );
  if ( responseElement.isNull() ) {
    emitResult();
    return;
  }

  // check for the valid propstat, without giving up on first error
  QDomElement propstatElement;
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
    emitResult();
    return;
  }

  QDomElement propElement = DavUtils::firstChildElementNS( propstatElement, "DAV:", "prop" );
  if ( propElement.isNull() ) {
    emitResult();
    return;
  }

  QDomElement principalCollectionSetElement = DavUtils::firstChildElementNS( propElement, "DAV:", "principal-collection-set" );
  if ( principalCollectionSetElement.isNull() ) {
    emitResult();
    return;
  }

  QDomNodeList hrefNodes = principalCollectionSetElement.elementsByTagNameNS( "DAV:", "href" );
  for ( int i = 0; i < hrefNodes.size(); ++i ) {
    QDomElement hrefElement = hrefNodes.at( i ).toElement();
    QString href = hrefElement.text();

    KUrl url = mUrl.url();
    if ( href.startsWith( '/' ) ) {
      // href is only a path, use request url to complete
      url.setEncodedPath( href.toAscii() );
    } else {
      // href is a complete url
      KUrl tmpUrl( href );
      tmpUrl.setUser( url.user() );
      tmpUrl.setPass( url.pass() );
      url = tmpUrl;
    }

    QDomDocument principalPropertySearchQuery;
    buildReportQuery( principalPropertySearchQuery );
    KIO::DavJob *reportJob = DavManager::self()->createReportJob( url, principalPropertySearchQuery );
    reportJob->addMetaData( "PropagateHttpHeader", "true" );
    connect( reportJob, SIGNAL(result(KJob*)), this, SLOT(principalPropertySearchFinished(KJob*)) );
    ++mPrincipalPropertySearchSubJobCount;
    reportJob->start();
  }
}

void DavPrincipalSearchJob::principalPropertySearchFinished( KJob* job )
{
  --mPrincipalPropertySearchSubJobCount;

  if ( job->error() && !mPrincipalPropertySearchSubJobSuccessful ) {
    setError( job->error() );
    setErrorText( job->errorText() );
    if ( mPrincipalPropertySearchSubJobCount == 0 )
      emitResult();
    return;
  }

  KIO::DavJob *davJob = qobject_cast<KIO::DavJob*>( job );

  const int responseCode = davJob->queryMetaData( "responsecode" ).toInt();

  if ( responseCode > 499 && responseCode < 600 && !mPrincipalPropertySearchSubJobSuccessful ) {
    // Server-side error, unrecoverable
    setError( UserDefinedError );
    setErrorText( i18n( "The server encountered an error that prevented it from completing your request" ) );
    if ( mPrincipalPropertySearchSubJobCount == 0 )
      emitResult();
    return;
  } else if ( responseCode > 399 && responseCode < 500 && !mPrincipalPropertySearchSubJobSuccessful ) {
    // User-side error
    setError( UserDefinedError );
    setErrorText( i18n( "There was a problem with the request : error %1.", responseCode ) );
    if ( mPrincipalPropertySearchSubJobCount == 0 )
      emitResult();
    return;
  }

  if ( !mPrincipalPropertySearchSubJobSuccessful ) {
    setError( 0 ); // nope, everything went fine
    mPrincipalPropertySearchSubJobSuccessful = true;
  }

  /*
   * Extract infos from a document like the following:
   * <?xml version="1.0" encoding="utf-8" ?>
   * <D:multistatus xmlns:D="DAV:" xmlns:B="http://BigCorp.com/ns/">
   *   <D:response>
   *     <D:href>http://www.example.com/users/jdoe</D:href>
   *     <D:propstat>
   *       <D:prop>
   *         <D:displayname>John Doe</D:displayname>
   *       </D:prop>
   *       <D:status>HTTP/1.1 200 OK</D:status>
   *     </D:propstat>
   * </D:multistatus>
  */

  const QDomDocument document = davJob->response();
  const QDomElement documentElement = document.documentElement();

  QDomElement responseElement = DavUtils::firstChildElementNS( documentElement, "DAV:", "response" );
  if ( responseElement.isNull() ) {
    if ( mPrincipalPropertySearchSubJobCount == 0 )
      emitResult();
    return;
  }

  // check for the valid propstat, without giving up on first error
  QDomElement propstatElement;
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
    if ( mPrincipalPropertySearchSubJobCount == 0 )
      emitResult();
    return;
  }

  QDomElement propElement = DavUtils::firstChildElementNS( propstatElement, "DAV:", "prop" );
  if ( propElement.isNull() ) {
    if ( mPrincipalPropertySearchSubJobCount == 0 )
      emitResult();
    return;
  }

  // All requested properties are now under propElement, so let's find them
  QPair<QString, QString> fetchProperty;
  foreach ( fetchProperty, mFetchProperties ) {
    QDomNodeList fetchNodes = propElement.elementsByTagNameNS( fetchProperty.first, fetchProperty.second );
    for ( int i = 0; i < fetchNodes.size(); ++i ) {
      QDomElement fetchElement = fetchNodes.at( i ).toElement();
      Result result;
      result.propertyNamespace = fetchProperty.first;
      result.property = fetchProperty.second;
      result.value = fetchElement.text();
      mResults << result;
    }
  }

  if ( mPrincipalPropertySearchSubJobCount == 0 )
    emitResult();
}

QList< DavPrincipalSearchJob::Result > DavPrincipalSearchJob::results() const
{
  return mResults;
}

void DavPrincipalSearchJob::buildReportQuery( QDomDocument& query )
{
  /*
   * Build a document like the following, where XXX will
   * be replaced by the properties the user want to fetch:
   *
   *  <?xml version="1.0" encoding="utf-8" ?>
   *  <D:principal-property-search xmlns:D="DAV:">
   *    <D:property-search>
   *      <D:prop>
   *        <D:displayname/>
   *      </D:prop>
   *      <D:match>FILTER</D:match>
   *    </D:property-search>
   *    <D:prop>
   *      XXX
   *    </D:prop>
   *  </D:principal-property-search>
   */

  QDomElement principalPropertySearch = query.createElementNS( "DAV:", "principal-property-search" );
  query.appendChild( principalPropertySearch );

  QDomElement propertySearch = query.createElementNS( "DAV:", "property-search" );
  principalPropertySearch.appendChild( propertySearch );

  QDomElement prop = query.createElementNS( "DAV:", "prop" );
  propertySearch.appendChild( prop );

  if ( mType == DisplayName ) {
    QDomElement displayName = query.createElementNS( "DAV:", "displayname" );
    prop.appendChild( displayName );
  }
  else if ( mType == EmailAddress ) {
    QDomElement calendarUserAddressSet = query.createElementNS( "urn:ietf:params:xml:ns:caldav", "calendar-user-address-set" );
    prop.appendChild( calendarUserAddressSet );
    //QDomElement hrefElement = query.createElementNS( "DAV:", "href" );
    //prop.appendChild( hrefElement );
  }

  QDomElement match = query.createElementNS( "DAV:", "match" );
  propertySearch.appendChild( match );

  QDomText propFilter = query.createTextNode( mFilter );
  match.appendChild( propFilter );

  prop = query.createElementNS( "DAV:", "prop" );
  principalPropertySearch.appendChild( prop );

  QPair<QString, QString> fetchProperty;
  foreach ( fetchProperty, mFetchProperties ) {
    QDomElement elem = query.createElementNS( fetchProperty.first, fetchProperty.second );
    prop.appendChild( elem );
  }
}
