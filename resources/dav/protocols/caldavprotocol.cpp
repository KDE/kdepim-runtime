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

#include <KCalCore/Event>
#include <KCalCore/Journal>
#include <KCalCore/Todo>

#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtXml/QDomDocument>

class CaldavCollectionQueryBuilder : public XMLQueryBuilder
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
        propElement.appendChild(document.createElementNS(QStringLiteral("http://apple.com/ns/ical/"), QStringLiteral("calendar-color")));
        propElement.appendChild(document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("supported-calendar-component-set")));
        propElement.appendChild(document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("current-user-privilege-set")));

        return document;
    }

    virtual QString mimeType() const
    {
        return QString();
    }
};

class CaldavListEventQueryBuilder : public XMLQueryBuilder
{
public:
    virtual QDomDocument buildQuery() const
    {
        QString startTime = parameter("start").toString();
        QString endTime = parameter("end").toString();
        QDomDocument document;

        QDomElement queryElement = document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("calendar-query"));
        document.appendChild(queryElement);

        QDomElement propElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("prop"));
        queryElement.appendChild(propElement);

        QDomElement getetagElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("getetag"));
        propElement.appendChild(getetagElement);

        QDomElement getRTypeElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("resourcetype"));
        propElement.appendChild(getRTypeElement);

        QDomElement filterElement = document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("filter"));
        queryElement.appendChild(filterElement);

        QDomElement compfilterElement = document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("comp-filter"));

        QDomAttr nameAttribute = document.createAttribute(QStringLiteral("name"));
        nameAttribute.setValue(QStringLiteral("VCALENDAR"));
        compfilterElement.setAttributeNode(nameAttribute);
        filterElement.appendChild(compfilterElement);

        QDomElement subcompfilterElement = document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("comp-filter"));
        nameAttribute = document.createAttribute(QStringLiteral("name"));
        nameAttribute.setValue(QStringLiteral("VEVENT"));
        subcompfilterElement.setAttributeNode(nameAttribute);

        if (!startTime.isEmpty() || !endTime.isEmpty()) {
            QDomElement timeRangeElement = document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("time-range"));

            if (!startTime.isEmpty()) {
                QDomAttr startAttribute = document.createAttribute(QStringLiteral("start"));
                startAttribute.setValue(startTime);
                timeRangeElement.setAttributeNode(startAttribute);
            }

            if (!endTime.isEmpty()) {
                QDomAttr endAttribute = document.createAttribute(QStringLiteral("end"));
                endAttribute.setValue(startTime);
                timeRangeElement.setAttributeNode(endAttribute);
            }

            subcompfilterElement.appendChild(timeRangeElement);
        }

        compfilterElement.appendChild(subcompfilterElement);

        return document;
    }

    virtual QString mimeType() const
    {
        return KCalCore::Event::eventMimeType();
    }
};

class CaldavListTodoQueryBuilder : public XMLQueryBuilder
{
public:
    virtual QDomDocument buildQuery() const
    {
        QString startTime = parameter("start").toString();
        QString endTime = parameter("end").toString();
        QDomDocument document;

        QDomElement queryElement = document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("calendar-query"));
        document.appendChild(queryElement);

        QDomElement propElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("prop"));
        queryElement.appendChild(propElement);

        QDomElement getetagElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("getetag"));
        propElement.appendChild(getetagElement);

        QDomElement getRTypeElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("resourcetype"));
        propElement.appendChild(getRTypeElement);

        QDomElement filterElement = document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("filter"));
        queryElement.appendChild(filterElement);

        QDomElement compfilterElement = document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("comp-filter"));

        QDomAttr nameAttribute = document.createAttribute(QStringLiteral("name"));
        nameAttribute.setValue(QStringLiteral("VCALENDAR"));
        compfilterElement.setAttributeNode(nameAttribute);
        filterElement.appendChild(compfilterElement);

        QDomElement subcompfilterElement = document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("comp-filter"));
        nameAttribute = document.createAttribute(QStringLiteral("name"));
        nameAttribute.setValue(QStringLiteral("VTODO"));
        subcompfilterElement.setAttributeNode(nameAttribute);

        if (!startTime.isEmpty() || !endTime.isEmpty()) {
            QDomElement timeRangeElement = document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("time-range"));

            if (!startTime.isEmpty()) {
                QDomAttr startAttribute = document.createAttribute(QStringLiteral("start"));
                startAttribute.setValue(startTime);
                timeRangeElement.setAttributeNode(startAttribute);
            }

            if (!endTime.isEmpty()) {
                QDomAttr endAttribute = document.createAttribute(QStringLiteral("end"));
                endAttribute.setValue(startTime);
                timeRangeElement.setAttributeNode(endAttribute);
            }

            subcompfilterElement.appendChild(timeRangeElement);
        }

        compfilterElement.appendChild(subcompfilterElement);

        return document;
    }

    virtual QString mimeType() const
    {
        return KCalCore::Todo::todoMimeType();
    }
};

class CaldavListJournalQueryBuilder : public XMLQueryBuilder
{
public:
    virtual QDomDocument buildQuery() const
    {
        QString startTime = parameter("start").toString();
        QString endTime = parameter("end").toString();
        QDomDocument document;

        QDomElement queryElement = document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("calendar-query"));
        document.appendChild(queryElement);

        QDomElement propElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("prop"));
        queryElement.appendChild(propElement);

        QDomElement getetagElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("getetag"));
        propElement.appendChild(getetagElement);

        QDomElement getRTypeElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("resourcetype"));
        propElement.appendChild(getRTypeElement);

        QDomElement filterElement = document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("filter"));
        queryElement.appendChild(filterElement);

        QDomElement compfilterElement = document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("comp-filter"));

        QDomAttr nameAttribute = document.createAttribute(QStringLiteral("name"));
        nameAttribute.setValue(QStringLiteral("VCALENDAR"));
        compfilterElement.setAttributeNode(nameAttribute);
        filterElement.appendChild(compfilterElement);

        QDomElement subcompfilterElement = document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("comp-filter"));
        nameAttribute = document.createAttribute(QStringLiteral("name"));
        nameAttribute.setValue(QStringLiteral("VJOURNAL"));
        subcompfilterElement.setAttributeNode(nameAttribute);

        if (!startTime.isEmpty() || !endTime.isEmpty()) {
            QDomElement timeRangeElement = document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("time-range"));

            if (!startTime.isEmpty()) {
                QDomAttr startAttribute = document.createAttribute(QStringLiteral("start"));
                startAttribute.setValue(startTime);
                timeRangeElement.setAttributeNode(startAttribute);
            }

            if (!endTime.isEmpty()) {
                QDomAttr endAttribute = document.createAttribute(QStringLiteral("end"));
                endAttribute.setValue(startTime);
                timeRangeElement.setAttributeNode(endAttribute);
            }

            subcompfilterElement.appendChild(timeRangeElement);
        }

        compfilterElement.appendChild(subcompfilterElement);

        return document;
    }

    virtual QString mimeType() const
    {
        return KCalCore::Journal::journalMimeType();
    }
};

class CaldavMultigetQueryBuilder : public XMLQueryBuilder
{
public:
    virtual QDomDocument buildQuery() const
    {
        QDomDocument document;
        QStringList urls = parameter(QStringLiteral("urls")).toStringList();
        if (urls.isEmpty())
            return document;

        QDomElement multigetElement = document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("calendar-multiget"));
        document.appendChild(multigetElement);

        QDomElement propElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("prop"));
        multigetElement.appendChild(propElement);

        propElement.appendChild(document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("getetag")));
        propElement.appendChild(document.createElementNS(QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("calendar-data")));

        foreach (const QString &url, urls) {
            QDomElement hrefElement = document.createElementNS(QStringLiteral("DAV:"), QStringLiteral("href"));
            const QUrl pathUrl(url);
            const QString encodedUrl = pathUrl.path() + ( pathUrl.hasQuery() ?
                                                            QLatin1String("?")+pathUrl.query() :
                                                            QString("") );

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

CaldavProtocol::CaldavProtocol()
{
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
    return QStringLiteral("calendar-home-set");
}

QString CaldavProtocol::principalHomeSetNS() const
{
    return QStringLiteral("urn:ietf:params:xml:ns:caldav");
}

XMLQueryBuilder::Ptr CaldavProtocol::collectionsQuery() const
{
    return XMLQueryBuilder::Ptr(new CaldavCollectionQueryBuilder());
}

QString CaldavProtocol::collectionsXQuery() const
{
    //const QString query( "//*[local-name()='calendar' and namespace-uri()='urn:ietf:params:xml:ns:caldav']/ancestor::*[local-name()='prop' and namespace-uri()='DAV:']/*[local-name()='supported-calendar-component-set' and namespace-uri()='urn:ietf:params:xml:ns:caldav']/*[local-name()='comp' and namespace-uri()='urn:ietf:params:xml:ns:caldav' and (@name='VTODO' or @name='VEVENT')]/ancestor::*[local-name()='response' and namespace-uri()='DAV:']" );
    const QString query(QStringLiteral("//*[local-name()='calendar' and namespace-uri()='urn:ietf:params:xml:ns:caldav']/ancestor::*[local-name()='prop' and namespace-uri()='DAV:']/ancestor::*[local-name()='response' and namespace-uri()='DAV:']"));

    return query;
}

QVector<XMLQueryBuilder::Ptr> CaldavProtocol::itemsQueries() const
{
    QVector<XMLQueryBuilder::Ptr> ret;
    ret << XMLQueryBuilder::Ptr( new CaldavListEventQueryBuilder() );
    ret << XMLQueryBuilder::Ptr( new CaldavListTodoQueryBuilder() );
    ret << XMLQueryBuilder::Ptr( new CaldavListJournalQueryBuilder() );
    return ret;
}

XMLQueryBuilder::Ptr CaldavProtocol::itemsReportQuery(const QStringList &urls) const
{
    XMLQueryBuilder::Ptr ret(new CaldavMultigetQueryBuilder());
    ret->setParameter(QStringLiteral("urls"), urls);
    return ret;
}

QString CaldavProtocol::responseNamespace() const
{
    return QStringLiteral("urn:ietf:params:xml:ns:caldav");
}

QString CaldavProtocol::dataTagName() const
{
    return QStringLiteral("calendar-data");
}

DavCollection::ContentTypes CaldavProtocol::collectionContentTypes(const QDomElement &propstatElement) const
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

    const QDomElement propElement = DavUtils::firstChildElementNS(propstatElement, QStringLiteral("DAV:"), QStringLiteral("prop"));
    const QDomElement supportedcomponentElement = DavUtils::firstChildElementNS(propElement, QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("supported-calendar-component-set"));

    DavCollection::ContentTypes contentTypes;
    QDomElement compElement = DavUtils::firstChildElementNS(supportedcomponentElement, QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("comp"));

    /*
     * Assign the content-type if the server didn't return anything.
     * According to RFC4791, §5.2.3:
     * In the absence of this property, the server MUST accept all
     * component types, and the client can assume that all component
     * types are accepted.
     */
    if (compElement.isNull()) {
        contentTypes |= DavCollection::Calendar;
        contentTypes |= DavCollection::Events;
        contentTypes |= DavCollection::Todos;
        contentTypes |= DavCollection::FreeBusy;
        contentTypes |= DavCollection::Journal;
    }

    while (!compElement.isNull()) {
        const QString type = compElement.attribute(QStringLiteral("name")).toLower();
        if (type == QLatin1String("vcalendar")) {
            contentTypes |= DavCollection::Calendar;
        } else if (type == QLatin1String("vevent")) {
            contentTypes |= DavCollection::Events;
        } else if (type == QLatin1String("vtodo")) {
            contentTypes |= DavCollection::Todos;
        } else if (type == QLatin1String("vfreebusy")) {
            contentTypes |= DavCollection::FreeBusy;
        } else if (type == QLatin1String("vjournal")) {
            contentTypes |= DavCollection::Journal;
        }

        compElement = DavUtils::nextSiblingElementNS(compElement, QStringLiteral("urn:ietf:params:xml:ns:caldav"), QStringLiteral("comp"));
    }

    return contentTypes;
}
