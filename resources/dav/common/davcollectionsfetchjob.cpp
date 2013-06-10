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
#include "davprincipalhomesetsfetchjob.h"
#include "davprotocolbase.h"
#include "davutils.h"

#include <kdebug.h>
#include <kio/davjob.h>
#include <klocale.h>

#include <QtCore/QBuffer>
#include <QtXmlPatterns/QXmlQuery>

DavCollectionsFetchJob::DavCollectionsFetchJob( const DavUtils::DavUrl &url, QObject *parent )
  : KJob( parent ), mUrl( url ), mSubJobCount( 0 )
{
}

void DavCollectionsFetchJob::start()
{
  if ( DavManager::self()->davProtocol( mUrl.protocol() )->supportsPrincipals() ) {
    DavPrincipalHomeSetsFetchJob *job = new DavPrincipalHomeSetsFetchJob( mUrl );
    connect( job, SIGNAL(result(KJob*)), SLOT(principalFetchFinished(KJob*)) );
    job->start();
  } else {
    doCollectionsFetch( mUrl.url() );
  }
}

DavCollection::List DavCollectionsFetchJob::collections() const
{
  return mCollections;
}

DavUtils::DavUrl DavCollectionsFetchJob::davUrl() const
{
  return mUrl;
}

void DavCollectionsFetchJob::doCollectionsFetch( const KUrl &url )
{
  ++mSubJobCount;

  const QDomDocument collectionQuery = DavManager::self()->davProtocol( mUrl.protocol() )->collectionsQuery();

  KIO::DavJob *job = DavManager::self()->createPropFindJob( url, collectionQuery );
  connect( job, SIGNAL(result(KJob*)), SLOT(collectionsFetchFinished(KJob*)) );
  job->addMetaData( "PropagateHttpHeader", "true" );
}

void DavCollectionsFetchJob::principalFetchFinished( KJob *job )
{
  if ( job->error() ) {
    // This may mean that the URL was not a principal URL.
    // Retry as if it were a calendar URL.
    kDebug() << job->errorText();
    doCollectionsFetch( mUrl.url() );
    return;
  }

  const DavPrincipalHomeSetsFetchJob *davJob = qobject_cast<DavPrincipalHomeSetsFetchJob*>( job );

  const QStringList homeSets = davJob->homeSets();
  kDebug() << "Found " << homeSets.size() << " homesets";
  kDebug() << homeSets;

  if ( homeSets.isEmpty() ) {
    // Same as above, retry as if it were a calendar URL.
    doCollectionsFetch( mUrl.url() );
    return;
  }

  foreach ( const QString &homeSet, homeSets ) {
    KUrl url = mUrl.url();

    if ( homeSet.startsWith( '/' ) ) {
      // homeSet is only a path, use request url to complete
      url.setEncodedPath( homeSet.toLatin1() );
    } else {
      // homeSet is a complete url
      KUrl tmpUrl( homeSet );
      tmpUrl.setUser( url.user() );
      tmpUrl.setPass( url.pass() );
      url = tmpUrl;
    }

    doCollectionsFetch( url );
  }
}

void DavCollectionsFetchJob::collectionsFetchFinished( KJob *job )
{
  KIO::DavJob *davJob = qobject_cast<KIO::DavJob*>( job );
  const int responseCode = davJob->queryMetaData( "responsecode" ).isEmpty() ?
                            0 :
                            davJob->queryMetaData( "responsecode" ).toInt();

  // KIO::DavJob does not set error() even if the HTTP status code is a 4xx or a 5xx
  if ( davJob->error() || ( responseCode >= 400 && responseCode < 600 ) ) {
    if ( davJob->url() != mUrl.url() ) {
      // Retry as if the initial URL was a calendar URL.
      // We can end up here when retrieving a homeset on
      // which a PROPFIND resulted in an error
      doCollectionsFetch( mUrl.url() );
      return;
    }

    QString err;
    if ( davJob->error() && davJob->error() != KIO::ERR_SLAVE_DEFINED )
      err = KIO::buildErrorString( davJob->error(), davJob->errorText() );
    else
      err = davJob->errorText();

    setError( UserDefinedError + responseCode );
    setErrorText( i18n( "There was a problem with the request.\n"
                        "%1 (%2).", err, responseCode ) );
  }
  else {
    // For use in the collectionDiscovered() signal
    KUrl _jobUrl = mUrl.url();
    _jobUrl.setUser( QString() );
    const QString jobUrl = _jobUrl.prettyUrl();

    //kDebug() << davJob->response().toString();

    QByteArray resp( davJob->response().toByteArray() );
    QBuffer buffer( &resp );
    buffer.open( QIODevice::ReadOnly );

    QXmlQuery xquery;
    if ( !xquery.setFocus( &buffer ) ) {
      setError( UserDefinedError );
      setErrorText( i18n( "Error setting focus for XQuery" ) );
    }

    xquery.setQuery( DavManager::self()->davProtocol( mUrl.protocol() )->collectionsXQuery() );
    if ( !xquery.isValid() ) {
      setError( UserDefinedError );
      setErrorText( i18n( "Invalid XQuery submitted by DAV implementation" ) );
    }

    QString responsesStr;
    xquery.evaluateTo( &responsesStr );
    responsesStr.prepend( "<responses>" );
    responsesStr.append( "</responses>" );

    QDomDocument document;
    if ( !document.setContent( responsesStr, true ) ) {
      setError( UserDefinedError );
      setErrorText( i18n( "Invalid responses from backend" ) );
    }

    if ( !error() ) {
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
       *         <current-user-privilege-set xmlns="DAV:">
       *           <privilege xmlns="DAV:">
       *             <read xmlns="DAV:"/>
       *           </privilege>
       *         </current-user-privilege-set>
       *       </prop>
       *       <status xmlns="DAV:">HTTP/1.1 200 OK</status>
       *     </propstat>
       *   </response>
       * </responses>
       */

      const QDomElement responsesElement = document.documentElement();

      QDomElement responseElement = DavUtils::firstChildElementNS( responsesElement, "DAV:", "response" );
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

        // extract url
        const QDomElement hrefElement = DavUtils::firstChildElementNS( responseElement, "DAV:", "href" );
        if ( hrefElement.isNull() ) {
          responseElement = DavUtils::nextSiblingElementNS( responseElement, "DAV:", "response" );
          continue;
        }

        QString href = hrefElement.text();
        if ( !href.endsWith( '/' ) )
          href.append( '/' );

        KUrl url = davJob->url();
        url.setUser( QString() );
        if ( href.startsWith( '/' ) ) {
          // href is only a path, use request url to complete
          url.setEncodedPath( href.toLatin1() );
        } else {
          // href is a complete url
          KUrl tmpUrl( href );
          url = tmpUrl;
        }

        // don't add this resource if it has already been detected
        bool alreadySeen = false;
        foreach ( const DavCollection &seen, mCollections ) {
          if ( seen.url() == url.prettyUrl() )
            alreadySeen = true;
        }
        if ( alreadySeen ) {
          responseElement = DavUtils::nextSiblingElementNS( responseElement, "DAV:", "response" );
          continue;
        }

        // extract display name
        const QDomElement propElement = DavUtils::firstChildElementNS( propstatElement, "DAV:", "prop" );
        const QDomElement displaynameElement = DavUtils::firstChildElementNS( propElement, "DAV:", "displayname" );
        const QString displayName = displaynameElement.text();

        // extract calendar color if provided
        const QDomElement colorElement = DavUtils::firstChildElementNS( propElement, "http://apple.com/ns/ical/", "calendar-color" );
        QColor color;
        if ( !colorElement.isNull() ) {
          QString colorValue = colorElement.text();
          if ( QColor::isValidColor( colorValue ) )
            color.setNamedColor( colorValue );
        }

        // extract allowed content types
        const DavCollection::ContentTypes contentTypes = DavManager::self()->davProtocol( mUrl.protocol() )->collectionContentTypes( propstatElement );

        DavCollection collection( mUrl.protocol(), url.prettyUrl(), displayName, contentTypes );
        if ( color.isValid() )
          collection.setColor( color );

        // extract privileges
        const QDomElement currentPrivsElement = DavUtils::firstChildElementNS( propElement, "DAV:", "current-user-privilege-set" );
        if ( currentPrivsElement.isNull() ) {
          // Assume that we have all privileges
          collection.setPrivileges( DavUtils::All );
        } else {
          DavUtils::Privileges privileges = DavUtils::extractPrivileges( currentPrivsElement );
          collection.setPrivileges( privileges );
        }

        kDebug() << url.prettyUrl() << "PRIVS: " << collection.privileges();
        mCollections << collection;
        emit collectionDiscovered( mUrl.protocol(), url.prettyUrl(), jobUrl );

        responseElement = DavUtils::nextSiblingElementNS( responseElement, "DAV:", "response" );
      }
    }
  }

  if ( --mSubJobCount == 0 )
    emitResult();
}

#include "davcollectionsfetchjob.moc"
