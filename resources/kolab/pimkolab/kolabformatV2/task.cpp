/*
    This file is part of the kolab resource - the implementation of the
    Kolab storage format. See www.kolab.org for documentation on this.

    Copyright (c) 2004 Bo Thorsen <bo@sonofthor.dk>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include "task.h"
#include "utils/porting.h"
#include "pimkolab_debug.h"

#include <kcalcore/todo.h>

using namespace KolabV2;

KCalCore::Todo::Ptr Task::fromXml(const QDomDocument &xmlDoc, const QString &tz)
{
    Task task(tz);
    task.loadXML(xmlDoc);
    KCalCore::Todo::Ptr todo(new KCalCore::Todo());
    task.saveTo(todo);
    return todo;
}

QString Task::taskToXML(const KCalCore::Todo::Ptr &todo, const QString &tz)
{
    Task task(tz, todo);
    return task.saveXML();
}

Task::Task(const QString &tz, const KCalCore::Todo::Ptr &task)
    : Incidence(tz, task)
    , mPercentCompleted(0)
    , mStatus(KCalCore::Incidence::StatusNone)
    , mHasStartDate(false)
    , mHasDueDate(false)
    , mHasCompletedDate(false)
{
    if (task) {
        setFields(task);
    }
}

Task::~Task()
{
}

void Task::setPercentCompleted(int percent)
{
    mPercentCompleted = percent;
}

int Task::percentCompleted() const
{
    return mPercentCompleted;
}

void Task::setStatus(KCalCore::Incidence::Status status)
{
    mStatus = status;
}

KCalCore::Incidence::Status Task::status() const
{
    return mStatus;
}

void Task::setParent(const QString &parentUid)
{
    mParent = parentUid;
}

QString Task::parent() const
{
    return mParent;
}

void Task::setDueDate(const KDateTime &date)
{
    mDueDate = date;
    mHasDueDate = true;
}

void Task::setDueDate(const QDate &date)
{
    mDueDate = KDateTime(date);
    mHasDueDate = true;
    mFloatingStatus = AllDay;
}

void Task::setDueDate(const QString &date)
{
    if (date.length() > 10) {
        // This is a date + time
        setDueDate(stringToKDateTime(date));
    } else {
        // This is only a date
        setDueDate(stringToDate(date));
    }
}

KDateTime Task::dueDate() const
{
    return mDueDate;
}

void Task::setHasStartDate(bool v)
{
    mHasStartDate = v;
}

bool Task::hasStartDate() const
{
    return mHasStartDate;
}

bool Task::hasDueDate() const
{
    return mHasDueDate;
}

void Task::setCompletedDate(const KDateTime &date)
{
    mCompletedDate = date;
    mHasCompletedDate = true;
}

KDateTime Task::completedDate() const
{
    return mCompletedDate;
}

bool Task::hasCompletedDate() const
{
    return mHasCompletedDate;
}

bool Task::loadAttribute(QDomElement &element)
{
    QString tagName = element.tagName();

    if (tagName == QLatin1String("completed")) {
        bool ok;
        int percent = element.text().toInt(&ok);
        if (!ok || percent < 0 || percent > 100) {
            percent = 0;
        }
        setPercentCompleted(percent);
    } else if (tagName == QLatin1String("status")) {
        if (element.text() == QLatin1String("in-progress")) {
            setStatus(KCalCore::Incidence::StatusInProcess);
        } else if (element.text() == QLatin1String("completed")) {
            setStatus(KCalCore::Incidence::StatusCompleted);
        } else if (element.text() == QLatin1String("waiting-on-someone-else")) {
            setStatus(KCalCore::Incidence::StatusNeedsAction);
        } else if (element.text() == QLatin1String("deferred")) {
            // Guessing a status here
            setStatus(KCalCore::Incidence::StatusCanceled);
        } else {
            // Default
            setStatus(KCalCore::Incidence::StatusNone);
        }
    } else if (tagName == QLatin1String("due-date")) {
        setDueDate(element.text());
    } else if (tagName == QLatin1String("parent")) {
        setParent(element.text());
    } else if (tagName == QLatin1String("x-completed-date")) {
        setCompletedDate(stringToKDateTime(element.text()));
    } else if (tagName == QLatin1String("start-date")) {
        setHasStartDate(true);
        setStartDate(element.text());
    } else {
        return Incidence::loadAttribute(element);
    }

    // We handled this
    return true;
}

bool Task::saveAttributes(QDomElement &element) const
{
    // Save the base class elements
    Incidence::saveAttributes(element);

    writeString(element, QStringLiteral("completed"), QString::number(percentCompleted()));

    switch (status()) {
    case KCalCore::Incidence::StatusInProcess:
        writeString(element, QStringLiteral("status"), QStringLiteral("in-progress"));
        break;
    case KCalCore::Incidence::StatusCompleted:
        writeString(element, QStringLiteral("status"), QStringLiteral("completed"));
        break;
    case KCalCore::Incidence::StatusNeedsAction:
        writeString(element, QStringLiteral("status"), QStringLiteral("waiting-on-someone-else"));
        break;
    case KCalCore::Incidence::StatusCanceled:
        writeString(element, QStringLiteral("status"), QStringLiteral("deferred"));
        break;
    case KCalCore::Incidence::StatusNone:
        writeString(element, QStringLiteral("status"), QStringLiteral("not-started"));
        break;
    case KCalCore::Incidence::StatusTentative:
    case KCalCore::Incidence::StatusConfirmed:
    case KCalCore::Incidence::StatusDraft:
    case KCalCore::Incidence::StatusFinal:
    case KCalCore::Incidence::StatusX:
        // All of these are saved as StatusNone.
        writeString(element, QStringLiteral("status"), QStringLiteral("not-started"));
        break;
    }

    if (hasDueDate()) {
        if (mFloatingStatus == HasTime) {
            writeString(element, QStringLiteral("due-date"), dateTimeToString(dueDate()));
        } else {
            writeString(element, QStringLiteral("due-date"), dateToString(dueDate().date()));
        }
    }

    if (!parent().isNull()) {
        writeString(element, QStringLiteral("parent"), parent());
    }

    if (hasCompletedDate() && percentCompleted() == 100) {
        writeString(element, QStringLiteral("x-completed-date"), dateTimeToString(completedDate()));
    }

    return true;
}

bool Task::loadXML(const QDomDocument &document)
{
    QDomElement top = document.documentElement();

    if (top.tagName() != QLatin1String("task")) {
        qCWarning(PIMKOLAB_LOG) << QStringLiteral("XML error: Top tag was %1 instead of the expected task").arg(top.tagName());
        return false;
    }
    setHasStartDate(false); // todo's don't necessarily have one

    for (QDomNode n = top.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (n.isComment()) {
            continue;
        }
        if (n.isElement()) {
            QDomElement e = n.toElement();
            if (!loadAttribute(e)) {
                // TODO: Unhandled tag - save for later storage
                qCDebug(PIMKOLAB_LOG) <<"Warning: Unhandled tag" << e.tagName();
            }
        } else {
            qCDebug(PIMKOLAB_LOG) <<"Node is not a comment or an element???";
        }
    }

    return true;
}

QString Task::saveXML() const
{
    QDomDocument document = domTree();
    QDomElement element = document.createElement(QStringLiteral("task"));
    element.setAttribute(QStringLiteral("version"), QStringLiteral("1.0"));
    saveAttributes(element);
    if (!hasStartDate() && startDate().isValid()) {
        // events and journals always have a start date, but tasks don't.
        // Remove the entry done by the inherited save above, because we
        // don't have one.
        QDomNodeList l = element.elementsByTagName(QStringLiteral("start-date"));
        Q_ASSERT(l.count() == 1);
        element.removeChild(l.item(0));
    }
    document.appendChild(element);
    return document.toString();
}

void Task::setFields(const KCalCore::Todo::Ptr &task)
{
    Incidence::setFields(task);

    setPercentCompleted(task->percentComplete());
    setStatus(task->status());
    setHasStartDate(task->hasStartDate());

    if (task->hasDueDate()) {
        if (task->allDay()) {
            // This is a floating task. Don't timezone move this one
            mFloatingStatus = AllDay;
            setDueDate(KDateTime(task->dtDue().date()));
        } else {
            mFloatingStatus = HasTime;
            setDueDate(Porting::q2k(localToUTC(task->dtDue())));
        }
    } else {
        mHasDueDate = false;
    }

    if (!task->relatedTo().isEmpty()) {
        setParent(task->relatedTo());
    } else {
        setParent(QString());
    }

    if (task->hasCompletedDate() && task->percentComplete() == 100) {
        setCompletedDate(Porting::q2k(localToUTC(task->completed())));
    } else {
        mHasCompletedDate = false;
    }
}

void Task::saveTo(const KCalCore::Todo::Ptr &task)
{
    Incidence::saveTo(task);

    task->setPercentComplete(percentCompleted());
    task->setStatus(status());
    //PORT KF5 task->setHasStartDate( hasStartDate() );
    //PORT KF5 task->setHasDueDate( hasDueDate() );
    if (hasDueDate()) {
        task->setDtDue(utcToLocal(Porting::k2q(dueDate())));
    }

    if (!parent().isEmpty()) {
        task->setRelatedTo(parent());
    }

    if (hasCompletedDate() && task->percentComplete() == 100) {
        task->setCompleted(utcToLocal(Porting::k2q(mCompletedDate)));
    }
}
