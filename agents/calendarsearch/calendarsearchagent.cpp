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
#include "calendarsearchagentadaptor.h"

#include <KDateTime>
#include <KDebug>

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
  new CalendarSearchAgentAdaptor( this );
  if ( !QDBusConnection::sessionBus().registerObject( QLatin1String( "/CalendarSearchAgent" ),
       this, QDBusConnection::ExportAdaptors ) ) {
      kWarning() << "Couldn't register a D-Bus service";
      kWarning() << QDBusConnection::sessionBus().lastError().message();
  }
}

CalendarSearchAgent::~CalendarSearchAgent()
{
    delete d;
}

QVariantMap CalendarSearchAgent::createSearch( const QString& startDateStr, const QString& endDateStr )
{
  const KDateTime startDate = KDateTime::fromString( startDateStr );
  const KDateTime endDate = KDateTime::fromString( endDateStr );
  //setDelayedReply( true );
  //TODO start job
  return QVariantMap();
}

void CalendarSearchAgent::doSetOnline( bool )
{
}

void CalendarSearchAgent::configure( WId )
{
}

AKONADI_AGENT_MAIN( CalendarSearchAgent )


#include "calendarsearchagent.moc"
