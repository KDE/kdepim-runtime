/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "graphtodohandler.h"

#include "calendar/grapheventhandler.h"

#include <KCalendarCore/Alarm>

#include <QJsonArray>
#include <QTimeZone>

using namespace KCalendarCore;

namespace
{
// Graph dateTimeTimeZone -> QDateTime. Unlike the calendar API, To Do ignores the
// Prefer: outlook.timezone header: the timestamp is naive and the zone is a separate
// field. Graph also uses 7 fractional digits, which QDateTime cannot parse.
QDateTime parseTaskDateTime(const QJsonObject &dtz)
{
    QString raw = dtz.value(QLatin1String("dateTime")).toString();
    if (raw.isEmpty()) {
        return {};
    }
    const int dot = raw.indexOf(QLatin1Char('.'));
    if (dot >= 0) {
        raw = raw.left(dot);
    }
    QDateTime dt = QDateTime::fromString(raw, QStringLiteral("yyyy-MM-ddTHH:mm:ss"));
    if (!dt.isValid()) {
        return {};
    }
    const QString tz = dtz.value(QLatin1String("timeZone")).toString();
    const QTimeZone zone = tz.isEmpty() ? QTimeZone::utc() : QTimeZone(tz.toUtf8());
    dt.setTimeZone(zone.isValid() ? zone : QTimeZone::utc());
    return dt;
}

QJsonObject utcTaskDateTime(const QDateTime &dt)
{
    QJsonObject o;
    o.insert(QStringLiteral("dateTime"), dt.toUTC().toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss.0000000")));
    o.insert(QStringLiteral("timeZone"), QStringLiteral("UTC"));
    return o;
}
} // namespace

namespace GraphTodoHandler
{
QString mimeType()
{
    return QStringLiteral("application/x-vnd.akonadi.calendar.todo");
}

KCalendarCore::Todo::Ptr toTodo(const QJsonObject &json)
{
    const QString id = json.value(QLatin1String("id")).toString();
    if (id.isEmpty()) {
        return {};
    }
    auto todo = Todo::Ptr(new Todo);
    todo->setUid(id);
    todo->setSummary(json.value(QLatin1String("title")).toString());

    const QJsonObject body = json.value(QLatin1String("body")).toObject();
    if (body.value(QLatin1String("contentType")).toString() == QLatin1String("text")) {
        todo->setDescription(body.value(QLatin1String("content")).toString());
    }

    const QDateTime start = parseTaskDateTime(json.value(QLatin1String("startDateTime")).toObject());
    if (start.isValid()) {
        todo->setDtStart(start);
    }
    const QDateTime due = parseTaskDateTime(json.value(QLatin1String("dueDateTime")).toObject());
    if (due.isValid()) {
        todo->setDtDue(due);
    }

    const QString status = json.value(QLatin1String("status")).toString();
    if (status == QLatin1String("completed")) {
        const QDateTime completed = parseTaskDateTime(json.value(QLatin1String("completedDateTime")).toObject());
        if (completed.isValid()) {
            todo->setCompleted(completed);
        } else {
            todo->setCompleted(true);
        }
    } else if (status == QLatin1String("inProgress")) {
        todo->setStatus(Incidence::StatusInProcess);
    }

    const QString importance = json.value(QLatin1String("importance")).toString();
    if (importance == QLatin1String("high")) {
        todo->setPriority(1);
    } else if (importance == QLatin1String("low")) {
        todo->setPriority(9);
    }

    if (json.value(QLatin1String("isReminderOn")).toBool()) {
        const QDateTime reminder = parseTaskDateTime(json.value(QLatin1String("reminderDateTime")).toObject());
        if (reminder.isValid()) {
            Alarm::Ptr alarm = todo->newAlarm();
            alarm->setDisplayAlarm(todo->summary());
            alarm->setTime(reminder);
            alarm->setEnabled(true);
        }
    }

    QStringList categories;
    const QJsonArray cats = json.value(QLatin1String("categories")).toArray();
    categories.reserve(cats.count());
    for (const auto &c : cats) {
        categories << c.toString();
    }
    todo->setCategories(categories);

    const QJsonObject recurrence = json.value(QLatin1String("recurrence")).toObject();
    if (!recurrence.isEmpty()) {
        GraphEventHandler::applyRecurrence(recurrence, todo);
    }
    return todo;
}

QJsonObject toJson(const KCalendarCore::Todo::Ptr &todo)
{
    QJsonObject json;
    json.insert(QStringLiteral("title"), todo->summary());

    // Only plain-text bodies are mapped (mirroring toTodo). Omit the body when there
    // is no local description so a server-side HTML body is not clobbered by an
    // empty text one.
    if (!todo->description().isEmpty()) {
        QJsonObject body;
        body.insert(QStringLiteral("contentType"), QStringLiteral("text"));
        body.insert(QStringLiteral("content"), todo->description());
        json.insert(QStringLiteral("body"), body);
    }

    if (todo->dtStart().isValid()) {
        json.insert(QStringLiteral("startDateTime"), utcTaskDateTime(todo->dtStart()));
    }
    if (todo->dtDue().isValid()) {
        json.insert(QStringLiteral("dueDateTime"), utcTaskDateTime(todo->dtDue()));
    }

    if (todo->isCompleted()) {
        json.insert(QStringLiteral("status"), QStringLiteral("completed"));
    } else if (todo->status() == Incidence::StatusInProcess || todo->percentComplete() > 0) {
        json.insert(QStringLiteral("status"), QStringLiteral("inProgress"));
    } else {
        json.insert(QStringLiteral("status"), QStringLiteral("notStarted"));
    }

    const int priority = todo->priority();
    if (priority >= 1 && priority <= 3) {
        json.insert(QStringLiteral("importance"), QStringLiteral("high"));
    } else if (priority >= 7) {
        json.insert(QStringLiteral("importance"), QStringLiteral("low"));
    } else {
        json.insert(QStringLiteral("importance"), QStringLiteral("normal"));
    }

    QDateTime reminder;
    const auto alarms = todo->alarms();
    for (const Alarm::Ptr &alarm : alarms) {
        if (alarm->enabled() && alarm->hasTime()) {
            reminder = alarm->time();
            break;
        }
    }
    if (reminder.isValid()) {
        json.insert(QStringLiteral("isReminderOn"), true);
        json.insert(QStringLiteral("reminderDateTime"), utcTaskDateTime(reminder));
    } else {
        json.insert(QStringLiteral("isReminderOn"), false);
    }

    if (!todo->categories().isEmpty()) {
        json.insert(QStringLiteral("categories"), QJsonArray::fromStringList(todo->categories()));
    }

    const QJsonObject recurrence = GraphEventHandler::recurrenceToJson(todo);
    if (!recurrence.isEmpty()) {
        json.insert(QStringLiteral("recurrence"), recurrence);
    }
    return json;
}
}
