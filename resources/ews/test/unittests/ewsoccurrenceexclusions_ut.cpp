/*
    SPDX-FileCopyrightText: 2026 The kdepim-runtime authors

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QDateTime>
#include <QTest>
#include <QTimeZone>
#include <QXmlStreamReader>

#include <KCalendarCore/Event>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>
#include <KCalendarCore/Recurrence>

#include "ewsitem.h"
#include "ewsoccurrence.h"
#include "ewsoccurrenceexclusions.h"

namespace
{

EwsItem itemFromXml(const QByteArray &xml)
{
    QXmlStreamReader reader(xml);
    while (!reader.atEnd() && !reader.isStartElement()) {
        reader.readNext();
    }

    return EwsItem(reader);
}

QDateTime utc(int year, int month, int day, int hour = 0, int minute = 0)
{
    return QDateTime(QDate(year, month, day), QTime(hour, minute), QTimeZone::UTC);
}

const QByteArray icalHead =
    "BEGIN:VCALENDAR\n"
    "VERSION:2.0\n"
    "PRODID:-//kdepim-runtime//ewsoccurrenceexclusions_ut//EN\n";

const QByteArray masterEvent =
    "BEGIN:VEVENT\n"
    "UID:series-1\n"
    "DTSTART:20261102T090000Z\n"
    "DTEND:20261102T100000Z\n"
    "RRULE:FREQ=WEEKLY;COUNT=4\n"
    "SUMMARY:Weekly\n"
    "END:VEVENT\n";

const QByteArray movedEvent =
    "BEGIN:VEVENT\n"
    "UID:series-1\n"
    "RECURRENCE-ID:20261109T090000Z\n"
    "DTSTART:20261109T130000Z\n"
    "DTEND:20261109T140000Z\n"
    "SUMMARY:Weekly moved\n"
    "END:VEVENT\n";

const QByteArray secondMovedEvent =
    "BEGIN:VEVENT\n"
    "UID:series-1\n"
    "RECURRENCE-ID:20261116T090000Z\n"
    "DTSTART:20261116T130000Z\n"
    "DTEND:20261116T140000Z\n"
    "SUMMARY:Weekly moved again\n"
    "END:VEVENT\n";

// An occurrence delivered on its own may still carry the rule of its series.
const QByteArray recurringMovedEvent =
    "BEGIN:VEVENT\n"
    "UID:series-1\n"
    "RECURRENCE-ID:20261109T090000Z\n"
    "DTSTART:20261109T130000Z\n"
    "DTEND:20261109T140000Z\n"
    "RRULE:FREQ=WEEKLY;COUNT=4\n"
    "SUMMARY:Weekly moved\n"
    "END:VEVENT\n";

KCalendarCore::Calendar::Ptr calendarFrom(const QByteArray &events)
{
    KCalendarCore::ICalFormat format;
    KCalendarCore::Calendar::Ptr calendar(new KCalendarCore::MemoryCalendar(QTimeZone::utc()));
    if (!format.fromString(calendar, QString::fromLatin1(icalHead + events + "END:VCALENDAR\n"))) {
        return {};
    }

    return calendar;
}

KCalendarCore::Event::Ptr weeklySeriesEvent()
{
    const auto events = calendarFrom(masterEvent)->events();
    if (events.isEmpty()) {
        return {};
    }

    return events.first();
}

} // namespace

class UtEwsOccurrenceExclusions : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void occurrenceParser();
    void occurrenceParserSkipsUnknownElements();
    void exclusionDateTimes_data();
    void exclusionDateTimes();
    void movedOccurrences();
    void pickIncidence();
    void seriesExclusions();
    void exclusionsAppliedToRecurrence();
};

void UtEwsOccurrenceExclusions::occurrenceParser()
{
    static const QByteArray xml =
        "<t:CalendarItem xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
        "<t:ItemId Id=\"master\" ChangeKey=\"1\"/>"
        "<t:ModifiedOccurrences>"
        "<t:Occurrence>"
        "<t:ItemId Id=\"moved-1\" ChangeKey=\"2\"/>"
        "<t:Start>2026-11-09T09:00:00Z</t:Start>"
        "<t:End>2026-11-09T10:00:00Z</t:End>"
        "<t:OriginalStart>2026-11-09T08:00:00Z</t:OriginalStart>"
        "</t:Occurrence>"
        "</t:ModifiedOccurrences>"
        "<t:DeletedOccurrences>"
        "<t:DeletedOccurrence>"
        "<t:Start>2026-11-16T09:00:00Z</t:Start>"
        "</t:DeletedOccurrence>"
        "</t:DeletedOccurrences>"
        "</t:CalendarItem>";

    const EwsItem item = itemFromXml(xml);
    QVERIFY(item.isValid());

    const auto modified = item[EwsItemFieldModifiedOccurrences].value<EwsOccurrence::List>();
    QCOMPARE(modified.size(), 1);
    const EwsOccurrence &moved = modified.first();
    QVERIFY(moved.isValid());
    QCOMPARE(moved.itemId().id(), QStringLiteral("moved-1"));
    QCOMPARE(moved.start().toUTC(), utc(2026, 11, 9, 9));
    QCOMPARE(moved.end().toUTC(), utc(2026, 11, 9, 10));
    // The original slot is a value of its own and must not overwrite Start.
    QCOMPARE(moved.originalStart().toUTC(), utc(2026, 11, 9, 8));

    const auto deleted = item[EwsItemFieldDeletedOccurrences].value<EwsOccurrence::List>();
    QCOMPARE(deleted.size(), 1);
    const EwsOccurrence &cancelled = deleted.first();
    QVERIFY(cancelled.isValid());
    QCOMPARE(cancelled.start().toUTC(), utc(2026, 11, 16, 9));
    QVERIFY(!cancelled.end().isValid());
    QVERIFY(!cancelled.originalStart().isValid());
    QVERIFY(cancelled.itemId().id().isEmpty());
}

void UtEwsOccurrenceExclusions::occurrenceParserSkipsUnknownElements()
{
    static const QByteArray xml =
        "<t:CalendarItem xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
        "<t:ModifiedOccurrences>"
        "<t:Occurrence>"
        "<t:ItemId Id=\"moved-1\" ChangeKey=\"2\"/>"
        "<t:Start>2026-11-09T09:00:00Z</t:Start>"
        "<t:OriginalStart>2026-11-09T08:00:00Z</t:OriginalStart>"
        "<t:SomeUnknownField>1</t:SomeUnknownField>"
        "</t:Occurrence>"
        "<t:Occurrence>"
        "<t:ItemId Id=\"moved-2\" ChangeKey=\"3\"/>"
        "<t:Start>2026-11-23T09:00:00Z</t:Start>"
        "<t:OriginalStart>2026-11-23T08:00:00Z</t:OriginalStart>"
        "</t:Occurrence>"
        "</t:ModifiedOccurrences>"
        "</t:CalendarItem>";

    const EwsItem item = itemFromXml(xml);
    QVERIFY(item.isValid());

    const auto modified = item[EwsItemFieldModifiedOccurrences].value<EwsOccurrence::List>();
    QCOMPARE(modified.size(), 2);
    QCOMPARE(modified.at(0).originalStart().toUTC(), utc(2026, 11, 9, 8));
    QCOMPARE(modified.at(1).originalStart().toUTC(), utc(2026, 11, 23, 8));
}

void UtEwsOccurrenceExclusions::exclusionDateTimes_data()
{
    QTest::addColumn<QByteArray>("xml");
    QTest::addColumn<QList<QDateTime>>("expected");

    QTest::newRow("no occurrences") << QByteArray(
        "<t:CalendarItem xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
        "<t:ItemId Id=\"master\" ChangeKey=\"1\"/>"
        "</t:CalendarItem>") << QList<QDateTime>();

    // The occurrence is drawn on its new date from its own item - an exclusion of
    // the slot it moved away from would leave it nowhere at all.
    QTest::newRow("moved occurrence is not excluded") << QByteArray(
        "<t:CalendarItem xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
        "<t:ModifiedOccurrences><t:Occurrence>"
        "<t:ItemId Id=\"moved-1\" ChangeKey=\"2\"/>"
        "<t:Start>2026-11-09T09:00:00Z</t:Start>"
        "<t:OriginalStart>2026-11-09T08:00:00Z</t:OriginalStart>"
        "</t:Occurrence></t:ModifiedOccurrences>"
        "</t:CalendarItem>") << QList<QDateTime>();

    QTest::newRow("moved and cancelled") << QByteArray(
        "<t:CalendarItem xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
        "<t:ModifiedOccurrences><t:Occurrence>"
        "<t:ItemId Id=\"moved-1\" ChangeKey=\"2\"/>"
        "<t:Start>2026-11-09T09:00:00Z</t:Start>"
        "<t:OriginalStart>2026-11-09T08:00:00Z</t:OriginalStart>"
        "</t:Occurrence></t:ModifiedOccurrences>"
        "<t:DeletedOccurrences><t:DeletedOccurrence>"
        "<t:Start>2026-11-16T08:00:00Z</t:Start>"
        "</t:DeletedOccurrence></t:DeletedOccurrences>"
        "</t:CalendarItem>") << QList<QDateTime>{utc(2026, 11, 16, 8)};

    QTest::newRow("duplicate") << QByteArray(
        "<t:CalendarItem xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
        "<t:DeletedOccurrences><t:DeletedOccurrence>"
        "<t:Start>2026-11-09T08:00:00Z</t:Start>"
        "</t:DeletedOccurrence><t:DeletedOccurrence>"
        "<t:Start>2026-11-09T08:00:00Z</t:Start>"
        "</t:DeletedOccurrence></t:DeletedOccurrences>"
        "</t:CalendarItem>") << QList<QDateTime>{utc(2026, 11, 9, 8)};

    QTest::newRow("cancelled without start") << QByteArray(
        "<t:CalendarItem xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
        "<t:DeletedOccurrences><t:DeletedOccurrence>"
        "</t:DeletedOccurrence></t:DeletedOccurrences>"
        "</t:CalendarItem>") << QList<QDateTime>();

    QTest::newRow("all day") << QByteArray(
        "<t:CalendarItem xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
        "<t:DeletedOccurrences><t:DeletedOccurrence>"
        "<t:Start>2026-11-16T00:00:00Z</t:Start>"
        "</t:DeletedOccurrence></t:DeletedOccurrences>"
        "</t:CalendarItem>") << QList<QDateTime>{utc(2026, 11, 16)};
}

void UtEwsOccurrenceExclusions::exclusionDateTimes()
{
    QFETCH(QByteArray, xml);
    QFETCH(QList<QDateTime>, expected);

    const EwsItem item = itemFromXml(xml);
    QVERIFY(item.isValid());

    const QList<QDateTime> exclusions = ewsExclusionDateTimes(item);
    QCOMPARE(exclusions.size(), expected.size());
    for (int i = 0; i < expected.size(); ++i) {
        QCOMPARE(exclusions.at(i).toUTC(), expected.at(i));
    }
}

void UtEwsOccurrenceExclusions::movedOccurrences()
{
    // A single modified occurrence is the case the old count() > 1 check dropped.
    static const QByteArray one =
        "<t:CalendarItem xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
        "<t:ModifiedOccurrences><t:Occurrence>"
        "<t:ItemId Id=\"moved-1\" ChangeKey=\"2\"/>"
        "<t:Start>2026-11-09T09:00:00Z</t:Start>"
        "<t:OriginalStart>2026-11-09T08:00:00Z</t:OriginalStart>"
        "</t:Occurrence></t:ModifiedOccurrences>"
        "</t:CalendarItem>";

    const QList<EwsMovedOccurrence> moved = ewsMovedOccurrences(itemFromXml(one));
    QCOMPARE(moved.size(), 1);
    QCOMPARE(moved.at(0).itemId.id(), QStringLiteral("moved-1"));
    QCOMPARE(moved.at(0).originalStart.toUTC(), utc(2026, 11, 9, 8));

    static const QByteArray two =
        "<t:CalendarItem xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
        "<t:ModifiedOccurrences><t:Occurrence>"
        "<t:ItemId Id=\"moved-1\" ChangeKey=\"2\"/>"
        "<t:Start>2026-11-09T09:00:00Z</t:Start>"
        "<t:OriginalStart>2026-11-09T08:00:00Z</t:OriginalStart>"
        "</t:Occurrence><t:Occurrence>"
        "<t:ItemId Id=\"moved-2\" ChangeKey=\"3\"/>"
        "<t:Start>2026-11-23T09:00:00Z</t:Start>"
        "<t:OriginalStart>2026-11-23T08:00:00Z</t:OriginalStart>"
        "</t:Occurrence></t:ModifiedOccurrences>"
        "</t:CalendarItem>";

    const QList<EwsMovedOccurrence> both = ewsMovedOccurrences(itemFromXml(two));
    QCOMPARE(both.size(), 2);
    QCOMPARE(both.at(0).itemId.id(), QStringLiteral("moved-1"));
    QCOMPARE(both.at(0).originalStart.toUTC(), utc(2026, 11, 9, 8));
    QCOMPARE(both.at(1).itemId.id(), QStringLiteral("moved-2"));
    QCOMPARE(both.at(1).originalStart.toUTC(), utc(2026, 11, 23, 8));

    // Without an item id there is nothing to fetch.
    static const QByteArray withoutId =
        "<t:CalendarItem xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
        "<t:ModifiedOccurrences><t:Occurrence>"
        "<t:Start>2026-11-09T09:00:00Z</t:Start>"
        "</t:Occurrence></t:ModifiedOccurrences>"
        "</t:CalendarItem>";
    QVERIFY(ewsMovedOccurrences(itemFromXml(withoutId)).isEmpty());

    // A cancelled occurrence is not an item of its own.
    static const QByteArray cancelled =
        "<t:CalendarItem xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
        "<t:DeletedOccurrences><t:DeletedOccurrence>"
        "<t:Start>2026-11-16T08:00:00Z</t:Start>"
        "</t:DeletedOccurrence></t:DeletedOccurrences>"
        "</t:CalendarItem>";
    QVERIFY(ewsMovedOccurrences(itemFromXml(cancelled)).isEmpty());
}

// The calendar data of an item hold either the master of a series or nothing but one
// occurrence of it: the server delivers a moved occurrence as an item of its own.
void UtEwsOccurrenceExclusions::pickIncidence()
{
    const EwsCalendarIncidence master = ewsPickIncidence(calendarFrom(masterEvent));
    QVERIFY(master.incidence);
    QVERIFY(master.isMaster);
    QCOMPARE(master.incidence->uid(), QStringLiteral("series-1"));
    QVERIFY(!master.incidence->recurrenceId().isValid());

    // The master wins over the occurrences delivered next to it.
    const EwsCalendarIncidence series = ewsPickIncidence(calendarFrom(masterEvent + movedEvent + secondMovedEvent));
    QVERIFY(series.incidence);
    QVERIFY(series.isMaster);
    QVERIFY(!series.incidence->recurrenceId().isValid());

    // The live case: one occurrence, no master - it is kept as it is.
    const EwsCalendarIncidence occurrence = ewsPickIncidence(calendarFrom(movedEvent));
    QVERIFY(occurrence.incidence);
    QVERIFY(!occurrence.isMaster);
    QCOMPARE(occurrence.incidence->uid(), QStringLiteral("series-1"));
    QCOMPARE(occurrence.incidence->recurrenceId().toUTC(), utc(2026, 11, 9, 9));

    const EwsCalendarIncidence empty = ewsPickIncidence(calendarFrom(QByteArray()));
    QVERIFY(!empty.incidence);
    QVERIFY(!empty.isMaster);
}

// A cancelled occurrence is excluded from the master; an occurrence delivered on
// its own loses the rule of the series and keeps its RECURRENCE-ID.
void UtEwsOccurrenceExclusions::seriesExclusions()
{
    static const QByteArray xml =
        "<t:CalendarItem xmlns:t=\"http://schemas.microsoft.com/exchange/services/2006/types\">"
        "<t:DeletedOccurrences><t:DeletedOccurrence>"
        "<t:Start>2026-11-16T09:00:00Z</t:Start>"
        "</t:DeletedOccurrence></t:DeletedOccurrences>"
        "</t:CalendarItem>";
    const EwsItem item = itemFromXml(xml);
    QVERIFY(item.isValid());

    EwsCalendarIncidence master = ewsPickIncidence(calendarFrom(masterEvent));
    QVERIFY(master.incidence);
    QVERIFY(master.isMaster);
    ewsPrepareForStorage(item, master);
    QCOMPARE(master.incidence->recurrence()->exDateTimes().size(), 1);
    QVERIFY(master.incidence->recurs());

    EwsCalendarIncidence occurrence = ewsPickIncidence(calendarFrom(movedEvent));
    QVERIFY(occurrence.incidence);
    QVERIFY(!occurrence.isMaster);
    ewsPrepareForStorage(item, occurrence);
    QVERIFY(occurrence.incidence->recurrence()->exDateTimes().isEmpty());

    // The live case: the data of the occurrence carry the rule of the series, which
    // must not be stored - the occurrence stays one occurrence of its series.
    EwsCalendarIncidence recurring = ewsPickIncidence(calendarFrom(recurringMovedEvent));
    QVERIFY(recurring.incidence);
    QVERIFY(!recurring.isMaster);
    QVERIFY(recurring.incidence->recurs());
    ewsPrepareForStorage(item, recurring);
    QVERIFY(!recurring.incidence->recurs());
    QVERIFY(recurring.incidence->recurrence()->exDateTimes().isEmpty());
    QCOMPARE(recurring.incidence->recurrenceId().toUTC(), utc(2026, 11, 9, 9));
    QCOMPARE(recurring.incidence->dtStart().toUTC(), utc(2026, 11, 9, 13));
}

void UtEwsOccurrenceExclusions::exclusionsAppliedToRecurrence()
{
    const KCalendarCore::Event::Ptr event = weeklySeriesEvent();
    QVERIFY(event);
    QVERIFY(event->recurs());

    const QList<QDateTime> exclusions{utc(2026, 11, 9, 9), utc(2026, 11, 16, 9)};
    ewsApplyExclusions(*event, exclusions);

    QCOMPARE(event->recurrence()->exDateTimes().size(), 2);

    QVERIFY(!event->recurrence()->recursAt(utc(2026, 11, 9, 9)));
    QVERIFY(!event->recurrence()->recursAt(utc(2026, 11, 16, 9)));
    QVERIFY(!event->recurrence()->recursOn(QDate(2026, 11, 9), QTimeZone::UTC));
    QVERIFY(!event->recurrence()->recursOn(QDate(2026, 11, 16), QTimeZone::UTC));

    QVERIFY(event->recurrence()->recursAt(utc(2026, 11, 2, 9)));
    QVERIFY(event->recurrence()->recursAt(utc(2026, 11, 23, 9)));
    QVERIFY(event->recurrence()->recursOn(QDate(2026, 11, 2), QTimeZone::UTC));
    QVERIFY(event->recurrence()->recursOn(QDate(2026, 11, 23), QTimeZone::UTC));
}

QTEST_MAIN(UtEwsOccurrenceExclusions)

#include "ewsoccurrenceexclusions_ut.moc"
