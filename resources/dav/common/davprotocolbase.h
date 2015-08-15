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

#ifndef DAVPROTOCOLBASE_H
#define DAVPROTOCOLBASE_H

#include "davcollection.h"

#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtXml/QDomDocument>

/**
 * @short Base class for XML query builders
 */
class XMLQueryBuilder
{
public:
    typedef QSharedPointer<XMLQueryBuilder> Ptr;

    virtual ~XMLQueryBuilder();

    virtual QDomDocument buildQuery() const = 0;
    virtual QString mimeType() const = 0;

    void setParameter(const QString &key, const QVariant &value);
    QVariant parameter(const QString &key) const;

private:
    QMap<QString, QVariant> mParameters;
};

/**
 * @short Base class for various DAV groupware dialects.
 *
 * This class provides an interface to query the DAV dialect
 * specific features and abstract them.
 *
 * The functionality is implemented in:
 *   @li CaldavProtocol
 *   @li CarddavProtocol
 *   @li GroupdavProtocol
 */
class DavProtocolBase
{
public:
    /**
     * Destroys the dav protocol base.
     */
    virtual ~DavProtocolBase();

    /**
     * Returns whether the dav protocol dialect supports principal
     * queries. If true, it must return the home set it provides
     * access to with principalHomeSet() and the home set namespace
     * with principalHomeSetNS();
     */
    virtual bool supportsPrincipals() const = 0;

    /**
     * Returns whether the dav protocol dialect supports the REPORT
     * command to query all resources of a collection.
     * If not, PROPFIND command will be used instead.
     */
    virtual bool useReport() const = 0;

    /**
     * Returns whether the dav protocol dialect supports the MULTIGET command.
     *
     * If MULTIGET is supported, the content of all dav resources
     * can be fetched in ResourceBase::retrieveItems() already and
     * there is no need to call ResourceBase::retrieveItem() for every single
     * dav resource.
     *
     * Protocols that have MULTIGET capabilities must inherit from
     * DavMultigetProtocol instead of this class.
     */
    virtual bool useMultiget() const = 0;

    /**
     * Returns the home set that this protocol supports.
     */
    virtual QString principalHomeSet() const;

    /**
     * Returns the namespace of the home set.
     */
    virtual QString principalHomeSetNS() const;

    /**
     * Returns the XML document that represents the DAV query to
     * list all available DAV collections.
     */
    virtual XMLQueryBuilder::Ptr collectionsQuery() const = 0;

    /**
     * Returns the XQuery string that filters out the relevant XML elements
     * from the result returned by the query that is provided by collectionQuery().
     */
    virtual QString collectionsXQuery() const = 0;

    /**
     * Returns a list of XML documents that represent DAV queries to
     * list all available DAV resources inside a specific DAV collection.
     */
    virtual QVector<XMLQueryBuilder::Ptr> itemsQueries() const = 0;

    /**
     * Returns the possible content types for the collection that
     * is described by the passed @p propstat element of a PROPFIND result.
     */
    virtual DavCollection::ContentTypes collectionContentTypes(const QDomElement &propstat) const = 0;
};

#endif
