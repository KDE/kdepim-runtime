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

#include <kio/davjob.h>
#include <kio/jobclasses.h>
#include <kio/job.h>
#include <kio/deletejob.h>
#include <klocalizedstring.h>
#include <kdebug.h>
#include <kstandarddirs.h>

#include "davaccessor.h"

davItem::davItem()
  : url(), contentType(), data()
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

davAccessor::davAccessor()
{
}

davAccessor::~davAccessor()
{
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

void davAccessor::jobWarning( KJob* j, const QString &p, const QString &r )
{
  kDebug() << "Warning : " << p;
  emit accessorError( p, true );
}

void davAccessor::validateCache()
{
  kDebug() << "Beginning cache validation";
  
  QSet<QString> latest;
  QMapIterator<QString, QSet<QString> > it( lastSeenItems );
  while( it.hasNext() ) {
    it.next();
    latest += it.value();
  }
  
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

void davAccessor::clearSeenUrls( const QString &url )
{
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
