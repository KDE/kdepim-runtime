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
#ifndef KCAL_MULTICALENDAR_H
#define KCAL_MULTICALENDAR_H

#include <qintdict.h>
#include <qmap.h>

#include <libkcal/calendar.h>

#include <resourcemanager.h>

namespace KCal {

class CalFormat;
class ResourceCalendar;

/**
  This class provides a calendar stored as a local file.
*/
class MultiCalendar : public Calendar, private KRES::ManagerListener<ResourceCalendar>
{
  public:
    /** constructs a new calendar that uses the ResourceManager for "calendar" */
    MultiCalendar();
    /** constructs a new calendar, with variables initialized to sane values. */
    MultiCalendar( const QString &timeZoneId );
    /** constructs a new calendar that uses a single private resource */
    MultiCalendar( ResourceCalendar* resource );
    virtual ~MultiCalendar();
  
    /**
      Loads a calendar on disk in vCalendar or iCalendar format into the current
      calendar. Any information already present is lost.
      @return true, if successfull, false on error.
      @param fileName the name of the calendar on disk.
    */
    bool load( const QString &fileName );
    /**
      Writes out the calendar to disk in the specified \a format.
      MultiCalendar takes ownership of the CalFormat object.
      @return true, if successfull, false on error.
      @param fileName the name of the file
    */
    bool save( const QString &fileName, CalFormat *format = 0 );

    /** clears out the current calendar, freeing all used memory etc. etc. */
    void close();
  
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

    /*
      Returns a QString with the text of the holiday (if any) that falls
      on the specified date.
    */
    QString getHolidayForDate(const QDate &qd);
    
    /** returns the number of events that are present on the specified date. */
    int numEvents(const QDate &qd);
  
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
    /**
      Return list of all todos.
      
      Workaround because compiler does not recognize function of base class.
    */
    QPtrList<Todo> todos() { return Calendar::todos(); }

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

  protected:
    /**
      The observer interface. So far not implemented.
    */
    virtual void incidenceUpdated( IncidenceBase * );

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

  // From ManagerListener<ResourceCalendar>
public:
  virtual void resourceAdded( ResourceCalendar* resource );
  virtual void resourceModified( ResourceCalendar* resource );
  virtual void resourceDeleted( ResourceCalendar* resource );

private:
    void init( ResourceCalendar* resource = 0 );

    ResourceCalendar* mPrivateResource;
    bool mKeepPrivateResource;
    QPtrList<ResourceCalendar> mResources;
    ResourceCalendar* mStandard;
    bool mOpen;

    KRES::ResourceManager<ResourceCalendar>* mManager;
};  

}

#endif
