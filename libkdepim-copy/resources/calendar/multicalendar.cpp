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

#include <stdlib.h>

#include <qdatetime.h>
#include <qstring.h>
#include <qptrlist.h>

#include <kdebug.h>

#include <libkcal/vcaldrag.h>
#include <libkcal/vcalformat.h>
#include <libkcal/icalformat.h>
#include <libkcal/exceptions.h>
#include <libkcal/incidence.h>
#include <libkcal/journal.h>
#include <libkcal/filestorage.h>

#include <resourcemanager.h>

#include "resourcecalendar.h"
#include "resourcelocal.h"

#include "multicalendar.h"

using namespace KCal;

MultiCalendar::MultiCalendar()
  : Calendar()
{
  init();
}

MultiCalendar::MultiCalendar(const QString &timeZoneId)
  : Calendar(timeZoneId)
{
  init();
}

MultiCalendar::MultiCalendar( ResourceCalendar* resource )
  : Calendar()
{
  init( resource );
}

void MultiCalendar::init( ResourceCalendar* resource )
{
  kdDebug(5800) << "MultiCalendar::init" << endl;
  if ( resource ) {
    mPrivateResource = resource;
    mStandard = 0;
    mManager = 0;
    mKeepPrivateResource = true;
  } else {
    mPrivateResource = 0;
    mKeepPrivateResource = false;
    mManager = new KRES::ResourceManager<ResourceCalendar>( "calendar" );
    mManager->addListener( this );
    mResources = mManager->resources( true ); // get active resources;
    mStandard = mManager->standardResource();
    if ( !mStandard )
      kdDebug() << "FIXME: We don't have a standard resource. Adding events isn't going to work" << endl;

    // Open all active resources
    ResourceCalendar *resource;
    for ( resource = mResources.first(); resource; resource = mResources.next() ) {
      kdDebug(5800) << "Opening resource " + resource->name() << endl;
      bool result = resource->open();
      // Really should remove resource if open not successful
    }
  }
  mOpen = true;
}


MultiCalendar::~MultiCalendar()
{
  kdDebug(5800) << "MultiCalendar::destructor" << endl;
  close();
  if ( mPrivateResource && !mKeepPrivateResource ) {
    delete mPrivateResource;
  }
  if ( mManager ) mManager->removeListener( this );
  delete mManager;
}

bool MultiCalendar::load( const QString &fileName )
{
  if ( mKeepPrivateResource ) return true;
  kdDebug(5800) << "MultiCalendar::load" << endl;
  close();
  ResourceLocal* local = new ResourceLocal( fileName );
  mPrivateResource = local;
  mKeepPrivateResource = false;
  mOpen = true;
  return local->doOpen();
}

bool MultiCalendar::save( const QString &fileName, CalFormat *format )
{
  kdDebug(5800) << "MultiCalendar::save" << endl;
  FileStorage storage( this, fileName, format );
  return storage.save();
}

void MultiCalendar::close()
{
  kdDebug(5800) << "MultiCalendar::close" << endl;
  if ( mOpen ) {
    if ( mPrivateResource ) {
      if ( ! mKeepPrivateResource ) mPrivateResource->close();
    } else {
      ResourceCalendar *resource;
      for ( resource = mResources.first(); resource; resource = mResources.next() ) 
        resource->close();
    }
    setModified( false );
    mOpen = false;
  }
}

void MultiCalendar::addEvent(Event *anEvent)
{
  kdDebug(5800) << "MultiCalendar::addEvent" << endl;
  if ( mPrivateResource ) {
    mPrivateResource->addEvent( anEvent );
  } else {
    if ( mStandard ) {
      mStandard->addEvent( anEvent );
    } else {
      kdDebug() << "FIXME: We don't have a standard resource. Adding events isn't going to work" << endl;
    }
  }
  setModified( true );
}

void MultiCalendar::deleteEvent(Event *event)
{
  kdDebug(5800) << "MultiCalendar::deleteEvent" << endl;
  if ( mPrivateResource ) {
    mPrivateResource->deleteEvent( event );
  } else {
    ResourceCalendar *resource;
    for ( resource = mResources.first(); resource; resource = mResources.next() ) 
      resource->deleteEvent( event );
  }
  setModified( true );
}


Event *MultiCalendar::event( const QString &uid )
{
//  kdDebug(5800) << "MultiCalendar::event(): " << uid << endl;

  if ( mPrivateResource ) {
    return mPrivateResource->event( uid );
  } else {
    ResourceCalendar *resource;
    for ( resource = mResources.first(); resource; resource = mResources.next() ) {
      Event* event = resource->event( uid );
      if ( event ) return event;
    }
  }

  // Not found
  return (Event *) 0L;
}

void MultiCalendar::addTodo(Todo *todo)
{
  kdDebug(5800) << "MultiCalendar::addTodo" << endl;
  if ( mPrivateResource ) {
    mPrivateResource->addTodo( todo );
  } else {
    if ( mStandard ) {
      mStandard->addTodo( todo );
    } else {
      kdDebug() << "FIXME: We don't have a standard resource. Adding todos isn't going to work" << endl;
    }
  }
  setModified( true );
}

void MultiCalendar::deleteTodo(Todo *todo)
{
  kdDebug(5800) << "MultiCalendar::deleteTodo" << endl;
  if ( mPrivateResource ) {
    mPrivateResource->deleteTodo( todo );
  } else {
    ResourceCalendar *resource;
    for ( resource = mResources.first(); resource; resource = mResources.next() ) 
      resource->deleteTodo( todo );
  }
  setModified( true );
}

QPtrList<Todo> MultiCalendar::rawTodos() const
{
  kdDebug(5800) << "MultiCalendar::rawTodos" << endl;
  if ( mPrivateResource ) {
    return mPrivateResource->rawTodos();
  } else {
    QPtrList<Todo> result;
    ResourceCalendar *resource;
    QPtrListIterator<ResourceCalendar> it( mResources );
    while ( ( resource = it.current() ) != 0 ) {
      ++it;
      kdDebug() << "Getting raw todos from " << resource->name() << endl;
      QPtrList<Todo> todos = resource->rawTodos();
      Todo* todo;
      for ( todo = todos.first(); todo; todo = todos.next() ) {
        kdDebug() << "Adding todo to result" << endl;
        result.append( todo );
      }
    }
    return result;
  }
}

Todo *MultiCalendar::todo( const QString &uid )
{
  kdDebug(5800) << "MultiCalendar::todo(uid)" << endl;
  if ( mPrivateResource ) {
    return mPrivateResource->todo( uid );
  } else {
    ResourceCalendar *resource;
    for ( resource = mResources.first(); resource; resource = mResources.next() ) {
      Todo* todo = resource->todo( uid );
      if ( todo ) return todo;
    }
  }

  // not found
  return 0;
}

QPtrList<Todo> MultiCalendar::todos( const QDate &date )
{
  kdDebug(5800) << "MultiCalendar::todos(date)" << endl;
  if ( mPrivateResource ) {
    return mPrivateResource->todos( date );
  } else {
    QPtrList<Todo> result;
    ResourceCalendar *resource;
    for ( resource = mResources.first(); resource; resource = mResources.next() ) {
      QPtrList<Todo> todos = resource->todos( date );
      Todo* todo;
      for ( todo = todos.first(); todo; todo = todos.next() )
        result.append( todo );
    }
    return result;
  }
}

int MultiCalendar::numEvents(const QDate &qd)
{
  kdDebug(5800) << "MultiCalendar::numEvents" << endl;
  if ( mPrivateResource ) {
    return mPrivateResource->numEvents( qd );
  } else {
    int count = 0;
    ResourceCalendar *resource;
    for ( resource = mResources.first(); resource; resource = mResources.next() ) 
      count += resource->numEvents( qd );
    return count;
  }
}


Alarm::List MultiCalendar::alarmsTo( const QDateTime &to )
{
  kdDebug(5800) << "MultiCalendar::alarmsTo" << endl;
  if ( mPrivateResource ) {
    return mPrivateResource->alarmsTo( to );
  } else {
    Alarm::List result;
    ResourceCalendar *resource;
    for ( resource = mResources.first(); resource; resource = mResources.next() ) {
      Alarm::List list = resource->alarmsTo( to );
      Alarm::List::iterator it;
      for ( it = list.begin(); it != list.end(); ++it )
        result.append( *it );
    }
    return result;
  }
}

Alarm::List MultiCalendar::alarms( const QDateTime &from, const QDateTime &to )
{
  kdDebug(5800) << "MultiCalendar::alarms(" << from.toString() << " - " << to.toString() << ")\n";
  if ( mPrivateResource ) {
    return mPrivateResource->alarms( from, to );
  } else {
    Alarm::List result;
    ResourceCalendar *resource;
    for ( resource = mResources.first(); resource; resource = mResources.next() ) {
      Alarm::List list = resource->alarms( from, to );
      Alarm::List::iterator it;
      for ( it = list.begin(); it != list.end(); ++it )
        result.append( *it );
    }
    return result;
  }
}

/****************************** PROTECTED METHODS ****************************/


// taking a QDate, this function will look for an eventlist in the dict
// with that date attached -
QPtrList<Event> MultiCalendar::rawEventsForDate(const QDate &qd, bool sorted)
{
  // kdDebug(5800) << "MultiCalendar::rawEventsForDate" << endl;
  if ( mPrivateResource ) {
    return mPrivateResource->rawEventsForDate( qd, sorted );
  } else {
    QPtrList<Event> result;
    ResourceCalendar *resource;
    for ( resource = mResources.first(); resource; resource = mResources.next() ) {
      // kdDebug() << "Getting events from " << resource->name() << endl;
      QPtrList<Event> list = resource->rawEventsForDate( qd, sorted );
      if ( sorted ) {
        Event* item;
        uint insertionPoint = 0;
        for ( item = list.first(); item; item = list.next() ) {
          while ( insertionPoint<result.count() && 
                  result.at( insertionPoint )->dtStart().time() <= item->dtStart().time() ) 
            insertionPoint++;
          result.insert( insertionPoint, item );
        }
      } else {
        Event* item;
        for ( item = list.first(); item; item = list.next() ) {
          result.append( item );
        }
      }
    }
    return result;
  }
}

QPtrList<Event> MultiCalendar::rawEvents( const QDate &start, const QDate &end,
                                          bool inclusive )
{
  kdDebug(5800) << "MultiCalendar::rawEvents(start,end,inclusive)" << endl;
  if ( mPrivateResource ) {
    return mPrivateResource->rawEvents( start, end, inclusive);
  } else {
    QPtrList<Event> result;
    ResourceCalendar *resource;
    for ( resource = mResources.first(); resource; resource = mResources.next() ) {
      QPtrList<Event> list = resource->rawEvents( start, end, inclusive );
      Event* item;
      for ( item = list.first(); item; item = list.next() ) {
        result.append( item );
      }
    }
    return result;
  }
}

QPtrList<Event> MultiCalendar::rawEventsForDate(const QDateTime &qdt)
{
  kdDebug(5800) << "MultiCalendar::rawEventsForDate(qdt)" << endl;
  if ( mPrivateResource ) {
    return mPrivateResource->rawEventsForDate( qdt );
  } else {
    QPtrList<Event> result;
    ResourceCalendar *resource;
    for ( resource = mResources.first(); resource; resource = mResources.next() ) {
      QPtrList<Event> list = resource->rawEventsForDate( qdt );
      Event* item;
      for ( item = list.first(); item; item = list.next() ) {
        result.append( item );
      }
    }
    return result;
  }
}

QPtrList<Event> MultiCalendar::rawEvents()
{
  kdDebug(5800) << "MultiCalendar::rawEvents()" << endl;
  if ( mPrivateResource ) {
    return mPrivateResource->rawEvents();
  } else {
    QPtrList<Event> result;
    ResourceCalendar *resource;
    for ( resource = mResources.first(); resource; resource = mResources.next() ) {
      QPtrList<Event> list = resource->rawEvents();
      Event* item;
      for ( item = list.first(); item; item = list.next() ) {
        result.append( item );
      }
    }
    return result;
  }
}


void MultiCalendar::addJournal(Journal *journal)
{
  kdDebug(5800) << "Adding Journal on " << journal->dtStart().toString() << endl;
  if ( mPrivateResource ) {
    mPrivateResource->addJournal( journal );
  } else {
    if ( mStandard ) {
      mStandard->addJournal( journal );
    } else {
      kdDebug() << "FIXME: We don't have a standard resource. Adding journals isn't going to work" << endl;
    }
  }
  setModified( true );
}

Journal *MultiCalendar::journal(const QDate &date)
{
  kdDebug(5800) << "MultiCalendar::journal() " << date.toString() << endl;
  kdDebug(5800) << "FIXME: what to do with the multiple journals from multiple calendar resources????" << endl;

  // If we're on a private resource, return that journal.
  // Else, first see if the standard resource has a journal for this date. If it has, return that journal.
  // If not, check all resources for a journal for this date.

  if ( mPrivateResource ) {
    return mPrivateResource->journal( date );
  } else {
    if ( mStandard ) {
      Journal* journal = mStandard->journal( date );
      if ( journal ) return journal;
    } 
    ResourceCalendar *resource;
    for ( resource = mResources.first(); resource; resource = mResources.next() ) {
      Journal* journal = resource->journal( date );
      if ( journal ) return journal;
    }
  }

  return 0;
}

Journal *MultiCalendar::journal(const QString &uid)
{
  kdDebug(5800) << "MultiCalendar::journal(uid)" << endl;
  if ( mPrivateResource ) {
    return mPrivateResource->journal( uid );
  } else {
    ResourceCalendar *resource;
    for ( resource = mResources.first(); resource; resource = mResources.next() ) {
      Journal* journal = resource->journal( uid );
      if ( journal ) return journal;
    }
  }

  // not found
  return 0;
}

QPtrList<Journal> MultiCalendar::journals()
{
  kdDebug(5800) << "MultiCalendar::journals()" << endl;
  if ( mPrivateResource ) {
    return mPrivateResource->journals();
  } else {
    QPtrList<Journal> result;
    ResourceCalendar *resource;
    for ( resource = mResources.first(); resource; resource = mResources.next() ) {
      QPtrList<Journal> list = resource->journals();
      Journal* item;
      for ( item = list.first(); item; item = list.next() ) {
        result.append( item );
      }
    }
    return result;
  }
}


// KRES::ManagerListener stuff
void MultiCalendar::resourceAdded( ResourceCalendar* resource ) {
  kdDebug() << "MultiCalendar::resourceAdded( " << resource->name() << " )" << endl;
  kdDebug() << "Ignoring for now..." << endl;
}
void MultiCalendar::resourceModified( ResourceCalendar* resource ) {
  kdDebug() << "MultiCalendar::resourceModified( " << resource->name() << " )" << endl;
  kdDebug() << "We should probably close and re-open the resource now!" << endl;
}
void MultiCalendar::resourceDeleted( ResourceCalendar* resource ) {
  kdDebug() << "MultiCalendar::resourceDeleted( " << resource->name() << " )" << endl;
  kdDebug() << "Closing the resource..." << endl;
  mResources.remove( resource );
  resource->close();
  if ( mStandard == resource ) {
    kdWarning() << "Deleted standard resource!" << endl;
    mStandard = 0;
  }
  // But now there may be Events, Journal items, Todos, etc dangling in KOrganizer
  setModified( true ); // let 'm know

}

void MultiCalendar::incidenceUpdated( IncidenceBase * ) {
  kdDebug() << "MultiCalendar::incidenceUpdated( IncidenceBase * ): Not yet implemented\n";
}
