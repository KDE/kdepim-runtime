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

#ifndef DAVCOLLECTION_H
#define DAVCOLLECTION_H

#include "davutils.h"

#include <QtCore/QList>
#include <QtCore/QString>

/**
 * @short A helper class to store information about DAV collection.
 *
 * This class is used as container to transfer information about DAV
 * collections between the Akonadi resource and the DAV jobs.
 */
class DavCollection
{
  public:
    /**
     * Defines a list of DAV collection objects.
     */
    typedef QList<DavCollection> List;

    /**
     * Describes the possible content type of the DAV collection.
     */
    enum ContentType
    {
      Events = 1,    ///< The collection can contain event DAV resources.
      Todos = 2,     ///< The collection can contain todo DAV resources.
      Contacts = 4,  ///< The collection can contain contact DAV resources.
      FreeBusy = 8,  ///< The collection can contain free/busy information.
      Journal = 16,  ///< The collection can contain journal DAV resources.
      Calendar = 32  ///< The collection can contain anything calendar-related.
    };
    Q_DECLARE_FLAGS( ContentTypes, ContentType )

    /**
     * Creates an empty DAV collection.
     */
    DavCollection();

    /**
     * Creates a new DAV collection.
     *
     * @param protocol The DAV protocol dialect the collection comes from.
     * @param url The url that identifies the collection.
     * @param displayName The display name of the collection.
     * @param contentTypes The possible content types of the collection.
     */
    DavCollection( DavUtils::Protocol protocol, const QString &url, const QString &displayName, ContentTypes contentTypes );

    /**
     * Sets the DAV @p protocol dialect the collection comes from.
     */
    void setProtocol( DavUtils::Protocol protocol );

    /**
     * Returns the DAV protocol dialect the collection comes from.
     */
    DavUtils::Protocol protocol() const;

    /**
     * Sets the @p url that identifies the collection.
     */
    void setUrl( const QString &url );

    /**
     * Returns the url that identifies the collection.
     */
    QString url() const;

    /**
     * Sets the display @p name of the collection.
     */
    void setDisplayName( const QString &name );

    /**
     * Returns the display name of the collection.
     */
    QString displayName() const;

    /**
     * Sets the possible content @p types of the collection.
     */
    void setContentTypes( ContentTypes types );

    /**
     * Returns the possible content types of the collection.
     */
    ContentTypes contentTypes() const;

  private:
    DavUtils::Protocol mProtocol;
    QString mUrl;
    QString mDisplayName;
    ContentTypes mContentTypes;
};

Q_DECLARE_OPERATORS_FOR_FLAGS( DavCollection::ContentTypes )

#endif
