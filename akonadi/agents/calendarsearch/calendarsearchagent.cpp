/*
    Copyright 2009 KDAB; Author: Frank Osterfeld <osterfeld@kde.org>

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

#include "calendarsearchagent.h"

using namespace Akonadi;

class CalendarSearchAgent::Private
{
    CalendarSearchAgent* const q;
public:
    explicit Private( CalendarSearchAgent* qq ) : q( qq )
    {
    }

    ~Private()
    {
    }
};



CalendarSearchAgent::CalendarSearchAgent( const QString &id )
  : AgentBase( id ),
    d( new Private( this ) )
{
}

CalendarSearchAgent::~CalendarSearchAgent()
{
    delete d;
}

void CalendarSearchAgent::doSetOnline( bool )
{
}

void CalendarSearchAgent::configure( WId )
{
}

AKONADI_AGENT_MAIN( CalendarSearchAgent )


#include "calendarsearchagent.moc"
