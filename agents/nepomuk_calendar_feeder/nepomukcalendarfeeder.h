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

#include <nepomukfeederagentbase.h>

#include <akonadi/agentbase.h>
#include <akonadi/item.h>

#include <kcalcore/event.h>
#include <kcalcore/journal.h>
#include <kcalcore/todo.h>


namespace Akonadi {

class NepomukCalendarFeeder : public NepomukFeederAgentBase
{
  Q_OBJECT

  public:
    NepomukCalendarFeeder( const QString &id );
    ~NepomukCalendarFeeder();

  private:
    void updateItem( const Akonadi::Item &item, Nepomuk::SimpleResource &res, Nepomuk::SimpleResourceGraph &graph );
    void updateEventItem( const Akonadi::Item& item, const KCalCore::Event::Ptr&, Nepomuk::SimpleResource &res, Nepomuk::SimpleResourceGraph &graph );
    void updateJournalItem( const Akonadi::Item& item, const KCalCore::Journal::Ptr&, Nepomuk::SimpleResource &res, Nepomuk::SimpleResourceGraph &graph );
    void updateTodoItem( const Akonadi::Item& item, const KCalCore::Todo::Ptr&, Nepomuk::SimpleResource &res, Nepomuk::SimpleResourceGraph &graph );
  
    void updateIncidenceItem( const KCalCore::Incidence::Ptr &calInc, Nepomuk::SimpleResource &res , Nepomuk::SimpleResourceGraph &graph);
    
    /** Finds (or if it doesn't exist creates) a PersonContact object for the given name and address.
    @param found Used to indicate if the contact is already there are was just newly created. In the latter case you might
    want to add additional information you have available for it.
    
    If a new contact is created it is added to @param graph and it's url is returned.
    */
    static QUrl findOrCreateContact( const QString &email, const QString &name, Nepomuk::SimpleResourceGraph &graph, bool *found = 0 );
};

}

#endif
