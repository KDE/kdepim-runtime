/*
    Copyright (c) 2010 Grégory Oestreicher <greg@kamago.net>
    Based on DavItemsListJob which is copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "davitemsfetchjob.h"
#include "davmanager.h"
#include "davmultigetprotocol.h"

#include <kio/davjob.h>
#include <klocale.h>

DavItemsFetchJob::DavItemsFetchJob( const DavUtils::DavUrl &collectionUrl, const QStringList &urls, QObject *parent )
  : KJob( parent ), mCollectionUrl( collectionUrl ), mUrls( urls )
{
}

void DavItemsFetchJob::start()
{
  const DavMultigetProtocol *protocol =
      dynamic_cast<const DavMultigetProtocol*>( DavManager::self()->davProtocol( mCollectionUrl.protocol() ) );
  if ( !protocol ) {
    setError( UserDefinedError );
    setErrorText( QString( "Protocol for the collection does not support MULTIGET" ) );
    emitResult();
    return;
  }

  const QDomDocument report = protocol->itemsReportQuery( mUrls );
  KIO::DavJob *job = DavManager::self()->createReportJob( mCollectionUrl.url(), report );
  job->addMetaData( "PropagateHttpHeader", "true" );
  connect( job, SIGNAL(result(KJob*)), this, SLOT(davJobFinished(KJob*)) );
}

DavItem::List DavItemsFetchJob::items() const
{
  return mItems.values();
}

DavItem DavItemsFetchJob::item( const QString &url ) const
{
  return mItems.value( url );
}

void DavItemsFetchJob::davJobFinished( KJob *job )
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
    QString extraMessage;
    if ( responseCode == 401 )
      extraMessage = i18n( "Invalid username/password" );
    else if ( responseCode == 403 )
      extraMessage = i18n( "Access forbidden" );
    else if ( responseCode == 404 )
      extraMessage = i18n( "Resource not found" );
    else
      extraMessage = i18n( "HTTP error" );

    setError( UserDefinedError );
    setErrorText( i18n( "There was a problem with the request.\n"
                        "%1 (%2).", extraMessage, responseCode ) );
    emitResult();
    return;
  }

  const DavMultigetProtocol *protocol =
      dynamic_cast<const DavMultigetProtocol*>( DavManager::self()->davProtocol( mCollectionUrl.protocol() ) );

  const QDomDocument document = davJob->response();
  const QDomElement documentElement = document.documentElement();

  QDomElement responseElement = DavUtils::firstChildElementNS( documentElement, "DAV:", "response" );

  while ( !responseElement.isNull() ) {
    QDomElement propstatElement = DavUtils::firstChildElementNS( responseElement, "DAV:", "propstat" );

    if ( propstatElement.isNull() ) {
      responseElement = DavUtils::nextSiblingElementNS( responseElement, "DAV:", "response" );
      continue;
    }

    // Check for errors
    const QDomElement statusElement = DavUtils::firstChildElementNS( propstatElement, "DAV:", "status" );
    if ( !statusElement.text().contains( "200" ) ) {
      responseElement = DavUtils::nextSiblingElementNS( responseElement, "DAV:", "response" );
      continue;
    }

    const QDomElement propElement = DavUtils::firstChildElementNS( propstatElement, "DAV:", "prop" );

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

    item.setUrl( url.prettyUrl() );

    // extract etag
    const QDomElement getetagElement = DavUtils::firstChildElementNS( propElement, "DAV:", "getetag" );
    item.setEtag( getetagElement.text() );

    // extract content
    const QDomElement dataElement = DavUtils::firstChildElementNS( propElement,
                                                                   protocol->responseNamespace(),
                                                                   protocol->dataTagName() );

    if ( dataElement.isNull() ) {
      responseElement = DavUtils::nextSiblingElementNS( responseElement, "DAV:", "response" );
      continue;
    }

    const QByteArray data = dataElement.firstChild().toText().data().toUtf8();
    if ( data.isEmpty() ) {
      responseElement = DavUtils::nextSiblingElementNS( responseElement, "DAV:", "response" );
      continue;
    }

    item.setData( data );

    mItems.insert( item.url(), item );
    responseElement = DavUtils::nextSiblingElementNS( responseElement, "DAV:", "response" );
  }

  emitResult();
}
