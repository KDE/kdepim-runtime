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

#include <QDomDocument>

#include <kio/davjob.h>
#include <kio/jobclasses.h>
#include <kio/job.h>
#include <klocalizedstring.h>
#include <kdebug.h>

#include "groupdavcalendar.h"

groupdavCalendarAccessor::groupdavCalendarAccessor()
{
}

void groupdavCalendarAccessor::retrieveCollections( const KUrl &url )
{
  QDomDocument props;
  QDomElement root = props.createElementNS( "DAV:", "propfind" );
  props.appendChild( root );
  QDomElement e1 = props.createElementNS( "DAV:", "prop" );
  root.appendChild( e1 );
  QDomElement e2 = props.createElementNS( "DAV:", "displayname" );
  e1.appendChild( e2 );
  e2 = props.createElementNS( "DAV:", "resourcetype" );
  e1.appendChild( e2 );
  
  KIO::DavJob *job = doPropfind( url, props, "1" );
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( collectionsPropfindFinished( KJob* ) ) );
}

void groupdavCalendarAccessor::retrieveItems( const KUrl &url )
{
  QDomDocument props;
  QDomElement root = props.createElementNS( "DAV:", "propfind" );
  props.appendChild( root );
  QDomElement e1 = props.createElementNS( "DAV:", "prop" );
  root.appendChild( e1 );
  QDomElement e2 = props.createElementNS( "DAV:", "displayname" );
  e1.appendChild( e2 );
  e2 = props.createElementNS( "DAV:", "resourcetype" );
  e1.appendChild( e2 );
  e2 = props.createElementNS( "DAV:", "getetag" );
  e1.appendChild( e2 );
  
  KIO::DavJob *job = doPropfind( url, props, "1" );
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( itemsPropfindFinished( KJob* ) ) );
}

void groupdavCalendarAccessor::retrieveItem( const KUrl &url )
{
  // TODO: implement this, if needed
}

void groupdavCalendarAccessor::getItem( const KUrl &url )
{
  KIO::StoredTransferJob *job = KIO::storedGet( url, KIO::Reload, KIO::HideProgressInfo | KIO::DefaultFlags );
  job->addMetaData( "PropagateHttpHeader", "true" );
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( itemGetFinished( KJob* ) ) );
}

void groupdavCalendarAccessor::collectionsPropfindFinished( KJob *j )
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
  if( !root.hasChildNodes() ) {
    emit accessorError( i18n( "No calendars found" ), true );
    return;
  }
  
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
    if( tmp.length() == 0 )
      continue;
    href = tmp.item( 0 ).firstChild().toText().data();
    
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
    
    tmp = propstat.elementsByTagNameNS( "http://groupdav.org/", "vevent-collection" );
    if( tmp.length() == 0 ) {
      tmp = propstat.elementsByTagNameNS( "http://groupdav.org/", "vtodo-collection" );
      if( tmp.length() == 0 )
        continue;
    }
    
    tmp = propstat.elementsByTagNameNS( "DAV:", "displayname" );
    if( tmp.length() != 0 )
      displayname = tmp.item( 0 ).firstChild().toText().data();
    else
      displayname = "GroupDAV calendar at " + href;
    
    emit( collectionRetrieved( href, displayname ) );
  }
  
  emit collectionsRetrieved();
}

void groupdavCalendarAccessor::itemsPropfindFinished( KJob *j )
{
  KIO::DavJob* job = dynamic_cast<KIO::DavJob*>( j );
  if( !job )
    return;
  
  if( job->error() ) {
    emit accessorError( job->errorString(), true );
    return;
  }
  
  QString collectionUrl = job->url().prettyUrl();
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
    
    fetchItemsQueue << href;
  }
  
  runItemsFetch();
}

void groupdavCalendarAccessor::itemGetFinished( KJob *j )
{
  KIO::StoredTransferJob *job = dynamic_cast<KIO::StoredTransferJob*>( j );
  if( !job ) {
    emit accessorError( i18n( "Problem with the retrieval job" ), true );
    return;
  }
  
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
  
  runItemsFetch();
}

void groupdavCalendarAccessor::runItemsFetch()
{
  QMutexLocker locker( &fetchItemsQueueMtx );
  
  if( fetchItemsQueue.isEmpty() ) {
    emit itemsRetrieved();
    return;
  }
  
  QString next = fetchItemsQueue.takeFirst();
  locker.unlock();
  getItem( next );
}
