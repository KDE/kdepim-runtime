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
#include <klocalizedstring.h>
#include <kdebug.h>

#include "caldavcalendar.h"

caldavCalendarAccessor::caldavCalendarAccessor()
  : runningQueries( 0 )
{
}

void caldavCalendarAccessor::retrieveCollections( const KUrl &url )
{
//   QString propfind =
//       "<D:propfind xmlns:D=\"DAV:\" xmlns:C=\"urn:ietf:params:xml:ns:caldav\">"
//       " <D:prop>"
//       "   <D:displayname/>"
//       "   <D:resourcetype/>"
//       "   <C:supported-calendar-component-set/>"
//       " </D:prop>"
//       "</D:propfind>";
  
  QDomDocument props;
  QDomElement root = props.createElementNS( "DAV:", "propfind" );
  props.appendChild( root );
  QDomElement e1 = props.createElementNS( "DAV:", "prop" );
  root.appendChild( e1 );
  QDomElement e2 = props.createElementNS( "DAV:", "displayname" );
  e1.appendChild( e2 );
  e2 = props.createElementNS( "DAV:", "resourcetype" );
  e1.appendChild( e2 );
  e2 = props.createElementNS( "urn:ietf:params:xml:ns:caldav", "supported-calendar-component-set" );
  e1.appendChild( e2 );
  
//   props.setContent( propfind );
  
  KIO::DavJob *job = doPropfind( url, props, "1" );
  connect( job, SIGNAL( result( KJob* ) ), this, SLOT( collectionsPropfindFinished( KJob* ) ) );
}

void caldavCalendarAccessor::collectionsPropfindFinished( KJob *j )
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
    QDomElement propstat, supportedCalendarComponentSet;
    QString href, status, displayname;
    KUrl url = job->url();
    
    tmp = r.elementsByTagNameNS( "DAV:", "href" );
    if( tmp.length() == 0 )
      continue;
    href = tmp.item( 0 ).firstChild().toText().data();
    
    if( !href.endsWith( "/" ) )
      href.append( "/" );
    
    if( href.startsWith( "/" ) )
      url.setEncodedPath( href.toAscii() );
    else
      url = href;
    href = url.prettyUrl(); //QUrl::fromPercentEncoding( url.url().toAscii() );
    
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
    
    tmp = propstat.elementsByTagNameNS( "urn:ietf:params:xml:ns:caldav", "calendar" );
    if( tmp.length() == 0 )
      continue;
    
    tmp = propstat.elementsByTagNameNS( "urn:ietf:params:xml:ns:caldav", "supported-calendar-component-set" );
    if( tmp.length() == 0 )
      continue;
    supportedCalendarComponentSet = tmp.item( 0 ).toElement();
    
    tmp = supportedCalendarComponentSet.elementsByTagNameNS( "urn:ietf:params:xml:ns:caldav", "comp" );
    if( tmp.length() == 0 )
      continue;
    int nComp = tmp.length();
    bool isCaldavResource = false;
    for( int i = 0; i < nComp; ++i ) {
      QDomElement comp = tmp.item( i ).toElement();
      QString compName = comp.attribute( "name" );
      if( compName == "VEVENT" || compName == "VTODO" )
        isCaldavResource = true;
    }
    if( !isCaldavResource )
      continue;
    
    tmp = propstat.elementsByTagNameNS( "DAV:", "displayname" );
    if( tmp.length() != 0 )
      displayname = tmp.item( 0 ).firstChild().toText().data();
    else
      displayname = "CalDAV calendar at " + href;
    
    kDebug() << "Seen collection at " << href << " (url)" << url.prettyUrl();
    emit( collectionRetrieved( href, displayname ) );
  }
  
  emit collectionsRetrieved();
}

void caldavCalendarAccessor::retrieveItems( const KUrl &url )
{
  QString collectionUrl = url.prettyUrl(); //QUrl::fromPercentEncoding( url.url().toAscii() );
  clearSeenUrls( collectionUrl );
  
  QDomDocument rep;
  QDomElement root = rep.createElementNS( "urn:ietf:params:xml:ns:caldav", "calendar-query" );
  rep.appendChild( root );
  QDomElement e1 = rep.createElementNS( "DAV:", "prop" );
  root.appendChild( e1 );
  QDomElement e2 = rep.createElementNS( "DAV:", "getetag" );
  e1.appendChild( e2 );
  e1 = rep.createElementNS( "urn:ietf:params:xml:ns:caldav", "filter" );
  root.appendChild( e1 );
  e2 = rep.createElementNS( "urn:ietf:params:xml:ns:caldav", "comp-filter" );
  QDomAttr a1 = rep.createAttribute( "name" );
  a1.setValue( "VCALENDAR" );
  e2.setAttributeNode( a1 );
  e1.appendChild( e2 );
  QDomElement e3 = rep.createElementNS( "urn:ietf:params:xml:ns:caldav", "comp-filter" );
  a1 = rep.createAttribute( "name" );
  a1.setValue( "VEVENT" );
  e3.setAttributeNode( a1 );
  e2.appendChild( e3 );
  
  KIO::DavJob *eventJob = doReport( url, rep, "1" );
  connect( eventJob, SIGNAL( result( KJob* ) ), this, SLOT( itemsReportFinished( KJob* ) ) );
  ++runningQueries;
  
  rep.clear();
  root = rep.createElementNS( "urn:ietf:params:xml:ns:caldav", "calendar-query" );
  rep.appendChild( root );
  e1 = rep.createElementNS( "DAV:", "prop" );
  root.appendChild( e1 );
  e2 = rep.createElementNS( "DAV:", "getetag" );
  e1.appendChild( e2 );
  e1 = rep.createElementNS( "urn:ietf:params:xml:ns:caldav", "filter" );
  root.appendChild( e1 );
  e2 = rep.createElementNS( "urn:ietf:params:xml:ns:caldav", "comp-filter" );
  a1 = rep.createAttribute( "name" );
  a1.setValue( "VCALENDAR" );
  e2.setAttributeNode( a1 );
  e1.appendChild( e2 );
  e3 = rep.createElementNS( "urn:ietf:params:xml:ns:caldav", "comp-filter" );
  a1 = rep.createAttribute( "name" );
  a1.setValue( "VTODO" );
  e3.setAttributeNode( a1 );
  e2.appendChild( e3 );
  
//   rep.setContent( report );
  
  KIO::DavJob *todoJob = doReport( url, rep, "1" );
  connect( todoJob, SIGNAL( result( KJob* ) ), this, SLOT( itemsReportFinished( KJob* ) ) );
  ++runningQueries;
}

void caldavCalendarAccessor::retrieveItem( const KUrl &url )
{
  // TODO: implement this, if needed
}

void caldavCalendarAccessor::itemsReportFinished( KJob *j )
{
  KIO::DavJob* job = dynamic_cast<KIO::DavJob*>( j );
  if( !job )
    return;
  
  if( job->error() ) {
    emit accessorError( job->errorString(), true );
    return;
  }
  
  QString collectionUrl = job->url().prettyUrl(); //QUrl::fromPercentEncoding( job->url().url().toAscii() );
  
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
    kDebug() << "Got href : " << href;
    if( href.startsWith( "/" ) )
      url.setEncodedPath( href.toAscii() );
    else
      url = href;
    href = url.prettyUrl(); //QUrl::fromPercentEncoding( url.url().toAscii() );
    
    // NOTE: nothing below should invalidate the item (return an error
    // and exit the function)
    seenUrl( job->url().url(), href );
    kDebug() << "Seen item at " << href << " in collection " << job->url().prettyUrl();
    
    tmp = r.elementsByTagNameNS( "DAV:", "getetag" );
    if( tmp.length() != 0 ) {
      etag = tmp.item( 0 ).firstChild().toText().data();
      
      davItemCacheStatus itemStatus = itemCacheStatus( href, etag );
      if( itemStatus == CACHED ) {
        emit itemRetrieved( getItemFromCache( href ) );
        continue;
      }
    }
    
    fetchItemsQueue[collectionUrl] << href;
  }
  
  --runningQueries;
  
  if( runningQueries == 0 )
    runItemsFetch( collectionUrl );
}

void caldavCalendarAccessor::runItemsFetch( const QString &collection )
{
  QDomDocument multiget;
  QDomElement root = multiget.createElementNS( "urn:ietf:params:xml:ns:caldav", "calendar-multiget" );
  multiget.appendChild( root );
  QDomElement prop = multiget.createElementNS( "DAV:", "prop" );
  root.appendChild( prop );
  QDomElement e1 = multiget.createElementNS( "DAV:", "getetag" );
  prop.appendChild( e1 );
  e1 = multiget.createElementNS( "urn:ietf:params:xml:ns:caldav", "calendar-data" );
  prop.appendChild( e1 );
  
  QStringList urls = fetchItemsQueue[collection];
  fetchItemsQueue[collection].clear();
  kDebug() << "Got " << urls.length() << " items for collection at " << collection;
  
  foreach( QString url, urls ) {
    e1 = multiget.createElementNS( "DAV:", "href" );
    KUrl u( url );
    QDomText t = multiget.createTextNode( u.path() );
    e1.appendChild( t );
    root.appendChild( e1 );
  }
  
  if( !urls.isEmpty() ) {
    KIO::DavJob *job = doReport( collection, multiget, "1" );
    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( multigetFinished( KJob* ) ) );
  }
  else {
    emit itemsRetrieved();
  }
}

void caldavCalendarAccessor::multigetFinished( KJob *j )
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
    href = url.prettyUrl(); //QUrl::fromPercentEncoding( url.url().toAscii() );
    
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
    davItem i( href, "text/calendar", data );
    
    davItemCacheStatus itemStatus = itemCacheStatus( href, etag );
    if( itemStatus == NOT_CACHED ) {
      emit itemRetrieved( i );
    }
    else {
      emit backendItemChanged( i );
    }
    
    addItemToCache( i, etag );
  }
  
  emit itemsRetrieved();
}
