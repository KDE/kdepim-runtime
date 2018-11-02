/*
 * Copyright (C) 2012  Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "calendaring.h"
#include "pimkolab_debug.h"

#include <kcalcore/event.h>
#include <kcalcore/todo.h>
#include <qdebug.h>
#include <kolabevent.h>
#include <QDate>
#include <QTimeZone>

#include "conversion/kcalconversion.h"
#include "conversion/commonconversion.h"

namespace Kolab {
namespace Calendaring {
bool conflicts(const Kolab::Event &e1, const Kolab::Event &e2)
{
    KCalCore::Event::Ptr k1 = Kolab::Conversion::toKCalCore(e1);
    KCalCore::Event::Ptr k2 = Kolab::Conversion::toKCalCore(e2);
    if (k2->dtEnd() < k1->dtStart()) {
        return false;
    } else if (k1->dtEnd() < k2->dtStart()) {
        return false;
    }
    return true;
}

std::vector< std::vector< Event > > getConflictingSets(const std::vector< Event > &events, const std::vector< Event > &events2)
{
    std::vector< std::vector< Kolab::Event > > ret;
    for (std::size_t i = 0; i < events.size(); i++) {
        std::vector<Kolab::Event> set;
        const Kolab::Event &event = events.at(i);
        set.push_back(event);
        for (std::size_t q = i+1; q < events.size(); q++) {
            const Kolab::Event &e2 = events.at(q);
            if (conflicts(event, e2)) {
                set.push_back(e2);
            }
        }
        for (std::size_t m = 0; m < events2.size(); m++) {
            const Kolab::Event &e2 = events2.at(m);
            if (conflicts(event, e2)) {
                set.push_back(e2);
            }
        }
        if (set.size() > 1) {
            ret.push_back(set);
        }
    }
    return ret;
}

std::vector<Kolab::cDateTime> timeInInterval(const Kolab::Event &e, const Kolab::cDateTime &start, const Kolab::cDateTime &end)
{
    KCalCore::Event::Ptr k = Kolab::Conversion::toKCalCore(e);
    const KCalCore::DateTimeList list = k->recurrence()->timesInInterval(Kolab::Conversion::toDate(start), Kolab::Conversion::toDate(end));
    std::vector<Kolab::cDateTime> dtList;
    dtList.reserve(list.count());
    for (const QDateTime &dt : list) {
        dtList.push_back(Kolab::Conversion::fromDate(dt, start.isDateOnly()));
    }
    return dtList;
}

Calendar::Calendar()
    :   mCalendar(new KCalCore::MemoryCalendar(Kolab::Conversion::getTimeSpec(true, std::string())))//Always utc as it doesn't change anything anyways
{
}

void Calendar::addEvent(const Kolab::Event &event)
{
    KCalCore::Event::Ptr k = Kolab::Conversion::toKCalCore(event);
    if (!mCalendar->addEvent(k)) {
        qCWarning(PIMKOLAB_LOG) << "failed to add event";
    }
}

std::vector<Kolab::Event> Calendar::getEvents(const Kolab::cDateTime &start, const Kolab::cDateTime &end, bool sort)
{
    const QDateTime s = Kolab::Conversion::toDate(start);
    const QDateTime e = Kolab::Conversion::toDate(end);
    const QTimeZone tz = s.timeZone();
    KCalCore::Event::List list = mCalendar->events(s.date(), e.date(), tz, true);
    if (sort) {
        list = mCalendar->sortEvents(list, KCalCore::EventSortStartDate, KCalCore::SortDirectionAscending);
    }
    std::vector<Kolab::Event> eventlist;
    for (const KCalCore::Event::Ptr &event : qAsConst(list)) {
        //We have to filter the list by time
        if (event->dtEnd() >= s && e >= event->dtStart()) {
            eventlist.push_back(Kolab::Conversion::fromKCalCore(*event));
        }
    }
    return eventlist;
}
}     //Namespace
} //Namespace
