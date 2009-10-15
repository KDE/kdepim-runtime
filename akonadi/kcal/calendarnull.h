/*
  This file is part of the kcal library.

  Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>

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
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/
/**
  @file
  This file is part of the API for handling calendar data and
  defines the CalendarNull class.

  @author Cornelius Schumacher \<schumacher@kde.org\>
*/
#ifndef AKONADI_KCAL_CALENDARNULL_H
#define AKONADI_KCAL_CALENDARNULL_H

#include "calendarbase.h"
#include "akonadi-kcal_export.h"

namespace KOrg {

/**
   @brief
   Represents a null calendar class; that is, a calendar which contains
   no information and provides no capabilities.

   The null calendar can be passed to functions which need a calendar object
   when there is no real calendar available yet.

   CalendarNull can be used to implement the null object design pattern:
   pass a CalendarNull object instead of passing a 0 pointer and checking
   for 0 with each access.
*/
class AKONADI_KCAL_EXPORT CalendarNull : public CalendarBase
{
  public:
    /**
      Construct Calendar object using a time specification (time zone, etc.).
      The time specification is used for creating or modifying incidences
      in the Calendar. It is also used for viewing incidences (see
      setViewTimeSpec()). The time specification does not alter existing
      incidences.

      @param timeSpec time specification
    */
    explicit CalendarNull( const KDateTime::Spec &timeSpec );

    /**
      Constructs a null calendar with a specified time zone @p timeZoneId.

      @param timeZoneId is a string containing a time zone ID, which is
      assumed to be valid.  If no time zone is found, the viewing time
      specification is set to local clock time.
      @e Example: "Europe/Berlin"
    */
    explicit CalendarNull( const QString &timeZoneId );

    /**
      Destroys the null calendar.
    */
    ~CalendarNull();

    /**
      Returns a pointer to the CalendarNull object, of which there can
      be only one.  The object is constructed if necessary.
    */
    static CalendarNull *self();

    /**
      @copydoc
      Calendar::close()
    */
    void close();

    /**
      @copydoc
      Calendar::save()
    */
    bool save();

    /**
      @copydoc
      Calendar::reload()
    */
    bool reload();

  // Event Specific Methods //

    /**
      @copydoc
      Calendar::addEvent()
    */
    bool addEvent( const KCal::Event::Ptr &event );

    /**
      @copydoc
      Calendar::deleteEvent()
    */
    bool deleteEvent( const Akonadi::Item &event );

    /**
      @copydoc
      Calendar::deleteAllEvents()
    */
    void deleteAllEvents();

    /**
      @copydoc
      Calendar::rawEvents(EventSortField, SortDirection)
    */
    Akonadi::Item::List rawEvents( EventSortField sortField,
                           SortDirection sortDirection );

    /**
      @copydoc
      Calendar::rawEvents(const QDate &, const QDate &, const KDateTime::Spec &, bool)
    */
    Akonadi::Item::List rawEvents( const QDate &start, const QDate &end,
                                 const KDateTime::Spec &timeSpec = KDateTime::Spec(),
                                 bool inclusive = false );

    /**
      Returns an unfiltered list of all Events which occur on the given date.

      @param date request unfiltered Event list for this QDate only.
      @param timeSpec time zone etc. to interpret @p date, or the calendar's
                      default time spec if none is specified
      @param sortField specifies the EventSortField.
      @param sortDirection specifies the SortDirection.

      @return the list of unfiltered Events occurring on the specified QDate.
    */
    Akonadi::Item::List rawEventsForDate( const QDate &date,
                                        const KDateTime::Spec &timeSpec = KDateTime::Spec(),
                                        KOrg::EventSortField sortField = KOrg::EventSortUnsorted,
                                        KOrg::SortDirection sortDirection = KOrg::SortDirectionAscending );

    /**
      @copydoc
      Calendar::rawEventsForDate(const KDateTime &)
    */
    Akonadi::Item::List rawEventsForDate( const KDateTime &dt );

    /**
      @copydoc
      Calendar::event()
    */
    Akonadi::Item event( const Akonadi::Item::Id &id );

  // To-do Specific Methods //

    /**
      @copydoc
      Calendar::addTodo()
    */
    bool addTodo( const KCal::Todo::Ptr &todo );

    /**
      @copydoc
      Calendar::deleteTodo()
    */
    bool deleteTodo( const Akonadi::Item &todo );

    /**
      @copydoc
      Calendar::deleteAllTodos()
    */
    void deleteAllTodos();

    /**
      @copydoc
      Calendar::rawTodos()
    */
    Akonadi::Item::List rawTodos( KOrg::TodoSortField sortField,
                               KOrg::SortDirection sortDirection );

    /**
      @copydoc
      Calendar::rawTodosForDate()
    */
    Akonadi::Item::List rawTodosForDate( const QDate &date );

    /**
      @copydoc
      Calendar::todo()
    */
    Akonadi::Item todo( const Akonadi::Item::Id &id );

  // Journal Specific Methods //

    /**
      @copydoc
      Calendar::addJournal()
    */
    bool addJournal( const KCal::Journal::Ptr &journal );

    /**
      @copydoc
      Calendar::deleteJournal()
    */
    bool deleteJournal( const Akonadi::Item &journal );

    /**
      @copydoc
      Calendar::deleteAllJournals()
    */
    void deleteAllJournals();

    /**
      @copydoc
      Calendar::rawJournals()
    */
    Akonadi::Item::List rawJournals( KOrg::JournalSortField sortField,
                                     KOrg::SortDirection sortDirection );

    /**
      @copydoc
      Calendar::rawJournalsForDate()
    */
    Akonadi::Item::List rawJournalsForDate( const QDate &date );

    /**
      @copydoc
      Calendar::journal()
    */
    Akonadi::Item journal( const Akonadi::Item::Id &id );

  // Alarm Specific Methods //

    /**
      @copydoc
      Calendar::alarms()
    */
    Akonadi::Item::List alarms( const KDateTime &from, const KDateTime &to );

  // Observer Specific Methods //

    /**
      @copydoc
      Calendar::incidenceUpdated()
    */
    void incidenceUpdated( KCal::IncidenceBase *incidenceBase );

    /* reimp */ Akonadi::Item findParent( const Akonadi::Item& item ) const;
    /* reimp */ Akonadi::Item::List findChildren( const Akonadi::Item &item ) const;
    /* reimp */ bool isChild( const Akonadi::Item& parent, const Akonadi::Item &child ) const;

    using QObject::event;   // prevent warning about hidden virtual method

  private:
    //@cond PRIVATE
    Q_DISABLE_COPY( CalendarNull )
    class Private;
    Private *const d;
    //@endcond
};

}

#endif
