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

#include <QVariant>
#include <QFile>
#include <QDataStream>
#include <QtXmlPatterns>
#include <QBuffer>
#include <QVariant>

#include <kio/davjob.h>
#include <kio/jobclasses.h>
#include <kio/job.h>
#include <kio/deletejob.h>
#include <klocalizedstring.h>
#include <kdebug.h>
#include <kstandarddirs.h>

#include "davaccessor.h"
#include "davimplementation.h"

davItem::davItem()
{
}

davItem::davItem( const QString &u, const QString &c, const QByteArray &d, const QString &e )
  : url( u ), contentType( c ), data( d ), etag( e )
{
}

QDataStream& operator<<( QDataStream &out, const davItem &item )
{
  out << item.url;
  out << item.contentType;
  out << item.data;
  out << item.etag;
  return out;
}

QDataStream& operator>>( QDataStream &in, davItem &item)
{
  in >> item.url;
  in >> item.contentType;
  in >> item.data;
  in >> item.etag;
  return in;
}

davAccessor::davAccessor( davImplementation *i )
  : davImpl( i )
{
}

davAccessor::~davAccessor()
{
  delete davImpl;
}

void davAccessor::retrieveCollections( const KUrl &url )
{
  QDomDocument collectionQuery = davImpl->collectionsQuery();
  
  KIO::DavJob *job = doPropfind( url, collectionQuery, "1" );
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( collectionsPropfindFinished( KJob* ) ) );
}

void davAccessor::collectionsPropfindFinished( KJob *j )
{
  KIO::DavJob* job = dynamic_cast<KIO::DavJob*>( j );
  if( !job )
    return;
  
  if( job->error() ) {
    emit accessorError( job->errorString(), true );
    return;
  }
  
  QByteArray resp( job->response().toByteArray() );
  QBuffer buffer( &resp );
  buffer.open( QIODevice::ReadOnly );
  
  QXmlQuery xquery;
  if( !xquery.setFocus( &buffer ) ) {
    emit accessorError( i18n( "Error setting focus for XQuery" ), true );
    return;
  }
  
  xquery.setQuery( davImpl->collectionsXQuery() );
  if( !xquery.isValid() ) {
    emit accessorError( i18n( "Invalid XQuery submitted by DAV implementation" ), true );
    return;
  }
  
  QString responsesStr;
  xquery.evaluateTo( &responsesStr );
  responsesStr.prepend( "<responses>" ); responsesStr.append( "</responses>" );
  
  QDomDocument responses;
  if( !responses.setContent( responsesStr, true ) ) {
    emit accessorError( i18n( "Invalid responses from backend" ), true );
    return;
  }
  
  QDomElement root = responses.documentElement();
  QDomNode n = root.firstChild();
  while( !n.isNull() ) {
    if( !n.isElement() ) {
      n = n.nextSibling();
      continue;
    }
    
    QDomElement r = n.toElement();
    n = n.nextSibling();
    QDomNodeList tmp;
    QDomElement propstat;
    QString href, status, displayname;
    KUrl url = job->url();
    
    tmp = r.elementsByTagNameNS( "DAV:", "href" );
//     tmp = r.elementsByTagName( "href" );
    if( tmp.length() == 0 )
      continue;
    href = tmp.item( 0 ).firstChild().toText().data();
    kDebug() << href;
    
    if( !href.endsWith( "/" ) )
      href.append( "/" );
    
    if( href.startsWith( "/" ) ) {
      url.setEncodedPath( href.toAscii() );
    }
    else {
      KUrl tmpUrl( href );
      tmpUrl.setUser( url.user() );
      url = tmpUrl;
    }
    href = url.prettyUrl();
    
    tmp = r.elementsByTagNameNS( "DAV:", "propstat" );
    if( tmp.length() == 0 )
      continue;
    int nPropstat = tmp.length();
    for( int i = 0; i < nPropstat; ++i ) {
      QDomElement node = tmp.item( i ).toElement();
      QDomNode status = node.elementsByTagNameNS( "DAV:", "status" ).item( 0 );
      QString statusText = status.firstChild().toText().data();
      if( statusText.contains( "200" ) ) {
        propstat = node.toElement();
      }
    }
    if( propstat.isNull() )
      continue;
    
    tmp = propstat.elementsByTagNameNS( "DAV:", "displayname" );
    if( tmp.length() != 0 )
      displayname = tmp.item( 0 ).firstChild().toText().data();
    else
      displayname = "DAV calendar at " + href;
    
    kDebug() << "Seen collection at " << href << " (url)" << url.prettyUrl();
    nRunningItemsQueries[href] = 0;
    emit( collectionRetrieved( href, displayname ) );
  }
  
  emit collectionsRetrieved();
}

void davAccessor::retrieveItems( const KUrl &url )
{
  QListIterator<QDomDocument> it( davImpl->itemsQueries() );
  
  while( it.hasNext() ) {
    QDomDocument props = it.next();
    
    if( davImpl->useReport() ) {
      KIO::DavJob *job = doReport( url, props, "1" );
      connect( job, SIGNAL( result( KJob* ) ), this, SLOT( itemsReportFinished( KJob* ) ) );
    }
    else {
      KIO::DavJob *job = doPropfind( url, props, "1" );
      connect( job, SIGNAL( result( KJob* ) ), this, SLOT( itemsPropfindFinished( KJob* ) ) );
    }
    
    ++nRunningItemsQueries[url.prettyUrl()];
  }
}

void davAccessor::itemsPropfindFinished( KJob *j )
{
  KIO::DavJob* job = dynamic_cast<KIO::DavJob*>( j );
  if( !job )
    return;
  
  if( job->error() ) {
    emit accessorError( job->errorString(), true );
    return;
  }
  
  QString collectionUrl = job->url().prettyUrl();
  if( --nRunningItemsQueries[collectionUrl] == 0 )
    clearSeenUrls( collectionUrl );
  
  QDomDocument xml = job->response();
  QDomElement root = xml.documentElement();
  QDomNode n = root.firstChild();
  while( !n.isNull() ) {
    if( !n.isElement() ) {
      n = n.nextSibling();
      continue;
    }
    
    QDomElement r = n.toElement();
    n = n.nextSibling();
    
    QDomNodeList tmp;
    QDomElement propstat;
    QString href, status, etag;
    KUrl url = job->url();
    
    tmp = r.elementsByTagNameNS( "DAV:", "href" );
    if( tmp.length() == 0 )
      continue;
    href = tmp.item( 0 ).firstChild().toText().data();
    if( href.startsWith( "/" ) )
      url.setEncodedPath( href.toAscii() );
    else
      url = href;
    href = url.prettyUrl();
    
    tmp = r.elementsByTagNameNS( "DAV:", "propstat" );
    if( tmp.length() == 0 )
      continue;
    propstat = tmp.item( 0 ).toElement();
    
    tmp = propstat.elementsByTagNameNS( "DAV:", "resourcetype" );
    if( tmp.length() != 0 ) {
      tmp = tmp.item( 0 ).toElement().elementsByTagNameNS( "DAV:", "collection" );
      if( tmp.length() != 0 )
        continue;
    }
    
    // NOTE: nothing below should invalidate the item (return an error
    // and exit the function)
    seenUrl( collectionUrl, href );
    
    tmp = propstat.elementsByTagNameNS( "DAV:", "getetag" );
    if( tmp.length() != 0 ) {
      etag = tmp.item( 0 ).firstChild().toText().data();
      
      davItemCacheStatus itemStatus = itemCacheStatus( href, etag );
      if( itemStatus == CACHED ) {
        continue;
      }
    }
    
    fetchItemsQueue[collectionUrl] << href;
  }
  
  if( nRunningItemsQueries[collectionUrl] == 0 )
    runItemsFetch( collectionUrl );
}

void davAccessor::itemsReportFinished( KJob *j )
{
  KIO::DavJob* job = dynamic_cast<KIO::DavJob*>( j );
  if( !job )
    return;
  
  if( job->error() ) {
    emit accessorError( job->errorString(), true );
    return;
  }
  
  QString collectionUrl = job->url().prettyUrl();
  if( nRunningItemsQueries[collectionUrl] == davImpl->itemsQueries().size() )
    clearSeenUrls( collectionUrl );
  
  QDomDocument xml = job->response();
  QDomElement root = xml.documentElement();
  QDomNode n = root.firstChild();
  while( !n.isNull() ) {
    if( !n.isElement() ) {
      n = n.nextSibling();
      continue;
    }
    
    QDomElement r = n.toElement();
    n = n.nextSibling();
    
    QDomNodeList tmp;
    QString href, etag;
    KUrl url = job->url();
    
    tmp = r.elementsByTagNameNS( "DAV:", "href" );
    if( tmp.length() == 0 )
      continue;
    href = tmp.item( 0 ).firstChild().toText().data();
    
    if( href.startsWith( "/" ) )
      url.setEncodedPath( href.toAscii() );
    else
      url = href;
    href = url.prettyUrl();
    
    // NOTE: nothing below should invalidate the item (return an error
    // and exit the function)
    kDebug() << "Listed URL " << href << " in collection " << collectionUrl;
    seenUrl( collectionUrl, href );
    
    tmp = r.elementsByTagNameNS( "DAV:", "getetag" );
    if( tmp.length() != 0 ) {
      etag = tmp.item( 0 ).firstChild().toText().data();
      
      davItemCacheStatus itemStatus = itemCacheStatus( href, etag );
      if( itemStatus == CACHED ) {
        continue;
      }
    }
    
    fetchItemsQueue[collectionUrl] << href;
  }
  
  if( --nRunningItemsQueries[collectionUrl] == 0 )
    runItemsFetch( collectionUrl );
}

void davAccessor::putItem( const KUrl &url, const QString &contentType, const QByteArray &data, bool useCachedEtag )
{
  QString urlStr = url.prettyUrl();
  
  QString etag;
  QString headers = "Content-Type: ";
  headers += contentType;
  headers += "\r\n";
  
  if( useCachedEtag && itemsCache.contains( urlStr ) && !itemsCache[urlStr].etag.isEmpty() ) {
    etag = itemsCache[urlStr].etag;
    headers += "If-Match: "+etag;
  }
  else {
    headers += "If-None-Match: *";
  }
  
  davItem i( urlStr, contentType, data, etag );
  itemsCache[urlStr] = i;
  
  KIO::StoredTransferJob *job = KIO::storedPut( data, url, -1, KIO::HideProgressInfo | KIO::DefaultFlags );
  job->addMetaData( "PropagateHttpHeader", "true" );
  job->addMetaData( "customHTTPHeader", headers );
  
  connect( job, SIGNAL( result( KJob* ) ),
           this, SLOT( itemPutFinished( KJob* ) ) );
}

void davAccessor::itemPutFinished( KJob *j )
{
  KIO::StoredTransferJob *job = dynamic_cast<KIO::StoredTransferJob*>( j );
  if( !job ) {
    emit accessorError( i18n( "Problem with the retrieval job" ), true );
    return;
  }
  
  if( job->error() ) {
    emit accessorError( job->errorString(), true );
    return;
  }
  
  // The Location: HTTP header is used to indicate the new URL
  QStringList allHeaders = job->queryMetaData( "HTTP-Headers" ).split( "\n" );
  QString location;
  KUrl oldUrl = job->url();
  QString oldUrlStr = oldUrl.prettyUrl();
  KUrl newUrl;
  QString newUrlStr;
  
  foreach( QString hdr, allHeaders ) {
    if( hdr.startsWith( "Location:" ) ) {
      location = hdr.section( ' ', 1 );
    }
  }
  
  if( location.isEmpty() ) {
    newUrl = job->url();
  }
  else {
    newUrl = location;
  }
  newUrlStr = newUrl.prettyUrl();
  
  QString etag = getEtagFromHeaders( job->queryMetaData( "HTTP-Headers" ) );
  
  kDebug() << "Last put item at (old)" << oldUrlStr << " (new)" << newUrlStr << " (etag)" << etag;
  itemsCache[oldUrlStr].etag = etag;
  
  if( oldUrl != newUrl ) {
    // itemsCache[oldUrl.url()] has been modified by putItem() before the job starts
    itemsCache[newUrlStr] = itemsCache[oldUrlStr];
    itemsCache[newUrlStr].url = newUrlStr;
    itemsCache.remove( oldUrlStr );
  }
  
  emit itemPut( oldUrl, itemsCache[newUrlStr] );
}

void davAccessor::removeItem( const KUrl &url )
{
  QString etag;
  if( itemsCache.contains( url.prettyUrl() ) )
    etag = itemsCache[url.prettyUrl()].etag;
  
  kDebug() << "Requesting removal of item at " << url.prettyUrl() << " with etag " << etag;
  
  if( etag.isEmpty() ) {
    emit( accessorError( i18n( "No item on the remote server for URL %1", url.prettyUrl() ), true ) );
    return;
  }
  
  KIO::DeleteJob *job = KIO::del( url, KIO::HideProgressInfo | KIO::DefaultFlags );
  job->addMetaData( "PropagateHttpHeader", "true" );
  job->addMetaData( "customHTTPHeader", "If-Match: "+etag );
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( itemDelFinished( KJob* ) ) );
  connect( job, SIGNAL( warning( KJob*, const QString&, const QString& ) ), this, SLOT( jobWarning( KJob*, const QString&, const QString& ) ) );
}

void davAccessor::itemDelFinished( KJob *j )
{
  KIO::DeleteJob *job = dynamic_cast<KIO::DeleteJob*>( j );
  if( !job ) {
    emit accessorError( i18n( "Problem with the retrieval job" ), true );
    return;
  }
  
  kDebug() << job->queryMetaData( "HTTP-Headers" );
  
  if( job->error() && job->queryMetaData( "response-code" ) != "404" ) {
    emit accessorError( job->errorString(), true );
    return;
  }
  
  KUrl url = job->urls().first();
  itemsCache.remove( url.prettyUrl() );
  emit itemRemoved( url );
}

void davAccessor::jobWarning( KJob* j, const QString &p, const QString &r )
{
  Q_UNUSED( j );
  Q_UNUSED( r );
  kDebug() << "Warning : " << p;
  emit accessorError( p, true );
}

void davAccessor::validateCache()
{
  kDebug() << "Beginning cache validation";
  kDebug() << itemsCache.size() << " items in cache";
  
  QSet<QString> latest;
  QMapIterator<QString, QSet<QString> > it( lastSeenItems );
  while( it.hasNext() ) {
    it.next();
    latest += it.value();
  }
  
  kDebug() << "Seen " << latest.size() << " items";
  
  QSet<QString> cache = QSet<QString>::fromList( itemsCache.keys() );
  // Theorically cache.size() >= latest.size()
  cache.subtract( latest );
  // So now cache should only contains deleted item
  
  QList<davItem> removed;
  
  foreach( QString url, cache ) {
    removed << itemsCache[url];
    itemsCache.remove( url );
    kDebug() << url << " disappeared from backend";
  }
  
  emit backendItemsRemoved( removed );
  
  kDebug() << "Finished cache validation";
}

KIO::DavJob* davAccessor::doPropfind( const KUrl &url, const QDomDocument &props, const QString &davDepth )
{
  KIO::DavJob *job = KIO::davPropFind( url, props, davDepth, KIO::HideProgressInfo | KIO::DefaultFlags );
  
  // workaround needed, Depth: header doesn't seem to be correctly added
  QString header = 
      "Content-Type: text/xml\r\n"
      "Depth: "+davDepth;
  job->addMetaData( "customHTTPHeader", header );
  job->setProperty( "extraDavDepth", QVariant::fromValue( davDepth ) );
  
  return job;
}

KIO::DavJob* davAccessor::doReport( const KUrl &url, const QDomDocument &rep, const QString &davDepth )
{
  KIO::DavJob *job = KIO::davReport( url, rep.toString(), davDepth, KIO::HideProgressInfo | KIO::DefaultFlags );
  
  // workaround needed, Depth: header doesn't seem to be correctly added
  QString header = 
      "Content-Type: text/xml\r\n"
      "Depth: "+davDepth;
  job->addMetaData( "customHTTPHeader", header );
  job->setProperty( "extraDavDepth", QVariant::fromValue( davDepth ) );
  
  return job;
}

davItemCacheStatus davAccessor::itemCacheStatus( const QString &url, const QString &etag )
{
  davItemCacheStatus ret = NOT_CACHED;
  
  if( itemsCache.contains( url ) ) {
    if( itemsCache[url].etag != etag ) {
      itemsCache.remove( url );
      ret = EXPIRED;
      kDebug() << "Item at " << url << " changed in the backend";
    }
    else {
      ret = CACHED;
    }
  }
  
  return ret;
}

davItem davAccessor::getItemFromCache( const QString &url )
{
  kDebug() << "Serving " << url << " from cache";
  return itemsCache.value( url );
}

void davAccessor::addItemToCache( const davItem &item )
{
  itemsCache[item.url] = item;
}

void davAccessor::clearSeenUrls( const QString &url )
{
  kDebug() << "Clearing seen items for collection " << url;
  lastSeenItems[url].clear();
}

void davAccessor::seenUrl( const QString &collectionUrl, const QString &itemUrl )
{
  lastSeenItems[collectionUrl] << itemUrl;
}

QString davAccessor::getEtagFromHeaders( const QString &headers )
{
  QString etag;
  QStringList allHeaders = headers.split( "\n" );
  
  foreach( QString hdr, allHeaders ) {
    if( hdr.startsWith( "etag:", Qt::CaseInsensitive ) ) {
      etag = hdr.section( ' ', 1 );
    }
  }
  
  return etag;
}

void davAccessor::runItemsFetch( const QString &collection )
{
  if( davImpl->useMultiget() ) {
    QMutexLocker locker( &fetchItemsQueueMtx );
    QStringList urls = fetchItemsQueue[collection];
    fetchItemsQueue[collection].clear();
    locker.unlock();
  
    if( !urls.isEmpty() ) {
      QDomDocument multiget = davImpl->itemsReportQuery( urls );
      KIO::DavJob *job = doReport( collection, multiget, "1" );
      connect( job, SIGNAL( result( KJob* ) ), this, SLOT( itemsMultigetFinished( KJob* ) ) );
    }
    else {
      emit itemsRetrieved();
    }
  }
  else {
    QMutexLocker locker( &fetchItemsQueueMtx );
  
    if( fetchItemsQueue[collection].isEmpty() ) {
      emit itemsRetrieved();
      return;
    }
  
    QString next = fetchItemsQueue[collection].takeFirst();
    locker.unlock();
    KIO::StoredTransferJob *job = KIO::storedGet( next, KIO::Reload, KIO::HideProgressInfo | KIO::DefaultFlags );
    job->setProperty( "collectionUrl", QVariant::fromValue( collection ) );
    job->addMetaData( "PropagateHttpHeader", "true" );
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( itemGetFinished( KJob* ) ) );
  }
}

void davAccessor::itemsMultigetFinished( KJob *j )
{
  KIO::DavJob* job = dynamic_cast<KIO::DavJob*>( j );
  if( !job )
    return;
  
  if( job->error() ) {
    emit accessorError( job->errorString(), true );
    return;
  }
  
  QDomDocument xml = job->response();
  QDomElement root = xml.documentElement();
  QDomNode n = root.firstChild();
  while( !n.isNull() ) {
    if( !n.isElement() ) {
      n = n.nextSibling();
      continue;
    }
    
    QDomElement r = n.toElement();
    n = n.nextSibling();
    
    QDomNodeList tmp;
    QString href, etag;
    QByteArray data;
    KUrl url = job->url();
    
    tmp = r.elementsByTagNameNS( "DAV:", "href" );
    if( tmp.length() == 0 )
      continue;
    href = tmp.item( 0 ).firstChild().toText().data();
    if( href.startsWith( "/" ) )
      url.setEncodedPath( href.toAscii() );
    else
      url = href;
    href = url.prettyUrl();
    
    tmp = r.elementsByTagNameNS( "DAV:", "getetag" );
    if( tmp.length() == 0 )
      continue;
    etag = tmp.item( 0 ).firstChild().toText().data();
    
    tmp = r.elementsByTagNameNS( "urn:ietf:params:xml:ns:caldav", "calendar-data" );
    if( tmp.length() == 0 )
      continue;
    data = tmp.item( 0 ).firstChild().toText().data().toUtf8();
    if( data.isEmpty() )
      continue;
    
    kDebug() << "Got item with url " << href;
    davItem i( href, "text/calendar", data, etag );
    
    davItemCacheStatus itemStatus = itemCacheStatus( href, etag );
    if( itemStatus != CACHED ) {
      emit itemRetrieved( i );
    }
  }
  
  emit itemsRetrieved();
}

void davAccessor::itemGetFinished( KJob *j )
{
  KIO::StoredTransferJob *job = dynamic_cast<KIO::StoredTransferJob*>( j );
  if( !job ) {
    emit accessorError( i18n( "Problem with the retrieval job" ), true );
    return;
  }
  
  QString collectionUrl = job->property( "collectionUrl" ).value<QString>();
  QByteArray d = job->data();
  QString url = job->url().prettyUrl();
  QString mimeType = job->queryMetaData( "content-type" );
  QString etag = getEtagFromHeaders( job->queryMetaData( "HTTP-Headers" ) );
  
  if( etag.isEmpty() ) {
    emit accessorError( i18n( "The server returned an invalid answer (no etag header)" ), true );
    return;
  }
  
  if( itemCacheStatus( url, etag ) != CACHED ) {
    kDebug() << "Got Item at URL " << url;
    davItem i( url, mimeType, d, etag );
    emit itemRetrieved( i );
  }
  
  runItemsFetch( collectionUrl );
}
