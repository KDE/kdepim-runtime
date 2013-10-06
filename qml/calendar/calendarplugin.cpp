/*
    Copyright (c) 2013 Mark Gaiser <markg85@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "calendarplugin.h"

#include "calendardata.h"
#include "calendar.h"
#include "incidencemodifier.h"
#include <qdeclarative.h>
#include <QAbstractItemModel>
#include <QAbstractListModel>


void CalendarPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.pim.calendar"));
    qmlRegisterType<CalendarData>(uri, 1, 0, "CalendarData");
    qmlRegisterType<Calendar>(uri, 1, 0, "Calendar");
    qmlRegisterType<IncidenceModifier>(uri, 1, 0, "IncidenceModifier");
    qmlRegisterType<QAbstractItemModel>();
    qmlRegisterType<QAbstractListModel>();
}

// If i add this line in calendarplugin.h it gives me compile errors.. I don't know why that happens, but this seems to work around it.
Q_EXPORT_PLUGIN2(calendarplugin, CalendarPlugin)
