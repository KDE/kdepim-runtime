/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "grapheventhandler.h"

#include "graph_debug.h"

#include <KCalendarCore/Attendee>
#include <KCalendarCore/Person>
#include <KCalendarCore/Recurrence>

#include <QBitArray>
#include <QDate>
#include <QDateTime>
#include <QJsonArray>
#include <QTimeZone>

using namespace KCalendarCore;

namespace
{
// Graph dateTimeTimeZone -> QDateTime. Times are requested in UTC via the Prefer header,
// so the string is UTC; all-day events carry a date at midnight.
QDateTime parseGraphDateTime(const QJsonObject &dtz, bool allDay)
{
    const QString raw = dtz.value(QLatin1String("dateTime")).toString();
    if (raw.isEmpty()) {
        return {};
    }
    // Graph uses 7 fractional digits; QDateTime::fromString(Qt::ISODateWithMs) wants <=3.
    QString normalized = raw;
    const int dot = normalized.indexOf(QLatin1Char('.'));
    if (dot >= 0) {
        normalized = normalized.left(dot);
    }
    QDateTime dt = QDateTime::fromString(normalized, QStringLiteral("yyyy-MM-ddTHH:mm:ss"));
    dt.setTimeZone(QTimeZone::utc());
    if (allDay) {
        // Take the date verbatim (converting the UTC midnight to local time would
        // shift the day for zones west of UTC).
        return QDateTime(dt.date(), QTime(0, 0));
    }
    return dt;
}

QJsonObject utcDateTime(const QDateTime &dt, bool allDay)
{
    QJsonObject o;
    if (allDay) {
        // Take the date verbatim: converting a local midnight to UTC would shift the
        // day for zones east of UTC.
        o.insert(QStringLiteral("dateTime"), QString(dt.date().toString(Qt::ISODate) + QLatin1String("T00:00:00.0000000")));
    } else {
        o.insert(QStringLiteral("dateTime"), dt.toUTC().toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss.0000000")));
    }
    o.insert(QStringLiteral("timeZone"), QStringLiteral("UTC"));
    return o;
}

} // namespace

namespace GraphEventHandler
{
void applyRecurrence(const QJsonObject &recurrence, const Incidence::Ptr &incidence)
{
    const QJsonObject pattern = recurrence.value(QLatin1String("pattern")).toObject();
    const QJsonObject range = recurrence.value(QLatin1String("range")).toObject();
    if (pattern.isEmpty()) {
        return;
    }
    const QString type = pattern.value(QLatin1String("type")).toString();
    const int interval = pattern.value(QLatin1String("interval")).toInt(1);
    Recurrence *r = incidence->recurrence();

    if (type == QLatin1String("daily")) {
        r->setDaily(interval);
    } else if (type == QLatin1String("weekly")) {
        QBitArray days(7);
        static const QHash<QString, int> dayMap = {{QStringLiteral("monday"), 0},
                                                   {QStringLiteral("tuesday"), 1},
                                                   {QStringLiteral("wednesday"), 2},
                                                   {QStringLiteral("thursday"), 3},
                                                   {QStringLiteral("friday"), 4},
                                                   {QStringLiteral("saturday"), 5},
                                                   {QStringLiteral("sunday"), 6}};
        const QJsonArray daysOfWeek = pattern.value(QLatin1String("daysOfWeek")).toArray();
        for (const auto &d : daysOfWeek) {
            const int idx = dayMap.value(d.toString(), -1);
            if (idx >= 0) {
                days.setBit(idx);
            }
        }
        r->setWeekly(interval, days);
    } else if (type == QLatin1String("absoluteMonthly") || type == QLatin1String("relativeMonthly")) {
        r->setMonthly(interval);
    } else if (type == QLatin1String("absoluteYearly") || type == QLatin1String("relativeYearly")) {
        r->setYearly(interval);
    } else {
        qCWarning(GRAPH_LOG) << "unsupported recurrence pattern type" << type;
        return; // treat as single occurrence
    }

    const QString rangeType = range.value(QLatin1String("type")).toString();
    if (rangeType == QLatin1String("numbered")) {
        r->setDuration(range.value(QLatin1String("numberOfOccurrences")).toInt());
    } else if (rangeType == QLatin1String("endDate")) {
        const QDate end = QDate::fromString(range.value(QLatin1String("endDate")).toString(), Qt::ISODate);
        if (end.isValid()) {
            r->setEndDate(end);
        }
    }
}

QJsonObject recurrenceToJson(const Incidence::Ptr &incidence)
{
    if (!incidence->recurs()) {
        return {};
    }
    const Recurrence *r = incidence->recurrence();
    const QDate startDate = incidence->dtStart().date();

    QJsonObject pattern;
    pattern.insert(QStringLiteral("interval"), qMax(1, static_cast<int>(r->frequency())));
    switch (r->recurrenceType()) {
    case Recurrence::rDaily:
        pattern.insert(QStringLiteral("type"), QStringLiteral("daily"));
        break;
    case Recurrence::rWeekly: {
        static const char *dayNames[] = {"monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday"};
        QJsonArray daysOfWeek;
        const QBitArray days = r->days();
        for (int i = 0; i < 7 && i < days.size(); ++i) {
            if (days.testBit(i)) {
                daysOfWeek.append(QLatin1String(dayNames[i]));
            }
        }
        if (daysOfWeek.isEmpty()) {
            // Graph requires at least one day; fall back to the start day.
            daysOfWeek.append(QLatin1String(dayNames[startDate.dayOfWeek() - 1]));
        }
        pattern.insert(QStringLiteral("type"), QStringLiteral("weekly"));
        pattern.insert(QStringLiteral("daysOfWeek"), daysOfWeek);
        break;
    }
    case Recurrence::rMonthlyDay:
        pattern.insert(QStringLiteral("type"), QStringLiteral("absoluteMonthly"));
        pattern.insert(QStringLiteral("dayOfMonth"), r->monthDays().isEmpty() ? startDate.day() : r->monthDays().constFirst());
        break;
    case Recurrence::rYearlyMonth:
        pattern.insert(QStringLiteral("type"), QStringLiteral("absoluteYearly"));
        pattern.insert(QStringLiteral("dayOfMonth"), startDate.day());
        pattern.insert(QStringLiteral("month"), startDate.month());
        break;
    default:
        // No Graph equivalent (e.g. "every first Monday"); write no recurrence
        // rather than a wrong one.
        return {};
    }

    // Graph requires range.type and range.startDate.
    QJsonObject rangeObj;
    rangeObj.insert(QStringLiteral("startDate"), startDate.toString(Qt::ISODate));
    rangeObj.insert(QStringLiteral("recurrenceTimeZone"), QStringLiteral("UTC"));
    const int duration = r->duration();
    if (duration > 0) {
        rangeObj.insert(QStringLiteral("type"), QStringLiteral("numbered"));
        rangeObj.insert(QStringLiteral("numberOfOccurrences"), duration);
    } else if (duration == 0 && r->endDate().isValid()) {
        rangeObj.insert(QStringLiteral("type"), QStringLiteral("endDate"));
        rangeObj.insert(QStringLiteral("endDate"), r->endDate().toString(Qt::ISODate));
    } else {
        rangeObj.insert(QStringLiteral("type"), QStringLiteral("noEnd"));
    }

    QJsonObject recurrence;
    recurrence.insert(QStringLiteral("pattern"), pattern);
    recurrence.insert(QStringLiteral("range"), rangeObj);
    return recurrence;
}
} // namespace

namespace GraphEventHandler
{
QString mimeType()
{
    return QStringLiteral("application/x-vnd.akonadi.calendar.event");
}

KCalendarCore::Event::Ptr toEvent(const QJsonObject &json)
{
    if (json.value(QLatin1String("id")).toString().isEmpty()) {
        return {};
    }
    auto event = Event::Ptr(new Event);
    const QString uid = json.value(QLatin1String("iCalUId")).toString();
    event->setUid(uid.isEmpty() ? json.value(QLatin1String("id")).toString() : uid);
    event->setSummary(json.value(QLatin1String("subject")).toString());

    const QJsonObject body = json.value(QLatin1String("body")).toObject();
    if (body.value(QLatin1String("contentType")).toString() == QLatin1String("text")) {
        event->setDescription(body.value(QLatin1String("content")).toString());
    } else {
        event->setDescription(json.value(QLatin1String("bodyPreview")).toString());
    }

    const bool allDay = json.value(QLatin1String("isAllDay")).toBool();
    event->setAllDay(allDay);
    event->setDtStart(parseGraphDateTime(json.value(QLatin1String("start")).toObject(), allDay));
    QDateTime end = parseGraphDateTime(json.value(QLatin1String("end")).toObject(), allDay);
    if (allDay && end.isValid()) {
        // Graph's all-day end is exclusive (midnight of the next day); KCalendarCore
        // expects the inclusive last day — otherwise every all-day event shows two days.
        end = end.addDays(-1);
    }
    event->setDtEnd(end);

    event->setLocation(json.value(QLatin1String("location")).toObject().value(QLatin1String("displayName")).toString());

    const QJsonObject organizer = json.value(QLatin1String("organizer")).toObject().value(QLatin1String("emailAddress")).toObject();
    if (!organizer.isEmpty()) {
        event->setOrganizer(Person(organizer.value(QLatin1String("name")).toString(), organizer.value(QLatin1String("address")).toString()));
    }

    const QJsonArray attendees = json.value(QLatin1String("attendees")).toArray();
    for (const auto &a : attendees) {
        const QJsonObject ao = a.toObject();
        const QJsonObject email = ao.value(QLatin1String("emailAddress")).toObject();
        if (email.isEmpty()) {
            continue;
        }
        Attendee att(email.value(QLatin1String("name")).toString(), email.value(QLatin1String("address")).toString());
        const QString resp = ao.value(QLatin1String("status")).toObject().value(QLatin1String("response")).toString();
        if (resp == QLatin1String("accepted")) {
            att.setStatus(Attendee::Accepted);
        } else if (resp == QLatin1String("declined")) {
            att.setStatus(Attendee::Declined);
        } else if (resp == QLatin1String("tentativelyAccepted")) {
            att.setStatus(Attendee::Tentative);
        }
        event->addAttendee(att);
    }

    QStringList categories;
    const QJsonArray cats = json.value(QLatin1String("categories")).toArray();
    categories.reserve(cats.count());
    for (const auto &c : cats) {
        categories << c.toString();
    }
    event->setCategories(categories);

    if (json.value(QLatin1String("showAs")).toString() == QLatin1String("free")) {
        event->setTransparency(Event::Transparent);
    }
    const QString sensitivity = json.value(QLatin1String("sensitivity")).toString();
    if (sensitivity == QLatin1String("private")) {
        event->setSecrecy(Incidence::SecrecyPrivate);
    } else if (sensitivity == QLatin1String("confidential")) {
        event->setSecrecy(Incidence::SecrecyConfidential);
    }

    const QJsonObject recurrence = json.value(QLatin1String("recurrence")).toObject();
    if (!recurrence.isEmpty()) {
        applyRecurrence(recurrence, event);
    }

    return event;
}

QJsonObject toJson(const KCalendarCore::Event::Ptr &event)
{
    QJsonObject json;
    json.insert(QStringLiteral("subject"), event->summary());

    QJsonObject body;
    body.insert(QStringLiteral("contentType"), QStringLiteral("text"));
    body.insert(QStringLiteral("content"), event->description());
    json.insert(QStringLiteral("body"), body);

    const bool allDay = event->allDay();
    json.insert(QStringLiteral("isAllDay"), allDay);
    json.insert(QStringLiteral("start"), utcDateTime(event->dtStart(), allDay));
    // Mirror the inclusive/exclusive conversion from toEvent().
    json.insert(QStringLiteral("end"), utcDateTime(allDay ? event->dtEnd().addDays(1) : event->dtEnd(), allDay));

    if (!event->location().isEmpty()) {
        QJsonObject loc;
        loc.insert(QStringLiteral("displayName"), event->location());
        json.insert(QStringLiteral("location"), loc);
    }

    if (!event->categories().isEmpty()) {
        json.insert(QStringLiteral("categories"), QJsonArray::fromStringList(event->categories()));
    }
    json.insert(QStringLiteral("showAs"), event->transparency() == Event::Transparent ? QStringLiteral("free") : QStringLiteral("busy"));

    const QJsonObject recurrence = recurrenceToJson(event);
    if (!recurrence.isEmpty()) {
        json.insert(QStringLiteral("recurrence"), recurrence);
    }
    return json;
}
}
