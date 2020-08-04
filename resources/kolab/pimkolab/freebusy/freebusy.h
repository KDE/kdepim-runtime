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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FREEBUSY_H
#define FREEBUSY_H

#include "kolab_export.h"
#include <kolabevent.h>
#include <kolabfreebusy.h>
#include <kcalendarcore/event.h>

namespace Kolab {
namespace FreebusyUtils {
KOLAB_EXPORT Freebusy generateFreeBusy(const QVector<KCalendarCore::Event::Ptr> &events, const QDateTime &startDate, const QDateTime &endDate, const KCalendarCore::Person &organizer, bool allDay);
KOLAB_EXPORT std::string toIFB(const Kolab::Freebusy &);

Kolab::Freebusy generateFreeBusy(const std::vector<Kolab::Event> &events, const Kolab::cDateTime &startDate, const Kolab::cDateTime &endDate);
KOLAB_EXPORT Kolab::Freebusy aggregateFreeBusy(const std::vector<Kolab::Freebusy> &fbs, const std::string &organizerEmail, const std::string &organizerName, bool simple = true);
}
}

#endif // FREEBUSY_H
