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

class DavItem;
namespace Akonadi {
  class Collection;
  class Item;
}

/**
 * @short A namespace that contains helper methods for DAV functionality.
 */
namespace DavUtils
{
  /**
   * Describes the DAV protocol dialect.
   */
  enum Protocol {
    CalDav = 0,   ///< The CalDav protocol as defined in http://caldav.calconnect.org
    CardDav,      ///< The CardDav protocol as defined in http://carddav.calconnect.org
    GroupDav      ///< The GroupDav protocol as defined in http://www.groupdav.org
  };

  /**
   * Describes the DAV privileges on a resource (see RFC3744)
   */
  enum Privilege {
    None = 0x0,
    Read = 0x1,
    Write = 0x2,
    WriteProperties = 0x4,
    WriteContent = 0x8,
    Unlock = 0x10,
    ReadAcl = 0x20,
    ReadCurrentUserPrivilegeSet = 0x40,
    WriteAcl = 0x80,
    Bind = 0x100,
    Unbind = 0x200,
    All = 0x400
  };
  Q_DECLARE_FLAGS( Privileges, Privilege )
  Q_DECLARE_OPERATORS_FOR_FLAGS( Privileges )

  /**
   * Returns the untranslated name of the given DAV @p protocol dialect.
   */
  QLatin1String protocolName( Protocol protocol );

  /**
   * Returns the i18n'ed name of the given DAV @p protocol dialect.
   */
  QString translatedProtocolName( Protocol protocol );

  /**
   * Returns the protocol matching the given name. This is the opposite of
   * DavUtils::protocolName().
   */
  Protocol protocolByName( const QString &name );

  /**
   * Returns the protocol matching the given i18n'ed @p name. This is the opposite
   * of DavUtils::translatedProtocolName().
   */
  Protocol protocolByTranslatedName( const QString &name );

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

  /**
   * Extracts privileges from @p element. The <privilege/> tags are expected to be first level children of @p element.
   */
  Privileges extractPrivileges( const QDomElement &element );

  /**
   * Parses a single <privilege/> tag and returns the final Privileges.
   */
  Privileges parsePrivilege( const QDomElement &element );

  /**
   * Creates a new DavItem from the Akonadi::Item @p item.
   *
   * The returned item will have no payload (DavItem::data() will return an empty
   * QByteArray) if the @p item payload is not recognized.
   */
  DavItem createDavItem( const Akonadi::Item &item, const Akonadi::Collection &collection );
}

#endif
