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

#include "carddavprotocol.h"

#include <QtXml/QDomDocument>

CarddavProtocol::CarddavProtocol()
{
  QDomDocument document;

  QDomElement propfindElement = document.createElementNS( "DAV:", "propfind" );
  document.appendChild( propfindElement );

  QDomElement propElement = document.createElementNS( "DAV:", "prop" );
  propfindElement.appendChild( propElement );

  propElement.appendChild( document.createElementNS( "DAV:", "displayname" ) );
  propElement.appendChild( document.createElementNS( "DAV:", "resourcetype" ) );
  propElement.appendChild( document.createElementNS( "DAV:", "getetag" ) );

  mItemsQueries << document;
}

bool CarddavProtocol::useReport() const
{
  return false;
}

bool CarddavProtocol::useMultiget() const
{
  return false;
}

QDomDocument CarddavProtocol::collectionsQuery() const
{
  QDomDocument document;

  QDomElement propfindElement = document.createElementNS( "DAV:", "propfind" );
  document.appendChild( propfindElement );

  QDomElement propElement = document.createElementNS( "DAV:", "prop" );
  propfindElement.appendChild( propElement );

  propElement.appendChild( document.createElementNS( "DAV:", "displayname" ) );
  propElement.appendChild( document.createElementNS( "DAV:", "resourcetype" ) );

  return document;
}

QString CarddavProtocol::collectionsXQuery() const
{
  const QString query( "//*[local-name()='addressbook' and namespace-uri()='urn:ietf:params:xml:ns:carddav']/ancestor::*[local-name()='response' and namespace-uri()='DAV:']" );

  return query;
}

QList<QDomDocument> CarddavProtocol::itemsQueries() const
{
  return mItemsQueries;
}

DavCollection::ContentTypes CarddavProtocol::collectionContentTypes( const QDomElement &propstatElement ) const
{
  return DavCollection::Contacts;
}

QString CarddavProtocol::contactsMimeType() const
{
  return QLatin1String( "text/vcard" );
}
