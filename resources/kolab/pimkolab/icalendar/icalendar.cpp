/*

    Copyright (C) 2012  Christian Mollekopf <chrigi_1@fastmail.fm>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "icalendar.h"
#include "imip.h"
#include "pimkolab_debug.h"
#include "libkolab-version.h"
#include <conversion/kcalconversion.h>
#include <conversion/commonconversion.h>
#include <mime/mimeutils.h>
#include <kcalcore/event.h>
#include <kcalcore/memorycalendar.h>
#include <kcalcore/icalformat.h>
#include <kmime/kmime_message.h>
#include <klocalizedstring.h>
#include <iostream>
#include <QDebug>
#include <QTimeZone>

namespace Kolab {
std::string toICal(const std::vector<Event> &events)
{
    KCalCore::Calendar::Ptr calendar(new KCalCore::MemoryCalendar(Kolab::Conversion::getTimeSpec(true, std::string())));
    for (const Event &event : events) {
        KCalCore::Event::Ptr kcalEvent = Conversion::toKCalCore(event);
        kcalEvent->setCreated(QDateTime::currentDateTimeUtc()); //sets dtstamp
        calendar->addEvent(kcalEvent);
    }
    KCalCore::ICalFormat format;
    format.setApplication(QStringLiteral("libkolab"), QStringLiteral(LIBKOLAB_LIB_VERSION_STRING));
//     qCDebug(PIMKOLAB_LOG) << format.createScheduleMessage(calendar->events().first(), KCalCore::iTIPRequest);

    return Conversion::toStdString(format.toString(calendar));
}

std::vector< Event > fromICalEvents(const std::string &input)
{
    KCalCore::Calendar::Ptr calendar(new KCalCore::MemoryCalendar(Kolab::Conversion::getTimeSpec(true, std::string())));
    KCalCore::ICalFormat format;
    format.setApplication(QStringLiteral("libkolab"), QStringLiteral(LIBKOLAB_LIB_VERSION_STRING));
    format.fromString(calendar, Conversion::fromStdString(input));
    std::vector<Event> events;
    foreach (const KCalCore::Event::Ptr &event, calendar->events()) {
        events.push_back(Conversion::fromKCalCore(*event));
    }
    return events;
}

ITipHandler::ITipHandler()
    :   mMethod(iTIPNoMethod)
{
}

ITipHandler::ITipMethod mapFromKCalCore(KCalCore::iTIPMethod method)
{
    Q_ASSERT((int)KCalCore::iTIPPublish == (int)ITipHandler::iTIPPublish);
    Q_ASSERT((int)KCalCore::iTIPNoMethod == (int)ITipHandler::iTIPNoMethod);
    return static_cast<ITipHandler::ITipMethod>(method);
}

KCalCore::iTIPMethod mapToKCalCore(ITipHandler::ITipMethod method)
{
    Q_ASSERT((int)KCalCore::iTIPPublish == (int)ITipHandler::iTIPPublish);
    Q_ASSERT((int)KCalCore::iTIPNoMethod == (int)ITipHandler::iTIPNoMethod);
    return static_cast<KCalCore::iTIPMethod>(method);
}

std::string ITipHandler::toITip(const Event &event, ITipHandler::ITipMethod method) const
{
    KCalCore::ICalFormat format;
    format.setApplication(QStringLiteral("libkolab"), QStringLiteral(LIBKOLAB_LIB_VERSION_STRING));
    KCalCore::iTIPMethod m = mapToKCalCore(method);
    if (m == KCalCore::iTIPNoMethod) {
        return std::string();
    }
//     qCDebug(PIMKOLAB_LOG) << event.start().
/* TODO
 * DTSTAMP is created
 * CREATED is current timestamp
 * LASTMODIFIED is lastModified
 *
 * Double check if that is correct.
 *
 * I think DTSTAMP should be the current timestamp, and CREATED should be the creation date.
 */
    KCalCore::Event::Ptr e = Conversion::toKCalCore(event);
    return Conversion::toStdString(format.createScheduleMessage(e, m));
}

std::vector< Event > ITipHandler::fromITip(const std::string &string)
{
    KCalCore::Calendar::Ptr calendar(new KCalCore::MemoryCalendar(QTimeZone::utc()));
    KCalCore::ICalFormat format;
    KCalCore::ScheduleMessage::Ptr msg = format.parseScheduleMessage(calendar, Conversion::fromStdString(string));
    KCalCore::Event::Ptr event = msg->event().dynamicCast<KCalCore::Event>();
    std::vector< Event > events;
    events.push_back(Conversion::fromKCalCore(*event));
    mMethod = mapFromKCalCore(msg->method());
    return events;
}

ITipHandler::ITipMethod ITipHandler::method() const
{
    return mMethod;
}

std::string ITipHandler::toIMip(const Event &event, ITipHandler::ITipMethod m, const std::string &from, bool bccMe) const
{
    KCalCore::Event::Ptr e = Conversion::toKCalCore(event);
//     e->recurrence()->addRDateTime(e->dtStart()); //FIXME The createScheduleMessage converts everything to utc without a recurrence.
    KCalCore::ICalFormat format;
    format.setApplication(QStringLiteral("libkolab"), QStringLiteral(LIBKOLAB_LIB_VERSION_STRING));
    KCalCore::iTIPMethod method = mapToKCalCore(m);
    const QString &messageText = format.createScheduleMessage(e, method);
    //This code is mostly from MailScheduler::performTransaction
    if (method == KCalCore::iTIPRequest
        || method == KCalCore::iTIPCancel
        || method == KCalCore::iTIPAdd
        || method == KCalCore::iTIPDeclineCounter) {
        return Conversion::toStdString(QString::fromUtf8(mailAttendees(e, bccMe, messageText)));
    } else {
        QString subject;
        if (e && method == KCalCore::iTIPCounter) {
            subject = i18n("Counter proposal: %1", e->summary());
        }
        return Conversion::toStdString(QString::fromUtf8(mailOrganizer(e, Conversion::fromStdString(from), bccMe, messageText, subject)));
    }
}

std::vector< Event > ITipHandler::fromIMip(const std::string &input)
{
    KMime::Message::Ptr msg = KMime::Message::Ptr(new KMime::Message);
    msg->setContent(Conversion::fromStdString(input).toUtf8());
    msg->parse();
    msg->content(KMime::ContentIndex());

    KMime::Content *c = Kolab::Mime::findContentByType(msg, "text/calendar");
    if (!c) {
        qCWarning(PIMKOLAB_LOG) << "could not find text/calendar part";
        return std::vector< Event >();
    }
    return fromITip(Conversion::toStdString(QString::fromUtf8(c->decodedContent())));
}
}
