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

#include <akonadi/agentbase.h>
#include <akonadi/item.h>
#include <nepomukfeederagent.h>

namespace Soprano
{
class NRLModel;
}

namespace KCal
{
class Event;
class Journal;
class Todo;
}

namespace Akonadi {

class NepomukCalendarFeeder : public NepomukFeederAgent
{
  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.kde.akonadi.NepomukCalendarFeeder" )

  public:
    NepomukCalendarFeeder( const QString &id );
    ~NepomukCalendarFeeder();

  public Q_SLOTS:
    Q_SCRIPTABLE void updateAll( bool force = false );

  private Q_SLOTS:
    void slotInitialItemScan();
    void slotItemsReceivedForInitialScan( const Akonadi::Item::List& items );

  private:
    void updateItem( const Akonadi::Item &item );
    void updateEventItem( const Akonadi::Item &item, KCal::Event*, const QUrl& );
    void updateJournalItem( const Akonadi::Item &item, KCal::Journal*, const QUrl& );
    void updateTodoItem( const Akonadi::Item &item, KCal::Todo*, const QUrl& );

    bool mForceUpdate;
    Soprano::NRLModel *mNrlModel;
};

}

#endif
