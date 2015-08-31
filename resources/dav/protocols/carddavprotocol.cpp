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

class CarddavCollectionQueryBuilder : public XMLQueryBuilder
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
        propElement.appendChild(document.createElementNS(QStringLiteral("http://calendarserver.org/ns/"), QStringLiteral("getctag")));

        return document;
    }

    virtual QString mimeType() const
    {
        return QString();
    }
};

class CarddavListItemsQueryBuilder : public XMLQueryBuilder
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
        return KContacts::Addressee::mimeType();
    }
};

class CarddavMultigetQueryBuilder : public XMLQueryBuilder
{
public:
    virtual QDomDocument buildQuery() const
    {
        QDomDocument document;
        QStringList urls = parameter(QStringLiteral("urls")).toStringList();
        if (urls.isEmpty())
            return document;

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
            const QUrl pathUrl = QUrl::fromUserInput(url);
            QString encodedUrl = QString::fromAscii(pathUrl.encodedPath());
            if ( pathUrl.hasQuery() ) {
                encodedUrl.append(QStringLiteral("?"));
                encodedUrl.append(QString::fromAscii(pathUrl.encodedQuery()));
            }

            const QDomText textNode = document.createTextNode(encodedUrl);
            hrefElement.appendChild(textNode);

            multigetElement.appendChild(hrefElement);
        }

        return document;
    }

    virtual QString mimeType() const
    {
        return QString();
    }
};

CarddavProtocol::CarddavProtocol()
{
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

XMLQueryBuilder::Ptr CarddavProtocol::collectionsQuery() const
{
    return XMLQueryBuilder::Ptr(new CarddavCollectionQueryBuilder());
}

QString CarddavProtocol::collectionsXQuery() const
{
    const QString query(QStringLiteral("//*[local-name()='addressbook' and namespace-uri()='urn:ietf:params:xml:ns:carddav']/ancestor::*[local-name()='response' and namespace-uri()='DAV:']"));

    return query;
}

QVector<XMLQueryBuilder::Ptr> CarddavProtocol::itemsQueries() const
{
    QVector<XMLQueryBuilder::Ptr> ret;
    ret << XMLQueryBuilder::Ptr(new CarddavListItemsQueryBuilder());
    return ret;
}

XMLQueryBuilder::Ptr CarddavProtocol::itemsReportQuery(const QStringList &urls) const
{
    XMLQueryBuilder::Ptr ret(new CarddavMultigetQueryBuilder());
    ret->setParameter(QStringLiteral("urls"), urls);
    return ret;
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
