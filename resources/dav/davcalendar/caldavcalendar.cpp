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

#include "caldavcalendar.h"

#include <kdebug.h>
#include <kio/davjob.h>
#include <klocalizedstring.h>

#include <QtCore/QMutexLocker>
#include <QtXml/QDomDocument>

caldavImplementation::caldavImplementation()
{
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

  itemsQueries_ << rep;

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

  itemsQueries_ << rep;
}

bool caldavImplementation::useReport() const
{
  return true;
}

bool caldavImplementation::useMultiget() const
{
  return true;
}

QDomDocument caldavImplementation::collectionsQuery() const
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
  e2 = props.createElementNS( "urn:ietf:params:xml:ns:caldav", "supported-calendar-component-set" );
  e1.appendChild( e2 );

  return props;
}

QString caldavImplementation::collectionsXQuery() const
{
  QString xquery( "//*[local-name()='calendar' and namespace-uri()='urn:ietf:params:xml:ns:caldav']/ancestor::*[local-name()='prop' and namespace-uri()='DAV:']/*[local-name()='supported-calendar-component-set' and namespace-uri()='urn:ietf:params:xml:ns:caldav']/*[local-name()='comp' and namespace-uri()='urn:ietf:params:xml:ns:caldav' and (@name='VTODO' or @name='VEVENT')]/ancestor::*[local-name()='response' and namespace-uri()='DAV:']" );
  return xquery;
}

const QList<QDomDocument>& caldavImplementation::itemsQueries() const
{
  return itemsQueries_;
}

QDomDocument caldavImplementation::itemsReportQuery( const QStringList &urls ) const
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

  foreach( QString url, urls ) {
    e1 = multiget.createElementNS( "DAV:", "href" );
    KUrl u( url );
    QDomText t = multiget.createTextNode( u.path() );
    e1.appendChild( t );
    root.appendChild( e1 );
  }

  return multiget;
}
