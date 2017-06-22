/*
 *    Copyright (C) 2017 Daniel Vrátil <dvratil@kde.org>
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "eventslistjob.h"
#include "settings.h"
#include "graph.h"

#include <QJsonObject>
#include <QDateTime>

#include <KCalCore/Event>

EventsListJob::EventsListJob(const Akonadi::Collection &col, QObject *parent)
    : ListJob(col, parent)
{
    setRequest(QStringLiteral("me/events"),
               { QStringLiteral("id"),
                 QStringLiteral("name"),
                 QStringLiteral("description"),
                 QStringLiteral("place"),
                 QStringLiteral("start_time"),
                 QStringLiteral("end_time"),
                 QStringLiteral("owner"),
                 QStringLiteral("is_canceled")
               },
               { { QStringLiteral("type"), col.remoteId() } });
}

EventsListJob::~EventsListJob()
{
}

KCalCore::Incidence::Status EventsListJob::parseStatus(const QJsonObject &data) const
{
    const auto isCanceled = data.constFind(QLatin1String("is_canceled"));
    if (isCanceled != data.constEnd()) {
        if (isCanceled->toBool() == true) {
            return KCalCore::Incidence::StatusCanceled;
        }
    }

    switch (Graph::rsvpFromString(collection().remoteId())) {
    case Graph::Attending:
        return KCalCore::Incidence::StatusConfirmed;
    case Graph::MaybeAttending:
        return KCalCore::Incidence::StatusTentative;
    case Graph::NotResponded:
        return KCalCore::Incidence::StatusNeedsAction;
    case Graph::Declined:
    case Graph::Birthday:
        break;
    }

    return KCalCore::Incidence::StatusNone;
}

bool EventsListJob::shouldHaveAlarm(const Akonadi::Collection &col) const
{
    const auto s = Settings::self();
    const auto rsvp = Graph::rsvpFromString(col.remoteId());
    return (rsvp == Graph::Attending && s->attendingReminders())
            || (rsvp == Graph::MaybeAttending && s->maybeAttendingReminders())
            || (rsvp == Graph::Declined && s->notAttendingReminders())
            || (rsvp == Graph::NotResponded && s->notRespondedToReminders());
}


QDateTime EventsListJob::parseDateTime(const QString &str) const
{
    // Format is: 2017-06-20T16:30:00+0200
    // Parse the absolute time
    auto dt = QDateTime::fromString(str.left(19), QStringLiteral("yyyy-MM-ddTHH:mm:ss"));
    // Parse the offset
    const auto tz = str.rightRef(5);
    const int sec = (tz.left(1) == QLatin1String("+") ? 1 : -1) * tz.mid(1, 2).toInt() * 3600 + tz.right(2).toInt() * 60;
    dt.setOffsetFromUtc(sec);

    // Convert to local time
    return dt.toLocalTime();
}


Akonadi::Item EventsListJob::handleResponse(const QJsonObject &data)
{
    auto event = KCalCore::Event::Ptr::create();
    event->setSummary(data.value(QLatin1String("name")).toString());

    const auto dataEnd = data.constEnd();
    const auto placeIt = data.constFind(QLatin1String("place"));
    if (placeIt != dataEnd) {
        const auto place = placeIt->toObject();
        QStringList locationStr;
        const auto placeEnd = place.constEnd();
        const auto name = place.constFind(QLatin1String("name"));
        if (name != placeEnd) {
            locationStr << name->toString();
        }
        const auto locationIt = place.constFind(QLatin1String("location"));
        if (locationIt != placeEnd) {
            const auto location = locationIt->toObject();
            for (const auto &loc : { QLatin1String("street"), QLatin1String("city"),
                                    QLatin1String("zip"), QLatin1String("country") }) {
                const auto it = location.constFind(loc);
                if (it != placeEnd) {
                    locationStr << it->toString();
                }
            }
            // no name, no address, try GPS coordinates
            if (locationStr.size() < 1) {
                const auto longitude = place.constFind(QLatin1String("longitude"));
                if (longitude != placeEnd) {
                    event->setGeoLongitude(longitude->toDouble());
                }
                const auto latitude = place.constFind(QLatin1String("latitude"));
                if (latitude != placeEnd) {
                    event->setGeoLatitude(latitude->toDouble());
                }
            }
        }
        if (!locationStr.isEmpty()) {
            event->setLocation(locationStr.join(QStringLiteral(", ")));
        }
    }

    const QString dtStart = data.value(QLatin1String("start_time")).toString();
    event->setDtStart(KDateTime(parseDateTime(dtStart)));

    const auto endTime = data.constFind(QLatin1String("end_time"));
    if (endTime != dataEnd) {
        event->setDtEnd(KDateTime(parseDateTime(endTime->toString())));
    }

    QString description = data.value(QLatin1String("description")).toString();
    description += QStringLiteral("\n\nhttps://www.facebook.com/events/%1").arg(data.value(QLatin1String("id")).toString());
    event->setDescription(description);

    const auto status = parseStatus(data);
    event->setStatus(status);
    if (status == KCalCore::Incidence::StatusCanceled) {
        event->setTransparency(KCalCore::Event::Transparent);
    } else {
        event->setTransparency(KCalCore::Event::Opaque);
    }

    event->setUid(data.value(QLatin1String("id")).toString());

    const auto owner = data.constFind(QLatin1String("owner"));
    if (owner != dataEnd) {
        event->setOrganizer(owner->toObject().value(QLatin1String("name")).toString());
    }

    if (shouldHaveAlarm(collection())) {
        auto alarm = KCalCore::Alarm::Ptr::create(event.data());
        alarm->setDisplayAlarm(event->summary());
        alarm->setStartOffset({ -Settings::self()->eventReminderHours() * 3600,
                                KCalCore::Duration::Seconds });
        alarm->setEnabled(true);
        event->addAlarm(alarm);
    }

    Akonadi::Item item;
    item.setMimeType(KCalCore::Event::eventMimeType());
    item.setRemoteId(event->uid());
    item.setGid(event->uid());
    item.setParentCollection(collection());
    item.setPayload(event);
    return item;
}
