/*
    This file is part of libkdepim.
    Copyright (c) 1998 Preston Brown
    Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>
    Copyright (c) 2002 Jan-Pascal van Best <janpascal@vanbest.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include <kconfig.h>

#include <libkcal/calendar.h>

#include "resourcecalendar.h"

using namespace KCal;

ResourceCalendar::ResourceCalendar( const KConfig *config )
    : KRES::Resource( config )
{
}

ResourceCalendar::~ResourceCalendar()
{
}

void ResourceCalendar::writeConfig( KConfig* config )
{
  KRES::Resource::writeConfig( config );
}

#include "resourcecalendar.moc"
