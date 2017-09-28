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

#ifndef KOLABCALENDARING_H
#define KOLABCALENDARING_H

#ifndef SWIG
#include "kolab_export.h"
#else
/* No export/import SWIG interface files */
#define KOLAB_EXPORT
#endif

#include <kcalcore/event.h>
#include <kcalcore/memorycalendar.h>
#include <boost/scoped_ptr.hpp>
#include <kolabevent.h>

namespace Kolab {
    namespace Calendaring {
/**
 * Returns true if the events conflict (overlap)
 * Start and end date/time is inclusive.
 *
 * Does not take recurrences into account.
 */
KOLAB_EXPORT bool conflicts(const Kolab::Event &, const Kolab::Event &);

/**
 * Returns sets of the events which are directly conflicting with each other.
 * The same event may appear in multiple sets.
 * Non-conflicting events are not returned.
 * conflicts() is used for conflict detection.
 *
 * If the second list is given, each event from the first list is additionally checked against each event of the second set.
 * Conflicts within the second list are not detected.
 *
 * The checked event from the first list comes always first in the returned set.
 */
KOLAB_EXPORT std::vector< std::vector<Kolab::Event> > getConflictingSets(const std::vector<Kolab::Event> &, const std::vector<Kolab::Event> & = std::vector<Kolab::Event>());

/**
 * Returns the dates in which the event recurs within the specified timespan.
 */
KOLAB_EXPORT std::vector<Kolab::cDateTime> timeInInterval(const Kolab::Event &, const Kolab::cDateTime &start, const Kolab::cDateTime &end);

/**
 * In-Memory Calendar Cache
 */
class KOLAB_EXPORT Calendar {
public:
    explicit Calendar();
    /**
     * Add an event to the in-memory calendar.
     */
    void addEvent(const Kolab::Event &);
    /**
     * Returns all events within the specified interval (start and end inclusive).
     *
     * @param sort controls if the resulting event set is sorted in ascending order according to the start date
     */
    std::vector<Kolab::Event> getEvents(const Kolab::cDateTime &start, const Kolab::cDateTime &end, bool sort);
private:
    Calendar(const Calendar &);
    void operator=(const Calendar &);
    boost::scoped_ptr<KCalCore::MemoryCalendar> mCalendar;
};

    }; //Namespace
}; //Namespace

#endif
