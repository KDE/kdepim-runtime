/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    Unit tests for the Graph todoTask JSON <-> KCalendarCore::Todo mapping.
*/

#include "todo/graphtodohandler.h"

#include <KCalendarCore/Alarm>
#include <KCalendarCore/Recurrence>

#include <QJsonArray>
#include <QJsonObject>
#include <QTest>
#include <QTimeZone>

using namespace KCalendarCore;

namespace
{
QJsonObject taskDateTime(const QString &dateTime, const QString &timeZone = QStringLiteral("UTC"))
{
    QJsonObject o;
    o.insert(QStringLiteral("dateTime"), dateTime);
    o.insert(QStringLiteral("timeZone"), timeZone);
    return o;
}
} // namespace

class GraphTodoHandlerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void shouldReturnNullWithoutId()
    {
        QJsonObject json;
        json.insert(QStringLiteral("title"), QStringLiteral("No id"));
        QVERIFY(!GraphTodoHandler::toTodo(json));
    }

    void shouldMapBasicFields()
    {
        QJsonObject json;
        json.insert(QStringLiteral("id"), QStringLiteral("AAMkTask1"));
        json.insert(QStringLiteral("title"), QStringLiteral("Steuererklärung"));
        QJsonObject body;
        body.insert(QStringLiteral("contentType"), QStringLiteral("text"));
        body.insert(QStringLiteral("content"), QStringLiteral("Belege äöü sammeln"));
        json.insert(QStringLiteral("body"), body);
        // Graph uses 7 fractional digits; the parser must cope with them.
        json.insert(QStringLiteral("startDateTime"), taskDateTime(QStringLiteral("2026-07-10T08:00:00.0000000")));
        json.insert(QStringLiteral("dueDateTime"), taskDateTime(QStringLiteral("2026-07-31T22:00:00.0000000")));
        json.insert(QStringLiteral("importance"), QStringLiteral("high"));
        json.insert(QStringLiteral("status"), QStringLiteral("inProgress"));
        json.insert(QStringLiteral("categories"), QJsonArray{QStringLiteral("Privat")});

        const Todo::Ptr todo = GraphTodoHandler::toTodo(json);
        QVERIFY(todo);
        QCOMPARE(todo->uid(), QStringLiteral("AAMkTask1"));
        QCOMPARE(todo->summary(), QStringLiteral("Steuererklärung"));
        QCOMPARE(todo->description(), QStringLiteral("Belege äöü sammeln"));
        QCOMPARE(todo->dtStart(), QDateTime(QDate(2026, 7, 10), QTime(8, 0), QTimeZone::utc()));
        QCOMPARE(todo->dtDue(), QDateTime(QDate(2026, 7, 31), QTime(22, 0), QTimeZone::utc()));
        QCOMPARE(todo->priority(), 1);
        QCOMPARE(todo->status(), Incidence::StatusInProcess);
        QVERIFY(!todo->isCompleted());
        QCOMPARE(todo->categories(), QStringList{QStringLiteral("Privat")});
    }

    void shouldResolveIanaTimeZones()
    {
        QJsonObject json;
        json.insert(QStringLiteral("id"), QStringLiteral("AAMkTask2"));
        json.insert(QStringLiteral("dueDateTime"), taskDateTime(QStringLiteral("2026-07-10T12:00:00.0000000"), QStringLiteral("Europe/Berlin")));
        const Todo::Ptr todo = GraphTodoHandler::toTodo(json);
        QVERIFY(todo);
        // Instant comparison: 12:00 Berlin summer time == 10:00 UTC.
        QCOMPARE(todo->dtDue(), QDateTime(QDate(2026, 7, 10), QTime(10, 0), QTimeZone::utc()));
    }

    void shouldMapCompletion()
    {
        QJsonObject json;
        json.insert(QStringLiteral("id"), QStringLiteral("AAMkTask3"));
        json.insert(QStringLiteral("status"), QStringLiteral("completed"));
        json.insert(QStringLiteral("completedDateTime"), taskDateTime(QStringLiteral("2026-07-01T15:30:00.0000000")));
        const Todo::Ptr todo = GraphTodoHandler::toTodo(json);
        QVERIFY(todo);
        QVERIFY(todo->isCompleted());
        QCOMPARE(todo->completed(), QDateTime(QDate(2026, 7, 1), QTime(15, 30), QTimeZone::utc()));
    }

    void shouldMapReminderToAlarm()
    {
        QJsonObject json;
        json.insert(QStringLiteral("id"), QStringLiteral("AAMkTask4"));
        json.insert(QStringLiteral("title"), QStringLiteral("Anrufen"));
        json.insert(QStringLiteral("isReminderOn"), true);
        json.insert(QStringLiteral("reminderDateTime"), taskDateTime(QStringLiteral("2026-07-09T09:00:00.0000000")));
        const Todo::Ptr todo = GraphTodoHandler::toTodo(json);
        QVERIFY(todo);
        QCOMPARE(todo->alarms().size(), 1);
        QCOMPARE(todo->alarms().constFirst()->time(), QDateTime(QDate(2026, 7, 9), QTime(9, 0), QTimeZone::utc()));
    }

    void shouldMapDailyRecurrence()
    {
        QJsonObject json;
        json.insert(QStringLiteral("id"), QStringLiteral("AAMkTask5"));
        json.insert(QStringLiteral("dueDateTime"), taskDateTime(QStringLiteral("2026-07-10T08:00:00.0000000")));
        QJsonObject pattern;
        pattern.insert(QStringLiteral("type"), QStringLiteral("daily"));
        pattern.insert(QStringLiteral("interval"), 2);
        QJsonObject recurrence;
        recurrence.insert(QStringLiteral("pattern"), pattern);
        json.insert(QStringLiteral("recurrence"), recurrence);

        const Todo::Ptr todo = GraphTodoHandler::toTodo(json);
        QVERIFY(todo);
        QVERIFY(todo->recurs());
        QCOMPARE(todo->recurrence()->recurrenceType(), static_cast<ushort>(Recurrence::rDaily));
        QCOMPARE(todo->recurrence()->frequency(), 2);
    }

    void shouldWriteJson()
    {
        Todo::Ptr todo(new Todo);
        todo->setSummary(QStringLiteral("Review"));
        todo->setDescription(QStringLiteral("Kapitel 3"));
        todo->setDtDue(QDateTime(QDate(2026, 7, 31), QTime(22, 0), QTimeZone::utc()));
        todo->setPriority(9);
        todo->setCategories(QStringList{QStringLiteral("Arbeit")});
        Alarm::Ptr alarm = todo->newAlarm();
        alarm->setDisplayAlarm(todo->summary());
        alarm->setTime(QDateTime(QDate(2026, 7, 30), QTime(9, 0), QTimeZone::utc()));
        alarm->setEnabled(true);

        const QJsonObject json = GraphTodoHandler::toJson(todo);
        QCOMPARE(json.value(QLatin1String("title")).toString(), QStringLiteral("Review"));
        QCOMPARE(json.value(QLatin1String("body")).toObject().value(QLatin1String("content")).toString(), QStringLiteral("Kapitel 3"));
        QCOMPARE(json.value(QLatin1String("dueDateTime")).toObject().value(QLatin1String("dateTime")).toString(),
                 QStringLiteral("2026-07-31T22:00:00.0000000"));
        QCOMPARE(json.value(QLatin1String("importance")).toString(), QStringLiteral("low"));
        QCOMPARE(json.value(QLatin1String("status")).toString(), QStringLiteral("notStarted"));
        QCOMPARE(json.value(QLatin1String("isReminderOn")).toBool(), true);
        QCOMPARE(json.value(QLatin1String("reminderDateTime")).toObject().value(QLatin1String("dateTime")).toString(),
                 QStringLiteral("2026-07-30T09:00:00.0000000"));
        QCOMPARE(json.value(QLatin1String("categories")).toArray(), QJsonArray{QStringLiteral("Arbeit")});
    }

    void shouldWriteCompletedStatus()
    {
        Todo::Ptr todo(new Todo);
        todo->setSummary(QStringLiteral("Done"));
        todo->setCompleted(QDateTime(QDate(2026, 7, 1), QTime(12, 0), QTimeZone::utc()));
        const QJsonObject json = GraphTodoHandler::toJson(todo);
        QCOMPARE(json.value(QLatin1String("status")).toString(), QStringLiteral("completed"));
    }

    void shouldWriteDailyRecurrenceWithEndDate()
    {
        Todo::Ptr todo(new Todo);
        todo->setSummary(QStringLiteral("Gießen"));
        todo->setDtStart(QDateTime(QDate(2026, 7, 10), QTime(8, 0), QTimeZone::utc()));
        todo->recurrence()->setDaily(1);
        todo->recurrence()->setEndDate(QDate(2026, 8, 31));

        const QJsonObject recurrence = GraphTodoHandler::toJson(todo).value(QLatin1String("recurrence")).toObject();
        QVERIFY(!recurrence.isEmpty());
        QCOMPARE(recurrence.value(QLatin1String("pattern")).toObject().value(QLatin1String("type")).toString(), QStringLiteral("daily"));
        const QJsonObject range = recurrence.value(QLatin1String("range")).toObject();
        QCOMPARE(range.value(QLatin1String("type")).toString(), QStringLiteral("endDate"));
        QCOMPARE(range.value(QLatin1String("endDate")).toString(), QStringLiteral("2026-08-31"));
        QCOMPARE(range.value(QLatin1String("startDate")).toString(), QStringLiteral("2026-07-10"));
    }

    void shouldRoundTripThroughJson()
    {
        QJsonObject json;
        json.insert(QStringLiteral("id"), QStringLiteral("AAMkTask6"));
        json.insert(QStringLiteral("title"), QStringLiteral("Round trip"));
        QJsonObject body;
        body.insert(QStringLiteral("contentType"), QStringLiteral("text"));
        body.insert(QStringLiteral("content"), QStringLiteral("Body"));
        json.insert(QStringLiteral("body"), body);
        json.insert(QStringLiteral("dueDateTime"), taskDateTime(QStringLiteral("2026-07-31T22:00:00.0000000")));

        const QJsonObject back = GraphTodoHandler::toJson(GraphTodoHandler::toTodo(json));
        QCOMPARE(back.value(QLatin1String("title")), json.value(QLatin1String("title")));
        QCOMPARE(back.value(QLatin1String("body")), json.value(QLatin1String("body")));
        QCOMPARE(back.value(QLatin1String("dueDateTime")), json.value(QLatin1String("dueDateTime")));
    }
};

QTEST_GUILESS_MAIN(GraphTodoHandlerTest)

#include "graphtodohandlertest.moc"
