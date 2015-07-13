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

#include <kcontacts/addressee.h>

#include <QtCore/QStringList>
#include <QtXml/QDomDocument>

CarddavProtocol::CarddavProtocol()
{
    QDomDocument document;

    QDomElement propfindElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("propfind"));
    document.appendChild(propfindElement);

    QDomElement propElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("prop"));
    propfindElement.appendChild(propElement);

    propElement.appendChild(document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("displayname")));
    propElement.appendChild(document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("resourcetype")));
    propElement.appendChild(document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("getetag")));

    mItemsQueries << document;
    mItemsMimeTypes << KContacts::Addressee::mimeType();
}

bool CarddavProtocol::supportsPrincipals() const
{
    return true;
}

bool CarddavProtocol::useReport() const
{
    return false;
}

bool CarddavProtocol::useMultiget() const
{
    return true;
}

QString CarddavProtocol::principalHomeSet() const
{
    return QStringLiteral("addressbook-home-set");
}

QString CarddavProtocol::principalHomeSetNS() const
{
    return QStringLiteral("urn:ietf:params:xml:ns:carddav");
}

QDomDocument CarddavProtocol::collectionsQuery() const
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

QString CarddavProtocol::collectionsXQuery() const
{
    const QString query(QStringLiteral("//*[local-name()='addressbook' and namespace-uri()='urn:ietf:params:xml:ns:carddav']/ancestor::*[local-name()='response' and namespace-uri()='DAV:']"));

    return query;
}

QVector<QDomDocument> CarddavProtocol::itemsQueries() const
{
    return mItemsQueries;
}

QString CarddavProtocol::mimeTypeForQuery(int index) const
{
    return mItemsMimeTypes.at(index);
}

QDomDocument CarddavProtocol::itemsReportQuery(const QStringList &urls) const
{
    QDomDocument document;

    QDomElement multigetElement = document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:carddav"), QStringLiteral("addressbook-multiget"));
    document.appendChild(multigetElement);

    QDomElement propElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("prop"));
    multigetElement.appendChild(propElement);

    propElement.appendChild(document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("getetag")));
    QDomElement addressDataElement = document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:carddav"), QStringLiteral("address-data"));
    addressDataElement.appendChild(document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("allprop")));
    propElement.appendChild(addressDataElement);

    foreach (const QString &url, urls) {
        QDomElement hrefElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("href"));
        const KUrl pathUrl(url);

        const QDomText textNode = document.createTextNode(pathUrl.encodedPathAndQuery());
        hrefElement.appendChild(textNode);

        multigetElement.appendChild(hrefElement);
    }

    return document;
}

QString CarddavProtocol::responseNamespace() const
{
    return QStringLiteral("urn:ietf:params:xml:ns:carddav");
}

QString CarddavProtocol::dataTagName() const
{
    return QStringLiteral("address-data");
}

DavCollection::ContentTypes CarddavProtocol::collectionContentTypes(const QDomElement &) const
{
    return DavCollection::Contacts;
}

QString CarddavProtocol::contactsMimeType() const
{
    return QStringLiteral("text/vcard");
}
