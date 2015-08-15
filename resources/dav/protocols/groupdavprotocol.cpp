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

#include "groupdavprotocol.h"

#include "davutils.h"

#include <QtXml/QDomDocument>

class GroupdavCollectionQueryBuilder : public XMLQueryBuilder
{
public:
    virtual QDomDocument buildQuery() const
    {
        QDomDocument document;

        QDomElement propfindElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("propfind"));
        document.appendChild(propfindElement);

        QDomElement propElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("prop"));
        propfindElement.appendChild(propElement);

        propElement.appendChild(document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("displayname")));
        propElement.appendChild(document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("resourcetype")));

        return document;
    }

    virtual QString mimeType() const
    {
        return QString();
    }
};

class GroupdavItemQueryBuilder : public XMLQueryBuilder
{
public:
    virtual QDomDocument buildQuery() const
    {
        QDomDocument document;

        QDomElement propfindElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("propfind"));
        document.appendChild(propfindElement);

        QDomElement propElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("prop"));
        propfindElement.appendChild(propElement);

        propElement.appendChild(document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("displayname")));
        propElement.appendChild(document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("resourcetype")));
        propElement.appendChild(document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("getetag")));

        return document;
    }

    virtual QString mimeType() const
    {
        return QString();
    }
};

GroupdavProtocol::GroupdavProtocol()
{
}

bool GroupdavProtocol::supportsPrincipals() const
{
    return false;
}

bool GroupdavProtocol::useReport() const
{
    return false;
}

bool GroupdavProtocol::useMultiget() const
{
    return false;
}

XMLQueryBuilder::Ptr GroupdavProtocol::collectionsQuery() const
{
    return XMLQueryBuilder::Ptr(new GroupdavCollectionQueryBuilder());
}

QString GroupdavProtocol::collectionsXQuery() const
{
    const QString query(QStringLiteral("//*[(local-name()='vevent-collection' or local-name()='vtodo-collection' or local-name()='vcard-collection') and namespace-uri()='http://groupdav.org/']/ancestor::*[local-name()='response' and namespace-uri()='DAV:']"));

    return query;
}

QVector<XMLQueryBuilder::Ptr> GroupdavProtocol::itemsQueries() const
{
    QVector<XMLQueryBuilder::Ptr> ret;
    ret << XMLQueryBuilder::Ptr(new GroupdavItemQueryBuilder());
    return ret;
}

DavCollection::ContentTypes GroupdavProtocol::collectionContentTypes(const QDomElement &propstatElement) const
{
    /*
     * Extract the content type information from a propstat like the following
     *
     *  <propstat>
     *    <status>HTTP/1.1 200 OK</status>
     *    <prop>
     *      <displayname>Tasks</displayname>
     *      <resourcetype>
     *        <collection/>
     *        <G:vtodo-collection xmlns:G="http://groupdav.org/"/>
     *      </resourcetype>
     *      <getlastmodified>Sat, 30 Jan 2010 17:52:41 -0100</getlastmodified>
     *    </prop>
     *  </propstat>
     */

    const QDomElement propElement = DavUtils::firstChildElementNS(propstatElement, QStringLiteral("DAV:"), QStringLiteral("prop"));
    const QDomElement resourcetypeElement = DavUtils::firstChildElementNS(propElement, QStringLiteral("DAV:"), QStringLiteral("resourcetype"));

    DavCollection::ContentTypes contentTypes;

    if (!DavUtils::firstChildElementNS(resourcetypeElement, QStringLiteral("http://groupdav.org/"), QStringLiteral("vevent-collection")).isNull()) {
        contentTypes |= DavCollection::Events;
    }

    if (!DavUtils::firstChildElementNS(resourcetypeElement, QStringLiteral("http://groupdav.org/"), QStringLiteral("vtodo-collection")).isNull()) {
        contentTypes |= DavCollection::Todos;
    }

    if (!DavUtils::firstChildElementNS(resourcetypeElement, QStringLiteral("http://groupdav.org/"), QStringLiteral("vcard-collection")).isNull()) {
        contentTypes |= DavCollection::Contacts;
    }

    return contentTypes;
}
