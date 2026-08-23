/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    Unit tests for the Graph event JSON <-> KCalendarCore::Event mapping.
*/

#include "calendar/grapheventhandler.h"

#include <KCalendarCore/Attendee>
#include <KCalendarCore/Recurrence>

#include <QJsonArray>
#include <QJsonObject>
#include <QTest>
#include <QTimeZone>

using namespace KCalendarCore;

namespace
{
QJsonObject graphDateTime(const QString &dateTime)
{
    QJsonObject o;
    o.insert(QStringLiteral("dateTime"), dateTime);
    o.insert(QStringLiteral("timeZone"), QStringLiteral("UTC"));
    return o;
}

QJsonObject emailAddress(const QString &name, const QString &address)
{
    QJsonObject email;
    email.insert(QStringLiteral("name"), name);
    email.insert(QStringLiteral("address"), address);
    QJsonObject o;
    o.insert(QStringLiteral("emailAddress"), email);
    return o;
}
} // namespace

class GraphEventHandlerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void shouldReturnNullWithoutId()
    {
        QJsonObject json;
        json.insert(QStringLiteral("subject"), QStringLiteral("No id"));
        QVERIFY(!GraphEventHandler::toEvent(json));
    }

    void shouldMapBasicFields()
    {
        QJsonObject json;
        json.insert(QStringLiteral("id"), QStringLiteral("AAMkAGI2"));
        json.insert(QStringLiteral("iCalUId"), QStringLiteral("040000008200E00074C5B7101A82E008"));
        json.insert(QStringLiteral("subject"), QStringLiteral("Team Meeting"));
        QJsonObject body;
        body.insert(QStringLiteral("contentType"), QStringLiteral("text"));
        body.insert(QStringLiteral("content"), QStringLiteral("Agenda: Ä, Ö, Ü"));
        json.insert(QStringLiteral("body"), body);
        json.insert(QStringLiteral("isAllDay"), false);
        // Graph returns 7 fractional digits; the parser must cope with them.
        json.insert(QStringLiteral("start"), graphDateTime(QStringLiteral("2026-07-10T09:30:00.0000000")));
        json.insert(QStringLiteral("end"), graphDateTime(QStringLiteral("2026-07-10T10:00:00.0000000")));
        QJsonObject location;
        location.insert(QStringLiteral("displayName"), QStringLiteral("Room 1"));
        json.insert(QStringLiteral("location"), location);
        json.insert(QStringLiteral("organizer"), emailAddress(QStringLiteral("Alice"), QStringLiteral("alice@example.com")));
        json.insert(QStringLiteral("categories"), QJsonArray{QStringLiteral("Work"), QStringLiteral("Travel")});
        json.insert(QStringLiteral("showAs"), QStringLiteral("free"));
        json.insert(QStringLiteral("sensitivity"), QStringLiteral("private"));

        const Event::Ptr event = GraphEventHandler::toEvent(json);
        QVERIFY(event);
        QCOMPARE(event->uid(), QStringLiteral("040000008200E00074C5B7101A82E008"));
        QCOMPARE(event->summary(), QStringLiteral("Team Meeting"));
        QCOMPARE(event->description(), QStringLiteral("Agenda: Ä, Ö, Ü"));
        QVERIFY(!event->allDay());
        // QDateTime comparison is instant-based, so this is time-zone independent.
        QCOMPARE(event->dtStart(), QDateTime(QDate(2026, 7, 10), QTime(9, 30), QTimeZone::utc()));
        QCOMPARE(event->dtEnd(), QDateTime(QDate(2026, 7, 10), QTime(10, 0), QTimeZone::utc()));
        QCOMPARE(event->location(), QStringLiteral("Room 1"));
        QCOMPARE(event->organizer().email(), QStringLiteral("alice@example.com"));
        QCOMPARE(event->categories(), QStringList({QStringLiteral("Work"), QStringLiteral("Travel")}));
        QCOMPARE(event->transparency(), Event::Transparent);
        QCOMPARE(event->secrecy(), Incidence::SecrecyPrivate);
    }

    void shouldFallBackToGraphIdAsUid()
    {
        QJsonObject json;
        json.insert(QStringLiteral("id"), QStringLiteral("AAMkAGI2"));
        const Event::Ptr event = GraphEventHandler::toEvent(json);
        QVERIFY(event);
        QCOMPARE(event->uid(), QStringLiteral("AAMkAGI2"));
    }

    void shouldMapAttendeesWithStatus()
    {
        QJsonObject json;
        json.insert(QStringLiteral("id"), QStringLiteral("AAMkAGI2"));
        QJsonArray attendees;
        const auto withStatus = [](QJsonObject attendee, const QString &response) {
            QJsonObject status;
            status.insert(QStringLiteral("response"), response);
            attendee.insert(QStringLiteral("status"), status);
            return attendee;
        };
        attendees.append(withStatus(emailAddress(QStringLiteral("Bob"), QStringLiteral("bob@example.com")), QStringLiteral("accepted")));
        attendees.append(withStatus(emailAddress(QStringLiteral("Carol"), QStringLiteral("carol@example.com")), QStringLiteral("declined")));
        attendees.append(withStatus(emailAddress(QStringLiteral("Dave"), QStringLiteral("dave@example.com")), QStringLiteral("tentativelyAccepted")));
        json.insert(QStringLiteral("attendees"), attendees);

        const Event::Ptr event = GraphEventHandler::toEvent(json);
        QVERIFY(event);
        const Attendee::List list = event->attendees();
        QCOMPARE(list.size(), 3);
        QCOMPARE(list.at(0).email(), QStringLiteral("bob@example.com"));
        QCOMPARE(list.at(0).status(), Attendee::Accepted);
        QCOMPARE(list.at(1).status(), Attendee::Declined);
        QCOMPARE(list.at(2).status(), Attendee::Tentative);
    }

    void shouldMapWeeklyRecurrence()
    {
        QJsonObject json;
        json.insert(QStringLiteral("id"), QStringLiteral("AAMkAGI2"));
        json.insert(QStringLiteral("start"), graphDateTime(QStringLiteral("2026-07-06T08:00:00.0000000")));
        json.insert(QStringLiteral("end"), graphDateTime(QStringLiteral("2026-07-06T09:00:00.0000000")));
        QJsonObject pattern;
        pattern.insert(QStringLiteral("type"), QStringLiteral("weekly"));
        pattern.insert(QStringLiteral("interval"), 2);
        pattern.insert(QStringLiteral("daysOfWeek"), QJsonArray{QStringLiteral("monday"), QStringLiteral("wednesday")});
        QJsonObject range;
        range.insert(QStringLiteral("type"), QStringLiteral("numbered"));
        range.insert(QStringLiteral("numberOfOccurrences"), 10);
        QJsonObject recurrence;
        recurrence.insert(QStringLiteral("pattern"), pattern);
        recurrence.insert(QStringLiteral("range"), range);
        json.insert(QStringLiteral("recurrence"), recurrence);

        const Event::Ptr event = GraphEventHandler::toEvent(json);
        QVERIFY(event);
        Recurrence *r = event->recurrence();
        QVERIFY(r->recurs());
        QCOMPARE(r->recurrenceType(), static_cast<ushort>(Recurrence::rWeekly));
        QCOMPARE(r->frequency(), 2);
        QCOMPARE(r->duration(), 10);
        const QBitArray days = r->days();
        QVERIFY(days.testBit(0)); // Monday
        QVERIFY(!days.testBit(1));
        QVERIFY(days.testBit(2)); // Wednesday
    }

    void shouldMapDailyRecurrenceWithEndDate()
    {
        QJsonObject json;
        json.insert(QStringLiteral("id"), QStringLiteral("AAMkAGI2"));
        json.insert(QStringLiteral("start"), graphDateTime(QStringLiteral("2026-07-06T08:00:00.0000000")));
        json.insert(QStringLiteral("end"), graphDateTime(QStringLiteral("2026-07-06T09:00:00.0000000")));
        QJsonObject pattern;
        pattern.insert(QStringLiteral("type"), QStringLiteral("daily"));
        pattern.insert(QStringLiteral("interval"), 1);
        QJsonObject range;
        range.insert(QStringLiteral("type"), QStringLiteral("endDate"));
        range.insert(QStringLiteral("endDate"), QStringLiteral("2026-08-31"));
        QJsonObject recurrence;
        recurrence.insert(QStringLiteral("pattern"), pattern);
        recurrence.insert(QStringLiteral("range"), range);
        json.insert(QStringLiteral("recurrence"), recurrence);

        const Event::Ptr event = GraphEventHandler::toEvent(json);
        QVERIFY(event);
        Recurrence *r = event->recurrence();
        QVERIFY(r->recurs());
        QCOMPARE(r->recurrenceType(), static_cast<ushort>(Recurrence::rDaily));
        QCOMPARE(r->endDate(), QDate(2026, 8, 31));
    }

    void shouldWriteJsonForTimedEvent()
    {
        Event::Ptr event(new Event);
        event->setSummary(QStringLiteral("Review"));
        event->setDescription(QStringLiteral("Bring the numbers"));
        event->setAllDay(false);
        event->setDtStart(QDateTime(QDate(2026, 7, 10), QTime(9, 30), QTimeZone::utc()));
        event->setDtEnd(QDateTime(QDate(2026, 7, 10), QTime(10, 0), QTimeZone::utc()));
        event->setLocation(QStringLiteral("Room 2"));
        event->setCategories(QStringList{QStringLiteral("Work")});

        const QJsonObject json = GraphEventHandler::toJson(event);
        QCOMPARE(json.value(QLatin1String("subject")).toString(), QStringLiteral("Review"));
        const QJsonObject body = json.value(QLatin1String("body")).toObject();
        QCOMPARE(body.value(QLatin1String("contentType")).toString(), QStringLiteral("text"));
        QCOMPARE(body.value(QLatin1String("content")).toString(), QStringLiteral("Bring the numbers"));
        QCOMPARE(json.value(QLatin1String("isAllDay")).toBool(), false);
        const QJsonObject start = json.value(QLatin1String("start")).toObject();
        QCOMPARE(start.value(QLatin1String("dateTime")).toString(), QStringLiteral("2026-07-10T09:30:00.0000000"));
        QCOMPARE(start.value(QLatin1String("timeZone")).toString(), QStringLiteral("UTC"));
        QCOMPARE(json.value(QLatin1String("location")).toObject().value(QLatin1String("displayName")).toString(), QStringLiteral("Room 2"));
        QCOMPARE(json.value(QLatin1String("categories")).toArray(), QJsonArray{QStringLiteral("Work")});
        QCOMPARE(json.value(QLatin1String("showAs")).toString(), QStringLiteral("busy"));
    }

    void shouldReadAllDayEventWithInclusiveEnd()
    {
        QJsonObject json;
        json.insert(QStringLiteral("id"), QStringLiteral("AAMkAGI2"));
        json.insert(QStringLiteral("subject"), QStringLiteral("Birthday"));
        json.insert(QStringLiteral("isAllDay"), true);
        // Graph's all-day end is exclusive: a one-day event ends at the next midnight.
        json.insert(QStringLiteral("start"), graphDateTime(QStringLiteral("2026-07-10T00:00:00.0000000")));
        json.insert(QStringLiteral("end"), graphDateTime(QStringLiteral("2026-07-11T00:00:00.0000000")));

        const Event::Ptr event = GraphEventHandler::toEvent(json);
        QVERIFY(event);
        QVERIFY(event->allDay());
        QCOMPARE(event->dtStart().date(), QDate(2026, 7, 10));
        // KCalendarCore's all-day end is inclusive: same day, not two days.
        QCOMPARE(event->dtEnd().date(), QDate(2026, 7, 10));
    }

    void shouldWriteJsonForAllDayEvent()
    {
        Event::Ptr event(new Event);
        event->setSummary(QStringLiteral("Holiday"));
        event->setAllDay(true);
        // Inclusive KCalendarCore semantics: a one-day event starts and ends on the 10th.
        event->setDtStart(QDateTime(QDate(2026, 7, 10), QTime(0, 0)));
        event->setDtEnd(QDateTime(QDate(2026, 7, 10), QTime(0, 0)));

        const QJsonObject json = GraphEventHandler::toJson(event);
        QCOMPARE(json.value(QLatin1String("isAllDay")).toBool(), true);
        // All-day events must carry a midnight timestamp, whatever the local zone.
        QCOMPARE(json.value(QLatin1String("start")).toObject().value(QLatin1String("dateTime")).toString(), QStringLiteral("2026-07-10T00:00:00.0000000"));
        // ...and Graph expects the exclusive end (midnight of the following day).
        QCOMPARE(json.value(QLatin1String("end")).toObject().value(QLatin1String("dateTime")).toString(), QStringLiteral("2026-07-11T00:00:00.0000000"));
    }

    void shouldWriteWeeklyRecurrence()
    {
        Event::Ptr event(new Event);
        event->setSummary(QStringLiteral("Weekly sync"));
        event->setDtStart(QDateTime(QDate(2026, 7, 6), QTime(8, 0), QTimeZone::utc())); // a Monday
        event->setDtEnd(QDateTime(QDate(2026, 7, 6), QTime(9, 0), QTimeZone::utc()));
        QBitArray days(7);
        days.setBit(0); // Monday
        days.setBit(2); // Wednesday
        event->recurrence()->setWeekly(2, days);
        event->recurrence()->setDuration(10);

        const QJsonObject json = GraphEventHandler::toJson(event);
        const QJsonObject recurrence = json.value(QLatin1String("recurrence")).toObject();
        QVERIFY(!recurrence.isEmpty());
        const QJsonObject pattern = recurrence.value(QLatin1String("pattern")).toObject();
        QCOMPARE(pattern.value(QLatin1String("type")).toString(), QStringLiteral("weekly"));
        QCOMPARE(pattern.value(QLatin1String("interval")).toInt(), 2);
        QCOMPARE(pattern.value(QLatin1String("daysOfWeek")).toArray(), (QJsonArray{QStringLiteral("monday"), QStringLiteral("wednesday")}));
        const QJsonObject range = recurrence.value(QLatin1String("range")).toObject();
        QCOMPARE(range.value(QLatin1String("type")).toString(), QStringLiteral("numbered"));
        QCOMPARE(range.value(QLatin1String("numberOfOccurrences")).toInt(), 10);
        QCOMPARE(range.value(QLatin1String("startDate")).toString(), QStringLiteral("2026-07-06"));
    }

    void shouldRoundTripRecurrence()
    {
        // read -> write must preserve the rule.
        QJsonObject json;
        json.insert(QStringLiteral("id"), QStringLiteral("AAMkAGI2"));
        json.insert(QStringLiteral("start"), graphDateTime(QStringLiteral("2026-07-06T08:00:00.0000000")));
        json.insert(QStringLiteral("end"), graphDateTime(QStringLiteral("2026-07-06T09:00:00.0000000")));
        QJsonObject pattern;
        pattern.insert(QStringLiteral("type"), QStringLiteral("weekly"));
        pattern.insert(QStringLiteral("interval"), 2);
        pattern.insert(QStringLiteral("daysOfWeek"), QJsonArray{QStringLiteral("monday"), QStringLiteral("wednesday")});
        QJsonObject range;
        range.insert(QStringLiteral("type"), QStringLiteral("numbered"));
        range.insert(QStringLiteral("numberOfOccurrences"), 10);
        QJsonObject recurrence;
        recurrence.insert(QStringLiteral("pattern"), pattern);
        recurrence.insert(QStringLiteral("range"), range);
        json.insert(QStringLiteral("recurrence"), recurrence);

        const QJsonObject back = GraphEventHandler::toJson(GraphEventHandler::toEvent(json)).value(QLatin1String("recurrence")).toObject();
        const QJsonObject backPattern = back.value(QLatin1String("pattern")).toObject();
        QCOMPARE(backPattern.value(QLatin1String("type")).toString(), QStringLiteral("weekly"));
        QCOMPARE(backPattern.value(QLatin1String("interval")).toInt(), 2);
        QCOMPARE(backPattern.value(QLatin1String("daysOfWeek")).toArray(), (QJsonArray{QStringLiteral("monday"), QStringLiteral("wednesday")}));
        QCOMPARE(back.value(QLatin1String("range")).toObject().value(QLatin1String("numberOfOccurrences")).toInt(), 10);
    }

    void shouldRoundTripThroughJson()
    {
        QJsonObject json;
        json.insert(QStringLiteral("id"), QStringLiteral("AAMkAGI2"));
        json.insert(QStringLiteral("subject"), QStringLiteral("Round trip"));
        QJsonObject body;
        body.insert(QStringLiteral("contentType"), QStringLiteral("text"));
        body.insert(QStringLiteral("content"), QStringLiteral("Body text"));
        json.insert(QStringLiteral("body"), body);
        json.insert(QStringLiteral("isAllDay"), false);
        json.insert(QStringLiteral("start"), graphDateTime(QStringLiteral("2026-07-10T09:30:00.0000000")));
        json.insert(QStringLiteral("end"), graphDateTime(QStringLiteral("2026-07-10T10:00:00.0000000")));

        const QJsonObject back = GraphEventHandler::toJson(GraphEventHandler::toEvent(json));
        QCOMPARE(back.value(QLatin1String("subject")), json.value(QLatin1String("subject")));
        QCOMPARE(back.value(QLatin1String("body")), json.value(QLatin1String("body")));
        QCOMPARE(back.value(QLatin1String("start")), json.value(QLatin1String("start")));
        QCOMPARE(back.value(QLatin1String("end")), json.value(QLatin1String("end")));
    }
};

QTEST_GUILESS_MAIN(GraphEventHandlerTest)

#include "grapheventhandlertest.moc"
