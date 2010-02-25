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

#ifndef DAVUTILS_H
#define DAVUTILS_H

#include <QtCore/QList>
#include <QtXml/QDomElement>

#include <KUrl>

/**
 * @short A namespace that contains helper methods for DAV functionality.
 */
namespace DavUtils
{
  /**
   * Describes the DAV protocol dialect.
   */
  enum Protocol
  {
    CalDav = 0,   ///< The CalDav protocol as defined in http://caldav.calconnect.org
    CardDav,      ///< The CardDav protocol as defined in http://carddav.calconnect.org
    GroupDav      ///< The GroupDav protocol as defined in http://www.groupdav.org
  };

  /**
   * Returns the i18n'ed name of the given DAV @p protocol dialect.
   */
  QString protocolName( Protocol protocol );

  /**
   * @short A helper class to combine url and protocol of a DAV url.
   */
  class DavUrl
  {
    public:
      /**
       * Defines a list of DAV url objects.
       */
      typedef QList<DavUrl> List;

      /**
       * Creates an empty DAV url.
       */
      DavUrl();

      /**
       * Creates a new DAV url.
       *
       * @param url The url that identifies the DAV object.
       * @param protocol The DAV protocol dialect that is used to retrieve the DAV object.
       */
      DavUrl( const KUrl &url, Protocol protocol );

      /**
       * Sets the @p url that identifies the DAV object.
       */
      void setUrl( const KUrl &url );

      /**
       * Returns the url that identifies the DAV object.
       */
      KUrl url() const;

      /**
       * Sets the DAV @p protocol dialect that is used to retrieve the DAV object.
       */
      void setProtocol( Protocol protocol );

      /**
       * Returns the DAV protocol dialect that is used to retrieve the DAV object.
       */
      Protocol protocol() const;

    private:
      KUrl mUrl;
      Protocol mProtocol;
  };

  /**
   * Returns the first child element of @p parent that has the given @p tagName and is part of the @p namespaceUri.
   */
  QDomElement firstChildElementNS( const QDomElement &parent, const QString &namespaceUri, const QString &tagName );

  /**
   * Returns the next sibling element of @p element that has the given @p tagName and is part of the @p namespaceUri.
   */
  QDomElement nextSiblingElementNS( const QDomElement &element, const QString &namespaceUri, const QString &tagName );
}

#endif
