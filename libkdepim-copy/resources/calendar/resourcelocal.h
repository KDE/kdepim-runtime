 /*
    This file is part of libkcal.
    Copyright (c) 1998 Preston Brown
    Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>

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
#ifndef KCAL_RESOURCELOCAL_H
#define KCAL_RESOURCELOCAL_H

#include <qintdict.h>
#include <qptrlist.h>
#include <qmap.h>
#include <qstring.h>
#include <qdatetime.h>

#include <kurl.h>
#include <kconfig.h>

#include <libkcal/incidence.h>

#include "resourcecalendar.h"

#define BIGPRIME 1031 /* should allow for at least 4 appointments 365 days/yr
			 to be almost instantly fast. */

namespace KCal {

/**
  This class provides a calendar stored as a local file.
*/
class ResourceLocal : public ResourceCalendar, public IncidenceBase::Observer
{
  public:
    ResourceLocal( const KConfig * );
    ResourceLocal( const QString& fileName );
    virtual ~ResourceLocal();

    virtual void writeConfig( KConfig* config );

    bool sync();

    /** Add Event to calendar. */
    void addEvent(Event *anEvent);
    /** deletes an event from this calendar. */
    void deleteEvent(Event *);

    /**
      Retrieves an event on the basis of the unique string ID.
    */
    Event *event(const QString &UniqueStr);
    /**
      Return filtered list of all events in calendar.
    */
//    QPtrList<Event> events();
    /**
      Return unfiltered list of all events in calendar.
    */
    QPtrList<Event> rawEvents();
    /**
      Builds and then returns a list of all events that match for the
      date specified. useful for dayView, etc. etc.
    */
    QPtrList<Event> rawEventsForDate( const QDate &date, bool sorted = false );
    /**
      Get unfiltered events for date \a qdt.
    */
    QPtrList<Event> rawEventsForDate( const QDateTime &qdt );
    /**
      Get unfiltered events in a range of dates. If inclusive is set to true,
      only events are returned, which are completely included in the range.
    */
    QPtrList<Event> rawEvents( const QDate &start, const QDate &end,
                               bool inclusive = false );

    /** returns the number of events that are present on the specified date. */
    int numEvents(const QDate &qd);
  

    /*
      Returns a QString with the text of the holiday (if any) that falls
      on the specified date.
    */
    // QString getHolidayForDate(const QDate &qd);
    
    /**
      Add a todo to the todolist.
    */
    void addTodo( Todo *todo );
    /**
      Remove a todo from the todolist.
    */
    void deleteTodo( Todo * );
    /**
      Searches todolist for an event with this unique string identifier,
      returns a pointer or null.
    */
    Todo *todo( const QString &uid );
    /**
      Return list of all todos.
    */
    QPtrList<Todo> rawTodos() const;
    /**
      Returns list of todos due on the specified date.
    */
    QPtrList<Todo> todos( const QDate &date );
    /** Add a Journal entry to calendar */
    virtual void addJournal(Journal *);
    /** Return Journal for given date */
    virtual Journal *journal(const QDate &);
    /** Return Journal with given UID */
    virtual Journal *journal(const QString &UID);
    /** Return list of all Journals stored in calendar */
    QPtrList<Journal> journals();

    /** Return all alarms, which ocur in the given time interval. */
    Alarm::List alarms( const QDateTime &from, const QDateTime &to );

    /** Return all alarms, which ocur before given date. */
    Alarm::List alarmsTo( const QDateTime &to );

    
    /** this method should be called whenever a Event is modified directly
     * via it's pointer.  It makes sure that the calendar is internally
     * consistent. */
    void update(IncidenceBase *incidence);
 
    friend class ResourceLocalConfig;

    // Public because needed in MultiCalendar::load()
    bool doOpen();

  protected:

    /** clears out the current calendar, freeing all used memory etc. etc. */
    void doClose();
 
    /** Notification function of IncidenceBase::Observer. */
    void incidenceUpdated( IncidenceBase *i ) { update( i ); }
  
    /** inserts an event into its "proper place" in the calendar. */
    void insertEvent(const Event *anEvent);
  
    /** on the basis of a QDateTime, forms a hash key for the dictionary. */
    long int makeKey(const QDateTime &dt);
    /** on the basis of a QDate, forms a hash key for the dictionary */
    long int makeKey(const QDate &d);
    /** Return the date for which the specified key was made. */
    QDate keyToDate(long int key);

    /** Append alarms of incidence in interval to list of alarms. */
    void appendAlarms( Alarm::List &alarms, Incidence *incidence,
                       const QDateTime &from, const QDateTime &to );

    /** Append alarms of recurring events in interval to list of alarms. */
    void appendRecurringAlarms( Alarm::List &alarms, Incidence *incidence,
                       const QDateTime &from, const QDateTime &to );

  private:
    void init();

    QIntDict<QPtrList<Event> > *mCalDict;    // dictionary of lists of events.
    QPtrList<Event> mRecursList;             // list of repeating events.

    QPtrList<Todo> mTodoList;               // list of todo items.

    QMap<QDate,Journal *> mJournalMap;
  
    QDate *mOldestDate;
    QDate *mNewestDate;

    KURL mURL;
    CalFormat* mFormat;
};  

}

#endif
