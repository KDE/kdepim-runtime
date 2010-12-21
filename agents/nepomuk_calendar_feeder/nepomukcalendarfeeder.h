/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_NEPOMUK_CALENDAR_FEEDER_H
#define AKONADI_NEPOMUK_CALENDAR_FEEDER_H

#include <nepomukfeederagent.h>
#include "calendar.h"

#include <akonadi/agentbase.h>
#include <akonadi/item.h>

#include <kcal/event.h>
#include <kcal/journal.h>
#include <kcal/todo.h>

#include "ncal.h"

namespace AkonadiFuture {

class NepomukCalendarFeeder : public NepomukFeederAgent<NepomukFast::Calendar>
{
  Q_OBJECT

  public:
    NepomukCalendarFeeder( const QString &id );
    ~NepomukCalendarFeeder();

  private:
    void updateItem( const Akonadi::Item &item, const QUrl &graphUri );
    void updateEventItem( const Akonadi::Item& item, const KCal::Event::Ptr&, const QUrl& );
    void updateJournalItem( const Akonadi::Item& item, const KCal::Journal::Ptr&, const QUrl& );
    void updateTodoItem( const Akonadi::Item& item, const KCal::Todo::Ptr&, const QUrl& );

    template <typename IncidencePtr>
    void updateIncidenceItem( const IncidencePtr &calInc, NepomukFast::Resource &incidence, const QUrl &graphUri )
    {
      Q_UNUSED( graphUri );

      incidence.setLabel( calInc->summary() );
      incidence.addProperty( Vocabulary::NCAL::summary(), Soprano::LiteralValue( calInc->summary() ) );
      if ( !calInc->location().isEmpty() )
        incidence.addProperty( Vocabulary::NCAL::location(), Soprano::LiteralValue( calInc->location() ) );
      if ( !calInc->description().isEmpty() )
        incidence.addProperty( Vocabulary::NCAL::description(), Soprano::LiteralValue( calInc->description() ) );

      tagsFromCategories( incidence, calInc->categories() );
    }
};

}

#endif
