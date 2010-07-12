/*
    This file is part of Akonadi.

    Copyright (c) 2009 KDAB
    Author: Sebastian Sauer <sebsauer@kdab.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#ifndef AKONADI_KCAL_CALENDARADAPTOR_H
#define AKONADI_KCAL_CALENDARADAPTOR_H

#include "akonadi-kcal_next_export.h"

#include "groupware.h"
#include "mailscheduler.h"
#include "incidencechanger.h"

#include <akonadi/kcal/calendar.h>
#include <Akonadi/Item>
#include <Akonadi/ItemCreateJob>
#include <Akonadi/ItemDeleteJob>
#include <Akonadi/Collection>
#include <akonadi/kcal/utils.h>

#include <KCal/Calendar>
#include <KCal/CalFilter>
#include <KCal/DndFactory>
#include <KCal/FileStorage>
#include <KCal/FreeBusy>
#include <KCal/ICalDrag>
#include <KCal/ICalFormat>
#include <KCal/VCalFormat>
#include <kcal/listbase.h>

#include <KDE/KMessageBox>

namespace Akonadi {

class AKONADI_KCAL_NEXT_EXPORT CalendarAdaptor : public KCal::Calendar
{
  Q_OBJECT
    // prevent warning about hidden virtual method
    using QObject::event;
    using KCal::Calendar::addIncidence;
    using KCal::Calendar::deleteIncidence;

  public:
    CalendarAdaptor( Akonadi::Calendar *calendar, QWidget *parent,
                     bool storeDefaultCollection = false );

    virtual ~CalendarAdaptor();

    virtual bool save();
    virtual bool reload();
    virtual void close();

    virtual bool addEvent( KCal::Event *event );
    virtual bool deleteEvent( KCal::Event *event );
    virtual void deleteAllEvents();  //unused

    virtual KCal::Event::List rawEvents( KCal::EventSortField sortField = KCal::EventSortUnsorted,
                                         KCal::SortDirection sortDirection = KCal::SortDirectionAscending );

    virtual KCal::Event::List rawEventsForDate( const KDateTime &dt );

    virtual KCal::Event::List rawEvents( const QDate &start, const QDate &end,
                                         const KDateTime::Spec &timeSpec = KDateTime::Spec(),
                                         bool inclusive = false );

    virtual KCal::Event::List rawEventsForDate( const QDate &date,
                                                const KDateTime::Spec &timeSpec = KDateTime::Spec(),
                                                KCal::EventSortField sortField = KCal::EventSortUnsorted,
                                                KCal::SortDirection sortDirection = KCal::SortDirectionAscending );

    virtual KCal::Event *event( const QString &uid );

    virtual bool addTodo( KCal::Todo *todo );

    virtual bool deleteTodo( KCal::Todo *todo );

    virtual void deleteAllTodos(); //unused

    virtual KCal::Todo::List rawTodos( KCal::TodoSortField sortField = KCal::TodoSortUnsorted,
                                 KCal::SortDirection sortDirection = KCal::SortDirectionAscending );

    virtual KCal::Todo::List rawTodosForDate( const QDate &date );

    virtual KCal::Todo *todo( const QString &uid );

    virtual bool addJournal( KCal::Journal *journal );

    virtual bool deleteJournal( KCal::Journal *journal );

    virtual void deleteAllJournals(); //unused

    virtual KCal::Journal::List rawJournals( KCal::JournalSortField sortField = KCal::JournalSortUnsorted,
                                       KCal::SortDirection sortDirection = KCal::SortDirectionAscending );

    virtual KCal::Journal::List rawJournalsForDate( const QDate &dt );

    virtual KCal::Journal *journal( const QString &uid );

    virtual KCal::Alarm::List alarms( const KDateTime &from, const KDateTime &to );

    // From IncidenceChanger
    bool addIncidence( const KCal::Incidence::Ptr &incidence );

    bool deleteIncidence( const Akonadi::Item &aitem, bool deleteCalendar = false );

  private slots:
    void addIncidenceFinished( KJob* j );

    void deleteIncidenceFinished( KJob* j );

  private:
    bool sendGroupwareMessage( const Akonadi::Item &aitem,
                               KCal::iTIPMethod method,
                               IncidenceChanger::HowChanged action );

    //Coming from CalendarView
    void schedule( KCal::iTIPMethod method, const Akonadi::Item &item );

    Akonadi::Collection mDefaultCollection;
    Akonadi::Calendar *mCalendar;
    QWidget *mParent;
    bool mDeleteCalendar;
    bool mStoreDefaultCollection;

};

}

#endif
