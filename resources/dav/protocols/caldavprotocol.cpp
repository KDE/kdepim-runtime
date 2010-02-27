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

#include "caldavprotocol.h"
#include "davutils.h"

#include <kurl.h>

#include <QtCore/QStringList>
#include <QtXml/QDomDocument>

CaldavProtocol::CaldavProtocol()
{
  /*
   * Create a document like the following:
   *
   * <calendar-query>
   *   <prop>
   *     <getetag/>
   *   </prop>
   *   <filter>
   *     <comp-filter name="VCALENDAR">
   *       <comp-filter name="VEVENT">
   *     </comp-filter>
   *   </filter>
   * </calendar-query>
   */
  {
    QDomDocument document;

    QDomElement queryElement = document.createElementNS( "urn:ietf:params:xml:ns:caldav", "calendar-query" );
    document.appendChild( queryElement );

    QDomElement propElement = document.createElementNS( "DAV:", "prop" );
    queryElement.appendChild( propElement );

    QDomElement getetagElement = document.createElementNS( "DAV:", "getetag" );
    propElement.appendChild( getetagElement );

    QDomElement filterElement = document.createElementNS( "urn:ietf:params:xml:ns:caldav", "filter" );
    queryElement.appendChild( filterElement );

    QDomElement compfilterElement = document.createElementNS( "urn:ietf:params:xml:ns:caldav", "comp-filter" );

    QDomAttr nameAttribute = document.createAttribute( "name" );
    nameAttribute.setValue( "VCALENDAR" );
    compfilterElement.setAttributeNode( nameAttribute );
    filterElement.appendChild( compfilterElement );

    QDomElement subcompfilterElement = document.createElementNS( "urn:ietf:params:xml:ns:caldav", "comp-filter" );
    nameAttribute = document.createAttribute( "name" );
    nameAttribute.setValue( "VEVENT" );
    subcompfilterElement.setAttributeNode( nameAttribute );
    compfilterElement.appendChild( subcompfilterElement );

    mItemsQueries << document;
  }

  /*
   * Create a document like the following:
   *
   * <calendar-query>
   *   <prop>
   *     <getetag/>
   *   </prop>
   *   <filter>
   *     <comp-filter name="VCALENDAR">
   *       <comp-filter name="VTODO">
   *     </comp-filter>
   *   </filter>
   * </calendar-query>
   */
  {
    QDomDocument document;

    QDomElement queryElement = document.createElementNS( "urn:ietf:params:xml:ns:caldav", "calendar-query" );
    document.appendChild( queryElement );

    QDomElement propElement = document.createElementNS( "DAV:", "prop" );
    queryElement.appendChild( propElement );

    QDomElement getetagElement = document.createElementNS( "DAV:", "getetag" );
    propElement.appendChild( getetagElement );

    QDomElement filterElement = document.createElementNS( "urn:ietf:params:xml:ns:caldav", "filter" );
    queryElement.appendChild( filterElement );

    QDomElement compfilterElement = document.createElementNS( "urn:ietf:params:xml:ns:caldav", "comp-filter" );

    QDomAttr nameAttribute = document.createAttribute( "name" );
    nameAttribute.setValue( "VCALENDAR" );
    compfilterElement.setAttributeNode( nameAttribute );
    filterElement.appendChild( compfilterElement );

    QDomElement subcompfilterElement = document.createElementNS( "urn:ietf:params:xml:ns:caldav", "comp-filter" );
    nameAttribute = document.createAttribute( "name" );
    nameAttribute.setValue( "VTODO" );
    subcompfilterElement.setAttributeNode( nameAttribute );
    compfilterElement.appendChild( subcompfilterElement );

    mItemsQueries << document;
  }
}

bool CaldavProtocol::useReport() const
{
  return true;
}

bool CaldavProtocol::useMultiget() const
{
  return true;
}

QDomDocument CaldavProtocol::collectionsQuery() const
{
  QDomDocument document;

  QDomElement propfindElement = document.createElementNS( "DAV:", "propfind" );
  document.appendChild( propfindElement );

  QDomElement propElement = document.createElementNS( "DAV:", "prop" );
  propfindElement.appendChild( propElement );

  propElement.appendChild( document.createElementNS( "DAV:", "displayname" ) );
  propElement.appendChild( document.createElementNS( "DAV:", "resourcetype" ) );
  propElement.appendChild( document.createElementNS( "urn:ietf:params:xml:ns:caldav", "supported-calendar-component-set" ) );

  return document;
}

QString CaldavProtocol::collectionsXQuery() const
{
  const QString query( "//*[local-name()='calendar' and namespace-uri()='urn:ietf:params:xml:ns:caldav']/ancestor::*[local-name()='prop' and namespace-uri()='DAV:']/*[local-name()='supported-calendar-component-set' and namespace-uri()='urn:ietf:params:xml:ns:caldav']/*[local-name()='comp' and namespace-uri()='urn:ietf:params:xml:ns:caldav' and (@name='VTODO' or @name='VEVENT')]/ancestor::*[local-name()='response' and namespace-uri()='DAV:']" );

  return query;
}

QList<QDomDocument> CaldavProtocol::itemsQueries() const
{
  return mItemsQueries;
}

QDomDocument CaldavProtocol::itemsReportQuery( const QStringList &urls ) const
{
  QDomDocument document;

  QDomElement multigetElement = document.createElementNS( "urn:ietf:params:xml:ns:caldav", "calendar-multiget" );
  document.appendChild( multigetElement );

  QDomElement propElement = document.createElementNS( "DAV:", "prop" );
  multigetElement.appendChild( propElement );

  propElement.appendChild( document.createElementNS( "DAV:", "getetag" ) );
  propElement.appendChild( document.createElementNS( "urn:ietf:params:xml:ns:caldav", "calendar-data" ) );

  foreach ( const QString &url, urls ) {
    QDomElement hrefElement = document.createElementNS( "DAV:", "href" );
    const KUrl pathUrl( url );

    const QDomText textNode = document.createTextNode( pathUrl.path() );
    hrefElement.appendChild( textNode );

    multigetElement.appendChild( hrefElement );
  }

  return document;
}

QString CaldavProtocol::responseNamespace() const
{
  return "urn:ietf:params:xml:ns:caldav";
}

QString CaldavProtocol::dataTagName() const
{
  return "calendar-data";
}

DavCollection::ContentTypes CaldavProtocol::collectionContentTypes( const QDomElement &propstatElement ) const
{
  /*
   * Extract the content type information from a propstat like the following
   *   <propstat xmlns="DAV:">
   *     <prop xmlns="DAV:">
   *       <C:supported-calendar-component-set xmlns:C="urn:ietf:params:xml:ns:caldav">
   *         <C:comp xmlns:C="urn:ietf:params:xml:ns:caldav" name="VEVENT"/>
   *         <C:comp xmlns:C="urn:ietf:params:xml:ns:caldav" name="VTODO"/>
   *         <C:comp xmlns:C="urn:ietf:params:xml:ns:caldav" name="VJOURNAL"/>
   *         <C:comp xmlns:C="urn:ietf:params:xml:ns:caldav" name="VTIMEZONE"/>
   *         <C:comp xmlns:C="urn:ietf:params:xml:ns:caldav" name="VFREEBUSY"/>
   *       </C:supported-calendar-component-set>
   *       <resourcetype xmlns="DAV:">
   *         <collection xmlns="DAV:"/>
   *         <C:calendar xmlns:C="urn:ietf:params:xml:ns:caldav"/>
   *         <C:schedule-calendar xmlns:C="urn:ietf:params:xml:ns:caldav"/>
   *       </resourcetype>
   *       <displayname xmlns="DAV:">Test1 User</displayname>
   *     </prop>
   *     <status xmlns="DAV:">HTTP/1.1 200 OK</status>
   *   </propstat>
   */

  const QDomElement propElement = DavUtils::firstChildElementNS( propstatElement, "DAV:", "prop" );
  const QDomElement supportedcomponentElement = DavUtils::firstChildElementNS( propElement, "urn:ietf:params:xml:ns:caldav", "supported-calendar-component-set" );

  DavCollection::ContentTypes contentTypes;
  QDomElement compElement = DavUtils::firstChildElementNS( supportedcomponentElement, "urn:ietf:params:xml:ns:caldav", "comp" );
  while ( !compElement.isNull() ) {
    const QString type = compElement.attribute( "name" ).toLower();
    if ( type == QLatin1String( "vevent" ) )
      contentTypes |= DavCollection::Events;
    else if ( type == QLatin1String( "vtodo" ) )
      contentTypes |= DavCollection::Todos;

    compElement = DavUtils::nextSiblingElementNS( compElement, "urn:ietf:params:xml:ns:caldav", "comp" );
  }

  return contentTypes;
}

QString CaldavProtocol::contactsMimeType() const
{
  return QString(); // will never be called for this protocol
}
