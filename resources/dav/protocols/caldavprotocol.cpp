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
#include <kcalcore/event.h>
#include <kcalcore/journal.h>
#include <kcalcore/todo.h>

#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtXml/QDomDocument>

CaldavProtocol::CaldavProtocol()
{
  // Only fetch items for the last 3 months
  QString startTime = QDateTime::currentDateTimeUtc().addMonths( -3 ).toString( "yyyyMMddTHHMMssZ" );

  /*
   * Create a document like the following:
   *
   * <calendar-query>
   *   <prop>
   *     <getetag/>
   *     <resourcetype/>
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

    QDomElement queryElement = document.createElementNS( QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("calendar-query") );
    document.appendChild( queryElement );

    QDomElement propElement = document.createElementNS( QLatin1String("DAV:"), QLatin1String("prop") );
    queryElement.appendChild( propElement );

    QDomElement getetagElement = document.createElementNS( QLatin1String("DAV:"), QLatin1String("getetag") );
    propElement.appendChild( getetagElement );

    QDomElement getRTypeElement = document.createElementNS( QLatin1String("DAV:"), QLatin1String("resourcetype") );
    propElement.appendChild( getRTypeElement );

    QDomElement filterElement = document.createElementNS( QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("filter") );
    queryElement.appendChild( filterElement );

    QDomElement compfilterElement = document.createElementNS( QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("comp-filter") );

    QDomAttr nameAttribute = document.createAttribute( QLatin1String("name") );
    nameAttribute.setValue( QLatin1String("VCALENDAR") );
    compfilterElement.setAttributeNode( nameAttribute );
    filterElement.appendChild( compfilterElement );

    QDomElement subcompfilterElement = document.createElementNS( QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("comp-filter") );
    nameAttribute = document.createAttribute( QLatin1String("name") );
    nameAttribute.setValue( QLatin1String("VEVENT") );
    subcompfilterElement.setAttributeNode( nameAttribute );

    QDomElement timeRangeElement = document.createElementNS( QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("time-range") );
    QDomAttr startAttribute = document.createAttribute( QLatin1String("start") );
    startAttribute.setValue( startTime );
    timeRangeElement.setAttributeNode( startAttribute );
    subcompfilterElement.appendChild( timeRangeElement );

    compfilterElement.appendChild( subcompfilterElement );

    mItemsQueries << document;
    mItemsMimeTypes << KCalCore::Event::eventMimeType();
  }

  /*
   * Create a document like the following:
   *
   * <calendar-query>
   *   <prop>
   *     <getetag/>
   *     <resourcetype/>
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

    QDomElement queryElement = document.createElementNS( QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("calendar-query") );
    document.appendChild( queryElement );

    QDomElement propElement = document.createElementNS( QLatin1String("DAV:"), QLatin1String("prop") );
    queryElement.appendChild( propElement );

    QDomElement getetagElement = document.createElementNS( QLatin1String("DAV:"), QLatin1String("getetag") );
    propElement.appendChild( getetagElement );

    QDomElement getRTypeElement = document.createElementNS( QLatin1String("DAV:"), QLatin1String("resourcetype") );
    propElement.appendChild( getRTypeElement );

    QDomElement filterElement = document.createElementNS( QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("filter") );
    queryElement.appendChild( filterElement );

    QDomElement compfilterElement = document.createElementNS( QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("comp-filter") );

    QDomAttr nameAttribute = document.createAttribute( QLatin1String("name") );
    nameAttribute.setValue( QLatin1String("VCALENDAR") );
    compfilterElement.setAttributeNode( nameAttribute );
    filterElement.appendChild( compfilterElement );

    QDomElement subcompfilterElement = document.createElementNS( QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("comp-filter") );
    nameAttribute = document.createAttribute( QLatin1String("name") );
    nameAttribute.setValue( QLatin1String("VTODO") );
    subcompfilterElement.setAttributeNode( nameAttribute );

    QDomElement timeRangeElement = document.createElementNS( QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("time-range") );
    QDomAttr startAttribute = document.createAttribute( QLatin1String("start") );
    startAttribute.setValue( startTime );
    timeRangeElement.setAttributeNode( startAttribute );
    subcompfilterElement.appendChild( timeRangeElement );

    compfilterElement.appendChild( subcompfilterElement );

    mItemsQueries << document;
    mItemsMimeTypes << KCalCore::Todo::todoMimeType();
  }

  /*
   * Create a document like the following:
   *
   * <calendar-query>
   *   <prop>
   *     <getetag/>
   *     <resourcetype/>
   *   </prop>
   *   <filter>
   *     <comp-filter name="VCALENDAR">
   *       <comp-filter name="VJOURNAL">
   *     </comp-filter>
   *   </filter>
   * </calendar-query>
   */
  {
    QDomDocument document;

    QDomElement queryElement = document.createElementNS( QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("calendar-query") );
    document.appendChild( queryElement );

    QDomElement propElement = document.createElementNS( QLatin1String("DAV:"),QLatin1String( "prop") );
    queryElement.appendChild( propElement );

    QDomElement getetagElement = document.createElementNS( QLatin1String("DAV:"), QLatin1String("getetag") );
    propElement.appendChild( getetagElement );

    QDomElement getRTypeElement = document.createElementNS( QLatin1String("DAV:"), QLatin1String("resourcetype") );
    propElement.appendChild( getRTypeElement );

    QDomElement filterElement = document.createElementNS( QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("filter") );
    queryElement.appendChild( filterElement );

    QDomElement compfilterElement = document.createElementNS( QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("comp-filter") );

    QDomAttr nameAttribute = document.createAttribute( QLatin1String("name") );
    nameAttribute.setValue( QLatin1String("VCALENDAR") );
    compfilterElement.setAttributeNode( nameAttribute );
    filterElement.appendChild( compfilterElement );

    QDomElement subcompfilterElement = document.createElementNS( QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("comp-filter") );
    nameAttribute = document.createAttribute( QLatin1String("name") );
    nameAttribute.setValue( QLatin1String("VJOURNAL") );
    subcompfilterElement.setAttributeNode( nameAttribute );

    QDomElement timeRangeElement = document.createElementNS( QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("time-range") );
    QDomAttr startAttribute = document.createAttribute( QLatin1String("start") );
    startAttribute.setValue( startTime );
    timeRangeElement.setAttributeNode( startAttribute );
    subcompfilterElement.appendChild( timeRangeElement );

    compfilterElement.appendChild( subcompfilterElement );

    mItemsQueries << document;
    mItemsMimeTypes << KCalCore::Journal::journalMimeType();
  }
}

bool CaldavProtocol::supportsPrincipals() const
{
  return true;
}

bool CaldavProtocol::useReport() const
{
  return true;
}

bool CaldavProtocol::useMultiget() const
{
  return true;
}

QString CaldavProtocol::principalHomeSet() const
{
  return QLatin1String( "calendar-home-set" );
}

QString CaldavProtocol::principalHomeSetNS() const
{
  return QLatin1String( "urn:ietf:params:xml:ns:caldav" );
}

QDomDocument CaldavProtocol::collectionsQuery() const
{
  QDomDocument document;

  QDomElement propfindElement = document.createElementNS( QLatin1String("DAV:"), QLatin1String("propfind") );
  document.appendChild( propfindElement );

  QDomElement propElement = document.createElementNS( QLatin1String("DAV:"), QLatin1String("prop") );
  propfindElement.appendChild( propElement );

  propElement.appendChild( document.createElementNS( QLatin1String("DAV:"), QLatin1String("displayname") ) );
  propElement.appendChild( document.createElementNS( QLatin1String("DAV:"), QLatin1String("resourcetype" )) );
  propElement.appendChild( document.createElementNS( QLatin1String("http://apple.com/ns/ical/"), QLatin1String("calendar-color") ) );
  propElement.appendChild( document.createElementNS( QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("supported-calendar-component-set") ) );
  propElement.appendChild( document.createElementNS( QLatin1String("DAV:"), QLatin1String("current-user-privilege-set") ) );

  return document;
}

QString CaldavProtocol::collectionsXQuery() const
{
  //const QString query( "//*[local-name()='calendar' and namespace-uri()='urn:ietf:params:xml:ns:caldav']/ancestor::*[local-name()='prop' and namespace-uri()='DAV:']/*[local-name()='supported-calendar-component-set' and namespace-uri()='urn:ietf:params:xml:ns:caldav']/*[local-name()='comp' and namespace-uri()='urn:ietf:params:xml:ns:caldav' and (@name='VTODO' or @name='VEVENT')]/ancestor::*[local-name()='response' and namespace-uri()='DAV:']" );
  const QString query( QLatin1String("//*[local-name()='calendar' and namespace-uri()='urn:ietf:params:xml:ns:caldav']/ancestor::*[local-name()='prop' and namespace-uri()='DAV:']/ancestor::*[local-name()='response' and namespace-uri()='DAV:']") );

  return query;
}

QList<QDomDocument> CaldavProtocol::itemsQueries() const
{
  return mItemsQueries;
}

QString CaldavProtocol::mimeTypeForQuery( int index ) const
{
  return mItemsMimeTypes.at( index );
}

QDomDocument CaldavProtocol::itemsReportQuery( const QStringList &urls ) const
{
  QDomDocument document;

  QDomElement multigetElement = document.createElementNS( QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("calendar-multiget") );
  document.appendChild( multigetElement );

  QDomElement propElement = document.createElementNS( QLatin1String("DAV:"), QLatin1String("prop") );
  multigetElement.appendChild( propElement );

  propElement.appendChild( document.createElementNS( QLatin1String("DAV:"),QLatin1String("getetag") ) );
  propElement.appendChild( document.createElementNS( QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("calendar-data") ) );

  foreach ( const QString &url, urls ) {
    QDomElement hrefElement = document.createElementNS(QLatin1String( "DAV:"), QLatin1String("href") );
    const KUrl pathUrl( url );

    const QDomText textNode = document.createTextNode( pathUrl.encodedPathAndQuery() );
    hrefElement.appendChild( textNode );

    multigetElement.appendChild( hrefElement );
  }

  return document;
}

QString CaldavProtocol::responseNamespace() const
{
  return QLatin1String("urn:ietf:params:xml:ns:caldav");
}

QString CaldavProtocol::dataTagName() const
{
  return QLatin1String("calendar-data");
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

  const QDomElement propElement = DavUtils::firstChildElementNS( propstatElement, QLatin1String("DAV:"), QLatin1String("prop") );
  const QDomElement supportedcomponentElement = DavUtils::firstChildElementNS( propElement, QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("supported-calendar-component-set") );

  DavCollection::ContentTypes contentTypes;
  QDomElement compElement = DavUtils::firstChildElementNS( supportedcomponentElement, QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("comp") );

  /*
   * Assign the content-type if the server didn't return anything.
   * According to RFC4791, §5.2.3:
   * In the absence of this property, the server MUST accept all
   * component types, and the client can assume that all component
   * types are accepted.
   */
  if ( compElement.isNull() ) {
    contentTypes |= DavCollection::Calendar;
    contentTypes |= DavCollection::Events;
    contentTypes |= DavCollection::Todos;
    contentTypes |= DavCollection::FreeBusy;
    contentTypes |= DavCollection::Journal;
  }

  while ( !compElement.isNull() ) {
    const QString type = compElement.attribute( QLatin1String("name") ).toLower();
    if ( type == QLatin1String( "vcalendar" ) )
      contentTypes |= DavCollection::Calendar;
    else if ( type == QLatin1String( "vevent" ) )
      contentTypes |= DavCollection::Events;
    else if ( type == QLatin1String( "vtodo" ) )
      contentTypes |= DavCollection::Todos;
    else if ( type == QLatin1String( "vfreebusy" ) )
      contentTypes |= DavCollection::FreeBusy;
    else if ( type == QLatin1String( "vjournal" ) )
      contentTypes |= DavCollection::Journal;

    compElement = DavUtils::nextSiblingElementNS( compElement, QLatin1String("urn:ietf:params:xml:ns:caldav"), QLatin1String("comp") );
  }

  return contentTypes;
}

QString CaldavProtocol::contactsMimeType() const
{
  return QString(); // will never be called for this protocol
}
