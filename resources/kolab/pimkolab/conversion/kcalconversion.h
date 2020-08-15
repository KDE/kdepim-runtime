/*
 * SPDX-FileCopyrightText: 2011 Christian Mollekopf <mollekopf@kolabsys.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
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
