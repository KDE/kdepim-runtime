/*
    Copyright (c) 2009 Grégory Oestreicher <greg@kamago.net>

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

#include "davaccessor.h"

#include "davprotocolbase.h"

#include <QtCore/QBuffer>
#include <QtCore/QDataStream>
#include <QtCore/QFile>
#include <QtCore/QVariant>
#include <QtXmlPatterns/QXmlQuery>

#include <kdebug.h>
#include <kio/davjob.h>
#include <kio/deletejob.h>
#include <kio/job.h>
#include <kio/jobclasses.h>
#include <klocalizedstring.h>
#include <kstandarddirs.h>

DavAccessor::DavAccessor( DavProtocolBase *implementation )
  : mDavImpl( implementation )
{
}

DavAccessor::~DavAccessor()
{
  delete mDavImpl;
}

void DavAccessor::retrieveCollections( const KUrl &url )
{
  const QDomDocument collectionQuery = mDavImpl->collectionsQuery();

  KIO::DavJob *job = doPropfind( url, collectionQuery, "1" );
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( collectionsPropfindFinished( KJob* ) ) );
}

void DavAccessor::collectionsPropfindFinished( KJob *j )
{
  KIO::DavJob* job = dynamic_cast<KIO::DavJob*>( j );
  if ( !job )
    return;

  if ( job->error() ) {
    emit accessorError( job->errorString(), true );
    return;
  }

  QByteArray resp( job->response().toByteArray() );
  QBuffer buffer( &resp );
  buffer.open( QIODevice::ReadOnly );

  QXmlQuery xquery;
  if ( !xquery.setFocus( &buffer ) ) {
    emit accessorError( i18n( "Error setting focus for XQuery" ), true );
    return;
  }

  xquery.setQuery( mDavImpl->collectionsXQuery() );
  if ( !xquery.isValid() ) {
    emit accessorError( i18n( "Invalid XQuery submitted by DAV implementation" ), true );
    return;
  }

  QString responsesStr;
  xquery.evaluateTo( &responsesStr );
  responsesStr.prepend( "<responses>" ); responsesStr.append( "</responses>" );

  QDomDocument responses;
  if ( !responses.setContent( responsesStr, true ) ) {
    emit accessorError( i18n( "Invalid responses from backend" ), true );
    return;
  }

  QDomElement root = responses.documentElement();
  QDomNode node = root.firstChild();
  while ( !node.isNull() ) {
    if ( !node.isElement() ) {
      node = node.nextSibling();
      continue;
    }

    QDomElement r = node.toElement();
    node = node.nextSibling();

    QDomNodeList tmp;
    QDomElement propstat;
    QString href, status, displayname;
    KUrl url = job->url();

    tmp = r.elementsByTagNameNS( "DAV:", "href" );

    if ( tmp.length() == 0 )
      continue;

    href = tmp.item( 0 ).firstChild().toText().data();
    kDebug() << href;

    if ( !href.endsWith( '/' ) )
      href.append( '/' );

    if ( href.startsWith( '/' ) )
      url.setEncodedPath( href.toAscii() );
    else {
      KUrl tmpUrl( href );
      tmpUrl.setUser( url.user() );
      url = tmpUrl;
    }

    href = url.prettyUrl();

    tmp = r.elementsByTagNameNS( "DAV:", "propstat" );
    if ( tmp.length() == 0 )
      continue;

    int nPropstat = tmp.length();
    for ( int i = 0; i < nPropstat; ++i ) {
      const QDomElement node = tmp.item( i ).toElement();
      const QDomNode status = node.elementsByTagNameNS( "DAV:", "status" ).item( 0 );
      const QString statusText = status.firstChild().toText().data();
      if ( statusText.contains( "200" ) ) {
        propstat = node.toElement();
      }
    }

    if ( propstat.isNull() )
      continue;

    tmp = propstat.elementsByTagNameNS( "DAV:", "displayname" );
    if ( tmp.length() != 0 )
      displayname = tmp.item( 0 ).firstChild().toText().data();
    else
      displayname = "DAV calendar at " + href;

    kDebug() << "Seen collection at " << href << " (url)" << url.prettyUrl();

    mRunningItemsQueryCount[href] = 0;

    emit( collectionRetrieved( DavCollection( href, displayname ) ) );
  }

  emit collectionsRetrieved();
}

void DavAccessor::retrieveItems( const KUrl &url )
{
  QListIterator<QDomDocument> it( mDavImpl->itemsQueries() );

  while ( it.hasNext() ) {
    const QDomDocument props = it.next();

    if ( mDavImpl->useReport() ) {
      KIO::DavJob *job = doReport( url, props, "1" );
      connect( job, SIGNAL( result( KJob* ) ), this, SLOT( itemsReportFinished( KJob* ) ) );
    } else {
      KIO::DavJob *job = doPropfind( url, props, "1" );
      connect( job, SIGNAL( result( KJob* ) ), this, SLOT( itemsPropfindFinished( KJob* ) ) );
    }

    ++mRunningItemsQueryCount[ url.prettyUrl() ];
  }
}

void DavAccessor::itemsPropfindFinished( KJob *job )
{
  KIO::DavJob* davJob = qobject_cast<KIO::DavJob*>( job );
  if ( !davJob )
    return;

  if ( davJob->error() ) {
    emit accessorError( davJob->errorString(), true );
    return;
  }

  const QString collectionUrl = davJob->url().prettyUrl();
  if ( --mRunningItemsQueryCount[ collectionUrl ] == 0 )
    clearSeenUrls( collectionUrl );

  const QDomDocument xml = davJob->response();
  const QDomElement root = xml.documentElement();

  QDomNode node = root.firstChild();
  while( !node.isNull() ) {
    if ( !node.isElement() ) {
      node = node.nextSibling();
      continue;
    }

    QDomElement r = node.toElement();
    node = node.nextSibling();

    QDomNodeList tmp;
    QDomElement propstat;
    QString href, status, etag;
    KUrl url = davJob->url();

    tmp = r.elementsByTagNameNS( "DAV:", "href" );
    if ( tmp.length() == 0 )
      continue;

    href = tmp.item( 0 ).firstChild().toText().data();
    if ( href.startsWith( '/' ) )
      url.setEncodedPath( href.toAscii() );
    else
      url = href;

    href = url.prettyUrl();

    tmp = r.elementsByTagNameNS( "DAV:", "propstat" );
    if ( tmp.length() == 0 )
      continue;

    propstat = tmp.item( 0 ).toElement();

    tmp = propstat.elementsByTagNameNS( "DAV:", "resourcetype" );
    if ( tmp.length() != 0 ) {
      tmp = tmp.item( 0 ).toElement().elementsByTagNameNS( "DAV:", "collection" );
      if ( tmp.length() != 0 )
        continue;
    }

    // NOTE: nothing below should invalidate the item (return an error
    // and exit the function)
    seenUrl( collectionUrl, href );

    tmp = propstat.elementsByTagNameNS( "DAV:", "getetag" );
    if ( tmp.length() != 0 ) {
      etag = tmp.item( 0 ).firstChild().toText().data();

      const DavItemCacheStatus itemStatus = itemCacheStatus( href, etag );
      if ( itemStatus == CACHED ) {
        continue;
      }
    }

    mFetchItemsQueue[ collectionUrl ] << href;
  }

  if ( mRunningItemsQueryCount[ collectionUrl ] == 0 )
    runItemsFetch( collectionUrl );
}

void DavAccessor::itemsReportFinished( KJob *job )
{
  KIO::DavJob* davJob = qobject_cast<KIO::DavJob*>( job );
  if ( !davJob )
    return;

  if ( davJob->error() ) {
    emit accessorError( davJob->errorString(), true );
    return;
  }

  const QString collectionUrl = davJob->url().prettyUrl();
  if ( mRunningItemsQueryCount[ collectionUrl ] == mDavImpl->itemsQueries().size() )
    clearSeenUrls( collectionUrl );

  const QDomDocument xml = davJob->response();
  qDebug() << xml.toString();
  const QDomElement root = xml.documentElement();

  QDomNode node = root.firstChild();
  while( !node.isNull() ) {
    if ( !node.isElement() ) {
      node = node.nextSibling();
      continue;
    }

    QDomElement r = node.toElement();
    node = node.nextSibling();

    QDomNodeList tmp;
    QString href, etag;
    KUrl url = davJob->url();

    tmp = r.elementsByTagNameNS( "DAV:", "href" );
    if ( tmp.length() == 0 )
      continue;

    href = tmp.item( 0 ).firstChild().toText().data();

    if ( href.startsWith( '/' ) )
      url.setEncodedPath( href.toAscii() );
    else
      url = href;

    href = url.prettyUrl();

    // NOTE: nothing below should invalidate the item (return an error
    // and exit the function)
    //kDebug() << "Listed URL " << href << " in collection " << collectionUrl;
    seenUrl( collectionUrl, href );

    tmp = r.elementsByTagNameNS( "DAV:", "getetag" );
    if ( tmp.length() != 0 ) {
      etag = tmp.item( 0 ).firstChild().toText().data();

      const DavItemCacheStatus itemStatus = itemCacheStatus( href, etag );
      if ( itemStatus == CACHED ) {
        continue;
      }
    }

    mFetchItemsQueue[ collectionUrl ] << href;
  }

  if ( --mRunningItemsQueryCount[ collectionUrl ] == 0 )
    runItemsFetch( collectionUrl );
}

void DavAccessor::putItem( const KUrl &url, const QString &contentType, const QByteArray &data, bool useCachedEtag )
{
  const QString urlStr = url.prettyUrl();

  QString etag;
  QString headers = "Content-Type: ";
  headers += contentType;
  headers += "\r\n";

  if ( useCachedEtag && mItemsCache.contains( urlStr ) && !mItemsCache[ urlStr ].etag().isEmpty() ) {
    etag = mItemsCache[ urlStr ].etag();
    headers += "If-Match: "+etag;
  } else {
    headers += "If-None-Match: *";
  }

  DavItem item( urlStr, contentType, data, etag );
  mItemsCache[ urlStr ] = item;

  KIO::StoredTransferJob *job = KIO::storedPut( data, url, -1, KIO::HideProgressInfo | KIO::DefaultFlags );
  job->addMetaData( "PropagateHttpHeader", "true" );
  job->addMetaData( "customHTTPHeader", headers );

  connect( job, SIGNAL( result( KJob* ) ),
           this, SLOT( itemPutFinished( KJob* ) ) );
}

void DavAccessor::itemPutFinished( KJob *job )
{
  KIO::StoredTransferJob *storedJob = qobject_cast<KIO::StoredTransferJob*>( job );
  if ( !storedJob ) {
    emit accessorError( i18n( "Problem with the retrieval job" ), true );
    return;
  }

  if ( storedJob->error() ) {
    emit accessorError( storedJob->errorString(), true );
    return;
  }

  // The Location: HTTP header is used to indicate the new URL
  const QStringList allHeaders = storedJob->queryMetaData( "HTTP-Headers" ).split( "\n" );

  const KUrl oldUrl = storedJob->url();
  const QString oldUrlStr = oldUrl.prettyUrl();

  QString location;
  foreach ( const QString &header, allHeaders ) {
    if ( header.startsWith( "Location:" ) )
      location = header.section( ' ', 1 );
  }

  KUrl newUrl;
  if ( location.isEmpty() )
    newUrl = storedJob->url();
  else
    newUrl = location;

  const QString newUrlStr = newUrl.prettyUrl();

  const QString etag = getEtagFromHeaders( storedJob->queryMetaData( "HTTP-Headers" ) );

  kDebug() << "Last put item at (old)" << oldUrlStr << " (new)" << newUrlStr << " (etag)" << etag;
  mItemsCache[ oldUrlStr ].setEtag( etag );

  if ( oldUrl != newUrl ) {
    // mItemsCache[oldUrl.url()] has been modified by putItem() before the job starts
    mItemsCache[ newUrlStr ] = mItemsCache[ oldUrlStr ];
    mItemsCache[ newUrlStr ].setUrl( newUrlStr );
    mItemsCache.remove( oldUrlStr );
  }

  emit itemPut( oldUrl, mItemsCache[ newUrlStr ] );
}

void DavAccessor::removeItem( const KUrl &url )
{
  QString etag;
  if ( mItemsCache.contains( url.prettyUrl() ) )
    etag = mItemsCache[ url.prettyUrl() ].etag();

  kDebug() << "Requesting removal of item at " << url.prettyUrl() << " with etag " << etag;

  if ( etag.isEmpty() ) {
    emit( accessorError( i18n( "No item on the remote server for URL %1", url.prettyUrl() ), true ) );
    return;
  }

  KIO::DeleteJob *job = KIO::del( url, KIO::HideProgressInfo | KIO::DefaultFlags );
  job->addMetaData( "PropagateHttpHeader", "true" );
  job->addMetaData( "customHTTPHeader", "If-Match: " + etag );
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( itemDelFinished( KJob* ) ) );
  connect( job, SIGNAL( warning( KJob*, const QString&, const QString& ) ),
           this, SLOT( jobWarning( KJob*, const QString&, const QString& ) ) );
}

void DavAccessor::itemDelFinished( KJob *job )
{
  KIO::DeleteJob *deleteJob = qobject_cast<KIO::DeleteJob*>( job );
  if ( !deleteJob ) {
    emit accessorError( i18n( "Problem with the retrieval job" ), true );
    return;
  }

  kDebug() << deleteJob->queryMetaData( "HTTP-Headers" );

  if ( deleteJob->error() && deleteJob->queryMetaData( "response-code" ) != "404" ) {
    emit accessorError( deleteJob->errorString(), true );
    return;
  }

  const KUrl url = deleteJob->urls().first();

  mItemsCache.remove( url.prettyUrl() );

  emit itemRemoved( url );
}

void DavAccessor::jobWarning( KJob*, const QString &p, const QString& )
{
  kDebug() << "Warning : " << p;
  emit accessorError( p, true );
}

void DavAccessor::validateCache()
{
  kDebug() << "Beginning cache validation";
  kDebug() << mItemsCache.size() << " items in cache";

  QSet<QString> latest;
  QMapIterator<QString, QSet<QString> > it( mLastSeenItems );
  while ( it.hasNext() ) {
    it.next();
    latest += it.value();
  }

  kDebug() << "Seen " << latest.size() << " items";

  QSet<QString> cache = QSet<QString>::fromList( mItemsCache.keys() );
  // Theorically cache.size() >= latest.size()
  cache.subtract( latest );
  // So now cache should only contains deleted item

  DavItem::List removedItems;

  foreach ( const QString &url, cache ) {
    removedItems << mItemsCache[ url ];
    mItemsCache.remove( url );
    kDebug() << url << " disappeared from backend";
  }

  emit backendItemsRemoved( removedItems );

  kDebug() << "Finished cache validation";
}

KIO::DavJob* DavAccessor::doPropfind( const KUrl &url, const QDomDocument &props, const QString &davDepth )
{
  KIO::DavJob *job = KIO::davPropFind( url, props, davDepth, KIO::HideProgressInfo | KIO::DefaultFlags );

  // workaround needed, Depth: header doesn't seem to be correctly added
  const QString header = "Content-Type: text/xml\r\nDepth: " + davDepth;
  job->addMetaData( "customHTTPHeader", header );
  job->setProperty( "extraDavDepth", QVariant::fromValue( davDepth ) );

  return job;
}

KIO::DavJob* DavAccessor::doReport( const KUrl &url, const QDomDocument &rep, const QString &davDepth )
{
  KIO::DavJob *job = KIO::davReport( url, rep.toString(), davDepth, KIO::HideProgressInfo | KIO::DefaultFlags );

  // workaround needed, Depth: header doesn't seem to be correctly added
  QString header = "Content-Type: text/xml\r\nDepth: " + davDepth;
  job->addMetaData( "customHTTPHeader", header );
  job->setProperty( "extraDavDepth", QVariant::fromValue( davDepth ) );

  return job;
}

DavItemCacheStatus DavAccessor::itemCacheStatus( const QString &url, const QString &etag )
{
  DavItemCacheStatus status = NOT_CACHED;

  if ( mItemsCache.contains( url ) ) {
    if ( mItemsCache[ url ].etag() != etag ) {
      mItemsCache.remove( url );
      status = EXPIRED;
      kDebug() << "Item at " << url << " changed in the backend";
    }
    else {
      status = CACHED;
    }
  }

  return status;
}

DavItem DavAccessor::getItemFromCache( const QString &url )
{
  kDebug() << "Serving " << url << " from cache";
  return mItemsCache.value( url );
}

void DavAccessor::addItemToCache( const DavItem &item )
{
  mItemsCache[ item.url() ] = item;
}

void DavAccessor::clearSeenUrls( const QString &url )
{
  kDebug() << "Clearing seen items for collection " << url;
  mLastSeenItems[ url ].clear();
}

void DavAccessor::seenUrl( const QString &collectionUrl, const QString &itemUrl )
{
  mLastSeenItems[ collectionUrl ] << itemUrl;
}

QString DavAccessor::getEtagFromHeaders( const QString &headers )
{
  QString etag;
  const QStringList allHeaders = headers.split( "\n" );

  foreach ( const QString &header, allHeaders ) {
    if ( header.startsWith( "etag:", Qt::CaseInsensitive ) )
      etag = header.section( ' ', 1 );
  }

  return etag;
}

void DavAccessor::runItemsFetch( const QString &collection )
{
  if ( mDavImpl->useMultiget() ) {
    QMutexLocker locker( &mFetchItemsQueueMutex );

    const QStringList urls = mFetchItemsQueue[ collection ];
    mFetchItemsQueue[ collection ].clear();

    locker.unlock();

    if ( !urls.isEmpty() ) {
      const QDomDocument multiget = mDavImpl->itemsReportQuery( urls );
      KIO::DavJob *job = doReport( collection, multiget, "1" );
      connect( job, SIGNAL( result( KJob* ) ), this, SLOT( itemsMultigetFinished( KJob* ) ) );
    } else {
      emit itemsRetrieved();
    }
  } else {
    QMutexLocker locker( &mFetchItemsQueueMutex );

    if ( mFetchItemsQueue[ collection ].isEmpty() ) {
      emit itemsRetrieved();
      return;
    }

    const QString next = mFetchItemsQueue[ collection ].takeFirst();

    locker.unlock();

    KIO::StoredTransferJob *job = KIO::storedGet( next, KIO::Reload, KIO::HideProgressInfo | KIO::DefaultFlags );
    job->setProperty( "collectionUrl", QVariant::fromValue( collection ) );
    job->addMetaData( "PropagateHttpHeader", "true" );
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( itemGetFinished( KJob* ) ) );
  }
}

void DavAccessor::itemsMultigetFinished( KJob *job )
{
  KIO::DavJob* davJob = qobject_cast<KIO::DavJob*>( job );
  if ( !davJob )
    return;

  if ( davJob->error() ) {
    emit accessorError( davJob->errorString(), true );
    return;
  }

  const QDomDocument xml = davJob->response();
  const QDomElement root = xml.documentElement();

  QDomNode node = root.firstChild();
  while( !node.isNull() ) {
    if ( !node.isElement() ) {
      node = node.nextSibling();
      continue;
    }

    QDomElement r = node.toElement();
    node = node.nextSibling();

    QDomNodeList tmp;
    QString href, etag;
    QByteArray data;
    KUrl url = davJob->url();

    tmp = r.elementsByTagNameNS( "DAV:", "href" );
    if ( tmp.length() == 0 )
      continue;

    href = tmp.item( 0 ).firstChild().toText().data();
    if ( href.startsWith( '/' ) )
      url.setEncodedPath( href.toAscii() );
    else
      url = href;

    href = url.prettyUrl();

    tmp = r.elementsByTagNameNS( "DAV:", "getetag" );
    if ( tmp.length() == 0 )
      continue;

    etag = tmp.item( 0 ).firstChild().toText().data();

    tmp = r.elementsByTagNameNS( "urn:ietf:params:xml:ns:caldav", "calendar-data" );
    if ( tmp.length() == 0 )
      continue;

    data = tmp.item( 0 ).firstChild().toText().data().toUtf8();
    if ( data.isEmpty() )
      continue;

    kDebug() << "Got item with url " << href;
    DavItem item( href, "text/calendar", data, etag );

    const DavItemCacheStatus itemStatus = itemCacheStatus( href, etag );
    if ( itemStatus != CACHED )
      emit itemRetrieved( item );
  }

  emit itemsRetrieved();
}

void DavAccessor::itemGetFinished( KJob *job )
{
  KIO::StoredTransferJob *storedJob = qobject_cast<KIO::StoredTransferJob*>( job );
  if ( !storedJob ) {
    emit accessorError( i18n( "Problem with the retrieval job" ), true );
    return;
  }

  const QString collectionUrl = storedJob->property( "collectionUrl" ).value<QString>();
  const QByteArray d = storedJob->data();

  const QString url = storedJob->url().prettyUrl();
  const QString mimeType = storedJob->queryMetaData( "content-type" );
  const QString etag = getEtagFromHeaders( storedJob->queryMetaData( "HTTP-Headers" ) );

  if ( etag.isEmpty() ) {
    emit accessorError( i18n( "The server returned an invalid answer (no etag header)" ), true );
    return;
  }

  if ( itemCacheStatus( url, etag ) != CACHED ) {
    kDebug() << "Got Item at URL " << url;
    DavItem item( url, mimeType, d, etag );
    emit itemRetrieved( item );
  }

  runItemsFetch( collectionUrl );
}
