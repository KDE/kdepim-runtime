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

#include "utils.h"
#include "davprotocolattribute.h"

#include <KDAV/DavItem>
#include <KDAV/DavUrl>
#include <KDAV/ProtocolInfo>

#include <AkonadiCore/Collection>
#include <AkonadiCore/Item>
#include <KContacts/Addressee>
#include <KContacts/VCardConverter>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/Incidence>
#include <KCalendarCore/MemoryCalendar>

#include <KLocalizedString>

#include <QByteArray>
#include <QString>
#include <QTimeZone>
#include <QRandomGenerator>

#include "davresource_debug.h"

typedef QSharedPointer<KCalendarCore::Incidence> IncidencePtr;

static QString createUniqueId()
{
    const qint64 time = QDateTime::currentMSecsSinceEpoch() / 1000;
    const int r = QRandomGenerator::global()->bounded(1000);
    const QString id = QLatin1Char('R') + QString::number(r);
    const QString uid = QString::number(time) + QLatin1Char('.') + id;
    return uid;
}

QString Utils::translatedProtocolName(KDAV::Protocol protocol)
{
    QString protocolName;

    switch (protocol) {
    case KDAV::CalDav:
        protocolName = i18n("CalDav");
        break;
    case KDAV::CardDav:
        protocolName = i18n("CardDav");
        break;
    case KDAV::GroupDav:
        protocolName = i18n("GroupDav");
        break;
    }

    return protocolName;
}

KDAV::Protocol Utils::protocolByTranslatedName(const QString &name)
{
    KDAV::Protocol protocol = KDAV::CalDav;

    if (name == i18n("CalDav")) {
        protocol = KDAV::CalDav;
    } else if (name == i18n("CardDav")) {
        protocol = KDAV::CardDav;
    } else if (name == i18n("GroupDav")) {
        protocol = KDAV::GroupDav;
    }

    return protocol;
}

KDAV::DavItem Utils::createDavItem(const Akonadi::Item &item, const Akonadi::Collection &collection, const Akonadi::Item::List &dependentItems)
{
    QByteArray rawData;
    QString mimeType;
    QUrl url;
    KDAV::DavItem davItem;
    const QString basePath = collection.remoteId();

    if (item.hasPayload<KContacts::Addressee>()) {
        const KContacts::Addressee contact = item.payload<KContacts::Addressee>();
        const QString fileName = createUniqueId();

        url = QUrl::fromUserInput(basePath + fileName + QLatin1String(".vcf"));

        const DavProtocolAttribute *protoAttr = collection.attribute<DavProtocolAttribute>();
        if (protoAttr) {
            mimeType = KDAV::ProtocolInfo::contactsMimeType(KDAV::Protocol(protoAttr->davProtocol()));
        } else {
            mimeType = KContacts::Addressee::mimeType();
        }

        KContacts::VCardConverter converter;
        // rawData is already UTF-8
        rawData = converter.exportVCard(contact, KContacts::VCardConverter::v3_0);
    } else if (item.hasPayload<IncidencePtr>()) {
        const KCalendarCore::MemoryCalendar::Ptr calendar(new KCalendarCore::MemoryCalendar(QTimeZone::systemTimeZone()));
        calendar->addIncidence(item.payload<IncidencePtr>());
        for (const Akonadi::Item &dependentItem : qAsConst(dependentItems)) {
            calendar->addIncidence(dependentItem.payload<IncidencePtr>());
        }

        const QString fileName = createUniqueId();

        url = QUrl::fromUserInput(basePath + fileName + QLatin1String(".ics"));
        mimeType = QStringLiteral("text/calendar");

        KCalendarCore::ICalFormat formatter;
        rawData = formatter.toString(calendar, QString()).toUtf8();
    }

    davItem.setContentType(mimeType);
    davItem.setData(rawData);
    davItem.setUrl(KDAV::DavUrl(url, KDAV::CalDav));
    davItem.setEtag(item.remoteRevision());

    return davItem;
}

bool Utils::parseDavData(const KDAV::DavItem &source, Akonadi::Item &target, Akonadi::Item::List &extraItems)
{
    const QString data = QString::fromUtf8(source.data());

    if (target.mimeType() == KContacts::Addressee::mimeType()) {
        KContacts::VCardConverter converter;
        const KContacts::Addressee contact = converter.parseVCard(source.data());

        if (contact.isEmpty()) {
            return false;
        }

        target.setPayloadFromData(source.data());
    } else {
        KCalendarCore::ICalFormat formatter;
        const KCalendarCore::MemoryCalendar::Ptr calendar(new KCalendarCore::MemoryCalendar(QTimeZone::systemTimeZone()));
        formatter.fromString(calendar, data);
        KCalendarCore::Incidence::List incidences = calendar->incidences();

        if (incidences.isEmpty()) {
            return false;
        }

        // All items must have the same uid in a single object.
        // Find the main VEVENT (if that's indeed what we have,
        // could be a VTODO or a VJOURNAL but that doesn't matter)
        // and then apply the recurrence exceptions
        IncidencePtr mainIncidence;
        KCalendarCore::Incidence::List exceptions;

        for (const IncidencePtr &incidence : qAsConst(incidences)) {
            if (incidence->hasRecurrenceId()) {
                qCDebug(DAVRESOURCE_LOG) << "Exception found with ID" << incidence->instanceIdentifier();
                exceptions << incidence;
            } else {
                mainIncidence = incidence;
            }
        }

        if (!mainIncidence) {
            return false;
        }

        for (const IncidencePtr &exception : qAsConst(exceptions)) {
            if (exception->status() == KCalendarCore::Incidence::StatusCanceled) {
                QDateTime exDateTime(exception->recurrenceId());
                mainIncidence->recurrence()->addExDateTime(exDateTime);
            } else {
                // The exception remote id will contain a fragment pointing to
                // its instance identifier to distinguish it from the main
                // event.
                QString rid = target.remoteId() + QLatin1String("#") + exception->instanceIdentifier();
                qCDebug(DAVRESOURCE_LOG) << "Extra incidence at" << rid;
                Akonadi::Item extraItem = target;
                extraItem.setRemoteId(rid);
                extraItem.setRemoteRevision(source.etag());
                extraItem.setMimeType(exception->mimeType());
                extraItem.setPayload<IncidencePtr>(exception);
                extraItems << extraItem;
            }
        }

        target.setPayload<IncidencePtr>(mainIncidence);
        // fix mime type for CalDAV collections
        target.setMimeType(mainIncidence->mimeType());

        /*
        foreach ( const IncidencePtr &incidence, incidences ) {
          QString rid = item.remoteId() + QLatin1String( "#" ) + incidence->instanceIdentifier();
          Akonadi::Item extraItem = item;
          extraItem.setRemoteId( rid );
          extraItem.setRemoteRevision( davItem.etag() );
          extraItem.setMimeType( incidence->mimeType() );
          extraItem.setPayload<IncidencePtr>( incidence );
          items << extraItem;
        }
        */
    }

    return true;
}
