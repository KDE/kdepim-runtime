/*
 * Copyright (C) 2011  Christian Mollekopf <mollekopf@kolabsys.com>
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

#ifndef KOLABKCALCONVERSION_H
#define KOLABKCALCONVERSION_H

#include "kolab_export.h"

#include <kolabevent.h>
#include <kolabtodo.h>
#include <kolabjournal.h>
#include <kcalendarcore/event.h>
#include <kcalendarcore/todo.h>
#include <kcalendarcore/journal.h>

namespace Kolab {
/**
 * Conversion of Kolab-Containers to/from KCalendarCore Containers.
 *
 */
namespace Conversion {
KOLAB_EXPORT KCalendarCore::Event::Ptr toKCalendarCore(const Kolab::Event &);
KOLAB_EXPORT Kolab::Event fromKCalendarCore(const KCalendarCore::Event &);

KOLAB_EXPORT KCalendarCore::Todo::Ptr toKCalendarCore(const Kolab::Todo &);
KOLAB_EXPORT Kolab::Todo fromKCalendarCore(const KCalendarCore::Todo &);

KOLAB_EXPORT KCalendarCore::Journal::Ptr toKCalendarCore(const Kolab::Journal &);
KOLAB_EXPORT Kolab::Journal fromKCalendarCore(const KCalendarCore::Journal &);
}
}

#endif
