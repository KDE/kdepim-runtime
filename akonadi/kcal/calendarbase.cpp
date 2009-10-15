/*
  This file is part of the kcal library.

  Copyright (c) 1998 Preston Brown <pbrown@kde.org>
  Copyright (c) 2000-2004 Cornelius Schumacher <schumacher@kde.org>
  Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  Copyright (c) 2006 David Jarvie <software@astrojar.org.uk>

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
  defines the Calendar class.

  @brief
  Represents the main calendar class.

  @author Preston Brown \<pbrown@kde.org\>
  @author Cornelius Schumacher \<schumacher@kde.org\>
  @author Reinhold Kainhofer \<reinhold@kainhofer.com\>
  @author David Jarvie \<software@astrojar.org.uk\>
*/

#include "calendarbase.h"
#include "utils.h"

#include <kcal/exceptions.h>
#include <kcal/calfilter.h>
#include <kcal/icaltimezones.h>
#include <kdebug.h>
#include <klocale.h>

extern "C" {
  #include <libical/icaltimezone.h>
}

using namespace Akonadi;
using namespace boost;
using namespace KCal;
using namespace KOrg;

namespace {
  class AddVisitor : public IncidenceBase::Visitor {
    CalendarBase* const mCalendar;
    const Incidence::Ptr mInc;
  public:
    explicit AddVisitor( CalendarBase* cal, const Incidence::Ptr& inc ) : mCalendar( cal ), mInc( inc ) {}

    /* reimp */ bool visit( Event * ) {
      return mCalendar->addEvent( static_pointer_cast<Event>( mInc ) );
    }

    /* reimp */ bool visit( Todo * ) {
      return mCalendar->addTodo( static_pointer_cast<Todo>( mInc ) );
    }

    /* reimp */ bool visit( Journal * ) {
      return mCalendar->addJournal( static_pointer_cast<Journal>( mInc ) );
    }
  };

  class DeleteVisitor : public IncidenceBase::Visitor {
    CalendarBase* const mCalendar;
    const Item mItem;
  public:
    explicit DeleteVisitor( CalendarBase* cal, const Item& item ) : mCalendar( cal ), mItem( item ) {}

    /* reimp */ bool visit( Event * ) {
      return mCalendar->deleteEvent( mItem );
    }

    /* reimp */ bool visit( Todo * ) {
      return mCalendar->deleteTodo( mItem );
    }

    /* reimp */ bool visit( Journal * ) {
      return mCalendar->deleteJournal( mItem );
    }
  };

}

/**
  Private class that helps to provide binary compatibility between releases.
  @internal
*/
//@cond PRIVATE
class KOrg::CalendarBase::Private
{
  public:
    Private()
      : mTimeZones( new ICalTimeZones ),
        mModified( false ),
        mNewObserver( false ),
        mObserversEnabled( true ),
        mDefaultFilter( new CalFilter )
    {
      // Setup default filter, which does nothing
      mFilter = mDefaultFilter;
      mFilter->setEnabled( false );

      // user information...
      mOwner.setName( i18n( "Unknown Name" ) );
      mOwner.setEmail( i18n( "unknown@nowhere" ) );
    }

    ~Private()
    {
      delete mTimeZones;
      delete mDefaultFilter;
    }
    KDateTime::Spec timeZoneIdSpec( const QString &timeZoneId, bool view );

    QString mProductId;
    Person mOwner;
    ICalTimeZones *mTimeZones; // collection of time zones used in this calendar
    ICalTimeZone mBuiltInTimeZone;   // cached time zone lookup
    ICalTimeZone mBuiltInViewTimeZone;   // cached viewing time zone lookup
    KDateTime::Spec mTimeSpec;
    mutable KDateTime::Spec mViewTimeSpec;
    bool mModified;
    bool mNewObserver;
    bool mObserversEnabled;
    QList<CalendarObserver*> mObservers;

    CalFilter *mDefaultFilter;
    CalFilter *mFilter;

#ifdef AKONADI_PORT_DISABLED
    // These lists are used to put together related To-dos
    QMultiHash<QString, Incidence*> mOrphans;
    QMultiHash<QString, Incidence*> mOrphanUids;
#endif
};
//@endcond

CalendarBase::CalendarBase( const KDateTime::Spec &timeSpec )
  : d( new Private )
{
  d->mTimeSpec = timeSpec;
  d->mViewTimeSpec = timeSpec;
}

CalendarBase::CalendarBase( const QString &timeZoneId )
  : d( new Private )
{
  setTimeZoneId( timeZoneId );
}

CalendarBase::~CalendarBase()
{
  delete d;
}

Person CalendarBase::owner() const
{
  return d->mOwner;
}

void CalendarBase::setOwner( const Person &owner )
{
  d->mOwner = owner;

  setModified( true );
}

void CalendarBase::setTimeSpec( const KDateTime::Spec &timeSpec )
{
  d->mTimeSpec = timeSpec;
  d->mBuiltInTimeZone = ICalTimeZone();
  setViewTimeSpec( timeSpec );

  doSetTimeSpec( d->mTimeSpec );
}

KDateTime::Spec CalendarBase::timeSpec() const
{
  return d->mTimeSpec;
}

void CalendarBase::setTimeZoneId( const QString &timeZoneId )
{
  d->mTimeSpec = d->timeZoneIdSpec( timeZoneId, false );
  d->mViewTimeSpec = d->mTimeSpec;
  d->mBuiltInViewTimeZone = d->mBuiltInTimeZone;

  doSetTimeSpec( d->mTimeSpec );
}

//@cond PRIVATE
KDateTime::Spec CalendarBase::Private::timeZoneIdSpec( const QString &timeZoneId,
                                                   bool view )
{
  if ( view ) {
    mBuiltInViewTimeZone = ICalTimeZone();
  } else {
    mBuiltInTimeZone = ICalTimeZone();
  }
  if ( timeZoneId == QLatin1String( "UTC" ) ) {
    return KDateTime::UTC;
  }
  ICalTimeZone tz = mTimeZones->zone( timeZoneId );
  if ( !tz.isValid() ) {
    ICalTimeZoneSource tzsrc;
#ifdef AKONADI_PORT_DISABLED
    tz = tzsrc.parse( icaltimezone_get_builtin_timezone( timeZoneId.toLatin1() ) );
#endif
    if ( view ) {
      mBuiltInViewTimeZone = tz;
    } else {
      mBuiltInTimeZone = tz;
    }
  }
  if ( tz.isValid() ) {
    return tz;
  } else {
    return KDateTime::ClockTime;
  }
}
//@endcond

QString CalendarBase::timeZoneId() const
{
  KTimeZone tz = d->mTimeSpec.timeZone();
  return tz.isValid() ? tz.name() : QString();
}

void CalendarBase::setViewTimeSpec( const KDateTime::Spec &timeSpec ) const
{
  d->mViewTimeSpec = timeSpec;
  d->mBuiltInViewTimeZone = ICalTimeZone();
}

void CalendarBase::setViewTimeZoneId( const QString &timeZoneId ) const
{
  d->mViewTimeSpec = d->timeZoneIdSpec( timeZoneId, true );
}

KDateTime::Spec CalendarBase::viewTimeSpec() const
{
  return d->mViewTimeSpec;
}

QString CalendarBase::viewTimeZoneId() const
{
  KTimeZone tz = d->mViewTimeSpec.timeZone();
  return tz.isValid() ? tz.name() : QString();
}

ICalTimeZones *CalendarBase::timeZones() const
{
  return d->mTimeZones;
}

void CalendarBase::shiftTimes( const KDateTime::Spec &oldSpec,
                           const KDateTime::Spec &newSpec )
{
  setTimeSpec( newSpec );
  int i, end;
  Item::List ev = events();
  for ( i = 0, end = ev.count();  i < end;  ++i ) {
    Akonadi::event( ev[i] )->shiftTimes( oldSpec, newSpec );
  }

  Item::List to = todos();
  for ( i = 0, end = to.count();  i < end;  ++i ) {
    Akonadi::todo( to[i] )->shiftTimes( oldSpec, newSpec );
  }

  Item::List jo = journals();
  for ( i = 0, end = jo.count();  i < end;  ++i ) {
    Akonadi::journal( jo[i] )->shiftTimes( oldSpec, newSpec );
  }
}

void CalendarBase::setFilter( CalFilter *filter )
{
  if ( filter ) {
    d->mFilter = filter;
  } else {
    d->mFilter = d->mDefaultFilter;
  }
}

CalFilter *CalendarBase::filter()
{
  return d->mFilter;
}

QStringList CalendarBase::categories()
{
  Item::List rawInc( rawIncidences() );
  QStringList cats, thisCats;
  // @TODO: For now just iterate over all incidences. In the future,
  // the list of categories should be built when reading the file.
  Q_FOREACH( const Item &i, rawInc ) {
    thisCats = Akonadi::incidence( i )->categories();
    for ( QStringList::ConstIterator si = thisCats.constBegin();
          si != thisCats.constEnd(); ++si ) {
      if ( !cats.contains( *si ) ) {
        cats.append( *si );
      }
    }
  }
  return cats;
}

Item::List CalendarBase::incidences( const QDate &date )
{
  return mergeIncidenceList( events( date ), todos( date ), journals( date ) );
}

Item::List CalendarBase::incidences()
{
  return mergeIncidenceList( events(), todos(), journals() );
}

Item::List CalendarBase::rawIncidences()
{
  return mergeIncidenceList( rawEvents(), rawTodos(), rawJournals() );
}

Item::List CalendarBase::sortEvents( const Item::List &eventList_,
                                  EventSortField sortField,
                                  SortDirection sortDirection )
{
  Item::List eventList = eventList_;
  Item::List eventListSorted;
  Item::List tempList, t;
  Item::List alphaList;
  Item::List::Iterator sortIt;
  Item::List::Iterator eit;

  // Notice we alphabetically presort Summaries first.
  // We do this so comparison "ties" stay in a nice order.

  switch( sortField ) {
  case EventSortUnsorted:
    eventListSorted = eventList;
    break;

  case EventSortStartDate:
    alphaList = sortEvents( eventList, EventSortSummary, sortDirection );
    for ( eit = alphaList.begin(); eit != alphaList.end(); ++eit) {
      Event::Ptr e = Akonadi::event( *eit );
      Q_ASSERT( e );
      if ( e->dtStart().isDateOnly() ) {
        tempList.append( *eit );
        continue;
      }
      sortIt = eventListSorted.begin();
      if ( sortDirection == SortDirectionAscending ) {
        while ( sortIt != eventListSorted.end() &&
                e->dtStart() >= Akonadi::event(*sortIt)->dtStart() ) {
          ++sortIt;
        }
      } else {
        while ( sortIt != eventListSorted.end() &&
                e->dtStart() < Akonadi::event(*sortIt)->dtStart() ) {
          ++sortIt;
        }
      }
      eventListSorted.insert( sortIt, *eit );
    }
    if ( sortDirection == SortDirectionAscending ) {
      // Prepend the list of Events without End DateTimes
      tempList += eventListSorted;
      eventListSorted = tempList;
    } else {
      // Append the list of Events without End DateTimes
      eventListSorted += tempList;
    }
    break;

  case EventSortEndDate:
    alphaList = sortEvents( eventList, EventSortSummary, sortDirection );
    for ( eit = alphaList.begin(); eit != alphaList.end(); ++eit ) {
      Event::Ptr e = Akonadi::event( *eit );
      Q_ASSERT( e );
      if ( e->hasEndDate() ) {
        sortIt = eventListSorted.begin();
        if ( sortDirection == SortDirectionAscending ) {
          while ( sortIt != eventListSorted.end() &&
                  e->dtEnd() >= Akonadi::event(*sortIt)->dtEnd() ) {
            ++sortIt;
          }
        } else {
          while ( sortIt != eventListSorted.end() &&
                  e->dtEnd() < Akonadi::event(*sortIt)->dtEnd() ) {
            ++sortIt;
          }
        }
      } else {
        // Keep a list of the Events without End DateTimes
        tempList.append( *eit );
      }
      eventListSorted.insert( sortIt, *eit );
    }
    if ( sortDirection == SortDirectionAscending ) {
      // Append the list of Events without End DateTimes
      eventListSorted += tempList;
    } else {
      // Prepend the list of Events without End DateTimes
      tempList += eventListSorted;
      eventListSorted = tempList;
    }
    break;

  case EventSortSummary:
    for ( eit = eventList.begin(); eit != eventList.end(); ++eit ) {
      Event::Ptr e = Akonadi::event( *eit );
      Q_ASSERT( e );
      sortIt = eventListSorted.begin();
      if ( sortDirection == SortDirectionAscending ) {
        while ( sortIt != eventListSorted.end() &&
                e->summary() >= Akonadi::event(*sortIt)->summary() ) {
          ++sortIt;
        }
      } else {
        while ( sortIt != eventListSorted.end() &&
                e->summary() < Akonadi::event(*sortIt)->summary() ) {
          ++sortIt;
        }
      }
      eventListSorted.insert( sortIt, *eit );
    }
    break;
  }

  return eventListSorted;
}

Item::List CalendarBase::events( const QDate &date,
                              const KDateTime::Spec &timeSpec,
                              EventSortField sortField,
                              SortDirection sortDirection )
{
  const Item::List el = rawEventsForDate( date, timeSpec, sortField, sortDirection );
  return Akonadi::applyCalFilter( el, d->mFilter );
}


Item::List CalendarBase::events( const KDateTime &dt )
{
  const Item::List el = rawEventsForDate( dt );
  return Akonadi::applyCalFilter( el, d->mFilter );
}


Item::List CalendarBase::events( const QDate &start, const QDate &end,
                              const KDateTime::Spec &timeSpec,
                              bool inclusive )
{
  const Item::List el = rawEvents( start, end, timeSpec, inclusive );
  return Akonadi::applyCalFilter( el, d->mFilter );
}

Item::List CalendarBase::events( EventSortField sortField,
                              SortDirection sortDirection )
{
  const Item::List el = rawEvents( sortField, sortDirection );
  return Akonadi::applyCalFilter( el, d->mFilter );
}

bool CalendarBase::addIncidence( const Incidence::Ptr &incidence )
{
  AddVisitor v( this, incidence );
  return incidence->accept( v );
}

bool CalendarBase::deleteIncidence( const Item &item )
{
  if ( !beginChange( item ) )
    return false;
  const Incidence::Ptr incidence = Akonadi::incidence( item );
  bool result = false;
  if ( incidence  ) {
    DeleteVisitor v( this, item );
    result = incidence->accept( v );
  }
  endChange( item );
  return result;
}

Incidence::Ptr CalendarBase::dissociateOccurrence( const Item &incidence,
                                           const QDate &date,
                                           const KDateTime::Spec &spec,
                                           bool single )
{
#ifdef AKONADI_PORT_DISABLED
  if ( !incidence || !incidence->recurs() ) {
    return 0;
  }

  Incidence *newInc = incidence->clone();
  newInc->recreate();
  // Do not call setRelatedTo() when dissociating recurring to-dos, otherwise the new to-do
  // will appear as a child.  Originally, we planned to set a relation with reltype SIBLING
  // when dissociating to-dos, but currently kcal only supports reltype PARENT.
  // We can uncomment the following line when we support the PARENT reltype.
  //newInc->setRelatedTo( incidence );
  Recurrence *recur = newInc->recurrence();
  if ( single ) {
    recur->clear();
  } else {
    // Adjust the recurrence for the future incidences. In particular adjust
    // the "end after n occurrences" rules! "No end date" and "end by ..."
    // don't need to be modified.
    int duration = recur->duration();
    if ( duration > 0 ) {
      int doneduration = recur->durationTo( date.addDays( -1 ) );
      if ( doneduration >= duration ) {
        kDebug() << "The dissociated event already occurred more often"
                 << "than it was supposed to ever occur. ERROR!";
        recur->clear();
      } else {
        recur->setDuration( duration - doneduration );
      }
    }
  }
  // Adjust the date of the incidence
  if ( incidence->type() == "Event" ) {
    Event *ev = static_cast<Event *>( newInc );
    KDateTime start( ev->dtStart() );
    int daysTo = start.toTimeSpec( spec ).date().daysTo( date );
    ev->setDtStart( start.addDays( daysTo ) );
    ev->setDtEnd( ev->dtEnd().addDays( daysTo ) );
  } else if ( incidence->type() == "Todo" ) {
    Todo *td = static_cast<Todo *>( newInc );
    bool haveOffset = false;
    int daysTo = 0;
    if ( td->hasDueDate() ) {
      KDateTime due( td->dtDue() );
      daysTo = due.toTimeSpec( spec ).date().daysTo( date );
      td->setDtDue( due.addDays( daysTo ), true );
      haveOffset = true;
    }
    if ( td->hasStartDate() ) {
      KDateTime start( td->dtStart() );
      if ( !haveOffset ) {
        daysTo = start.toTimeSpec( spec ).date().daysTo( date );
      }
      td->setDtStart( start.addDays( daysTo ) );
      haveOffset = true;
    }
  }
  recur = incidence->recurrence();
  if ( recur ) {
    if ( single ) {
      recur->addExDate( date );
    } else {
      // Make sure the recurrence of the past events ends
      // at the corresponding day
      recur->setEndDate( date.addDays(-1) );
    }
  }
  return newInc;
#else //AKONADI_PORT_DISABLED
  return Incidence::Ptr();
#endif // AKONADI_PORT_DISABLED
}

Item CalendarBase::incidence( const Item::Id &uid )
{
  Item i = event( uid );
  if ( i.isValid() ) {
    return i;
  }

  i = todo( uid );
  if ( i.isValid() ) {
    return i;
  }

  i = journal( uid );
  return i;
}


Item::List CalendarBase::incidencesFromSchedulingID( const QString &sid )
{
  Item::List result;
  const Item::List incidences = rawIncidences();
  Item::List::const_iterator it = incidences.begin();
  for ( ; it != incidences.end(); ++it ) {
    if ( Akonadi::incidence(*it)->schedulingID() == sid ) {
      result.append( *it );
    }
  }
  return result;
}

Item CalendarBase::incidenceFromSchedulingID( const QString &UID )
{
  const Item::List incidences = rawIncidences();
  Item::List::const_iterator it = incidences.begin();
  for ( ; it != incidences.end(); ++it ) {
    if ( Akonadi::incidence(*it)->schedulingID() == UID ) {
      // Touchdown, and the crowd goes wild
      return *it;
    }
  }
  // Not found
  return Item();
}

Item::List CalendarBase::sortTodos( const Item::List &todoList_,
                                TodoSortField sortField,
                                SortDirection sortDirection )
{
  Item::List todoList( todoList_ );
  Item::List todoListSorted;
  Item::List tempList, t;
  Item::List alphaList;
  Item::List::Iterator sortIt;
  Item::List::ConstIterator eit;

  // Notice we alphabetically presort Summaries first.
  // We do this so comparison "ties" stay in a nice order.

  // Note that To-dos may not have Start DateTimes nor due DateTimes.

  switch( sortField ) {
  case TodoSortUnsorted:
    todoListSorted = todoList;
    break;

  case TodoSortStartDate:
    alphaList = sortTodos( todoList, TodoSortSummary, sortDirection );
    for ( eit = alphaList.constBegin(); eit != alphaList.constEnd(); ++eit ) {
      const Todo::Ptr e = Akonadi::todo( *eit );
      if ( e->hasStartDate() ) {
        sortIt = todoListSorted.begin();
        if ( sortDirection == SortDirectionAscending ) {
          while ( sortIt != todoListSorted.end() &&
                  e->dtStart() >= Akonadi::todo(*sortIt)->dtStart() ) {
            ++sortIt;
          }
        } else {
          while ( sortIt != todoListSorted.end() &&
                  e->dtStart() < Akonadi::todo(*sortIt)->dtStart() ) {
            ++sortIt;
          }
        }
        todoListSorted.insert( sortIt, *eit );
      } else {
        // Keep a list of the To-dos without Start DateTimes
        tempList.append( *eit );
      }
    }
    if ( sortDirection == SortDirectionAscending ) {
      // Append the list of To-dos without Start DateTimes
      todoListSorted += tempList;
    } else {
      // Prepend the list of To-dos without Start DateTimes
      tempList += todoListSorted;
      todoListSorted = tempList;
    }
    break;

  case TodoSortDueDate:
    alphaList = sortTodos( todoList, TodoSortSummary, sortDirection );
    for ( eit = alphaList.constBegin(); eit != alphaList.constEnd(); ++eit ) {
      const Todo::Ptr e = Akonadi::todo( *eit );
      if ( e->hasDueDate() ) {
        sortIt = todoListSorted.begin();
        if ( sortDirection == SortDirectionAscending ) {
          while ( sortIt != todoListSorted.end() &&
                  e->dtDue() >= Akonadi::todo( *sortIt )->dtDue() ) {
            ++sortIt;
          }
        } else {
          while ( sortIt != todoListSorted.end() &&
                  e->dtDue() < Akonadi::todo( *sortIt )->dtDue() ) {
            ++sortIt;
          }
        }
        todoListSorted.insert( sortIt, *eit );
      } else {
        // Keep a list of the To-dos without Due DateTimes
        tempList.append( *eit );
      }
    }
    if ( sortDirection == SortDirectionAscending ) {
      // Append the list of To-dos without Due DateTimes
      todoListSorted += tempList;
    } else {
      // Prepend the list of To-dos without Due DateTimes
      tempList += todoListSorted;
      todoListSorted = tempList;
    }
    break;

  case TodoSortPriority:
    alphaList = sortTodos( todoList, TodoSortSummary, sortDirection );
    for ( eit = alphaList.constBegin(); eit != alphaList.constEnd(); ++eit ) {
      const Todo::Ptr e = Akonadi::todo( *eit );
      sortIt = todoListSorted.begin();
      if ( sortDirection == SortDirectionAscending ) {
        while ( sortIt != todoListSorted.end() &&
                e->priority() >= Akonadi::todo(*sortIt)->priority() ) {
          ++sortIt;
        }
      } else {
        while ( sortIt != todoListSorted.end() &&
                e->priority() < Akonadi::todo(*sortIt)->priority() ) {
          ++sortIt;
        }
      }
      todoListSorted.insert( sortIt, *eit );
    }
    break;

  case TodoSortPercentComplete:
    alphaList = sortTodos( todoList, TodoSortSummary, sortDirection );
    for ( eit = alphaList.constBegin(); eit != alphaList.constEnd(); ++eit ) {
      const Todo::Ptr e = Akonadi::todo( *eit );
      sortIt = todoListSorted.begin();
      if ( sortDirection == SortDirectionAscending ) {
        while ( sortIt != todoListSorted.end() &&
                e->percentComplete() >= Akonadi::todo(*sortIt)->percentComplete() ) {
          ++sortIt;
        }
      } else {
        while ( sortIt != todoListSorted.end() &&
                e->percentComplete() < Akonadi::todo(*sortIt)->percentComplete() ) {
          ++sortIt;
        }
      }
      todoListSorted.insert( sortIt, *eit );
    }
    break;

  case TodoSortSummary:
    for ( eit = todoList.constBegin(); eit != todoList.constEnd(); ++eit ) {
      const Todo::Ptr e = Akonadi::todo( *eit );
      sortIt = todoListSorted.begin();
      if ( sortDirection == SortDirectionAscending ) {
        while ( sortIt != todoListSorted.end() &&
                e->summary() >= Akonadi::todo(*sortIt)->summary() ) {
          ++sortIt;
        }
      } else {
        while ( sortIt != todoListSorted.end() &&
                e->summary() < Akonadi::todo(*sortIt)->summary() ) {
          ++sortIt;
        }
      }
      todoListSorted.insert( sortIt, *eit );
    }
    break;
  }

  return todoListSorted;
}

Item::List CalendarBase::todos( TodoSortField sortField,
                            SortDirection sortDirection )
{
  const Item::List tl = rawTodos( sortField, sortDirection );
  return Akonadi::applyCalFilter( tl, d->mFilter );
}

Item::List CalendarBase::todos( const QDate &date )
{
  Item::List el = rawTodosForDate( date );
  return Akonadi::applyCalFilter( el, d->mFilter );
}

Item::List CalendarBase::sortJournals( const Item::List &journalList_,
                                      JournalSortField sortField,
                                      SortDirection sortDirection )
{
  Item::List journalList( journalList_ );
  Item::List journalListSorted;
  Item::List::Iterator sortIt;
  Item::List::ConstIterator eit;

  switch( sortField ) {
  case JournalSortUnsorted:
    journalListSorted = journalList;
    break;

  case JournalSortDate:
    for ( eit = journalList.constBegin(); eit != journalList.constEnd(); ++eit ) {
      const Journal::Ptr e = Akonadi::journal( *eit );
      sortIt = journalListSorted.begin();
      if ( sortDirection == SortDirectionAscending ) {
        while ( sortIt != journalListSorted.end() &&
                e->dtStart() >= Akonadi::journal(*sortIt)->dtStart() ) {
          ++sortIt;
        }
      } else {
        while ( sortIt != journalListSorted.end() &&
                e->dtStart() < Akonadi::journal(*sortIt)->dtStart() ) {
          ++sortIt;
        }
      }
      journalListSorted.insert( sortIt, *eit );
    }
    break;

  case JournalSortSummary:
    for ( eit = journalList.constBegin(); eit != journalList.constEnd(); ++eit ) {
      const Journal::Ptr e = Akonadi::journal( *eit );
      sortIt = journalListSorted.begin();
      if ( sortDirection == SortDirectionAscending ) {
        while ( sortIt != journalListSorted.end() &&
                e->summary() >= Akonadi::journal(*sortIt)->summary() ) {
          ++sortIt;
        }
      } else {
        while ( sortIt != journalListSorted.end() &&
                e->summary() < Akonadi::journal(*sortIt)->summary() ) {
          ++sortIt;
        }
      }
      journalListSorted.insert( sortIt, *eit );
    }
    break;
  }

  return journalListSorted;
}

Item::List CalendarBase::journals( JournalSortField sortField,
                                  SortDirection sortDirection )
{
  const Item::List jl = rawJournals( sortField, sortDirection );
  return Akonadi::applyCalFilter( jl, d->mFilter );
}

Item::List CalendarBase::journals( const QDate &date )
{
  Item::List el = rawJournalsForDate( date );
  return Akonadi::applyCalFilter( el, d->mFilter );
}

void CalendarBase::beginBatchAdding()
{
  emit batchAddingBegins();
}

void CalendarBase::endBatchAdding()
{
  emit batchAddingEnds();
}

#ifdef AKONADI_PORT_DISABLED

void CalendarBase::setupRelations( const Item &forincidence )
{
  if ( !forincidence ) {
    return;
  }

  QString uid = forincidence->uid();

  // First, go over the list of orphans and see if this is their parent
  QList<Incidence*> l = d->mOrphans.values( uid );
  d->mOrphans.remove( uid );
  for ( int i = 0, end = l.count();  i < end;  ++i ) {
    l[i]->setRelatedTo( forincidence );
    forincidence->addRelation( l[i] );
    d->mOrphanUids.remove( l[i]->uid() );
  }

  // Now see about this incidences parent
  if ( !forincidence->relatedTo() && !forincidence->relatedToUid().isEmpty() ) {
    // Incidence has a uid it is related to but is not registered to it yet.
    // Try to find it
    Incidence *parent = incidence( forincidence->relatedToUid() );
    if ( parent ) {
      // Found it
      forincidence->setRelatedTo( parent );
      parent->addRelation( forincidence );
    } else {
      // Not found, put this in the mOrphans list
      // Note that the mOrphans dict might contain multiple entries with the
      // same key! which are multiple children that wait for the parent
      // incidence to be inserted.
      d->mOrphans.insert( forincidence->relatedToUid(), forincidence );
      d->mOrphanUids.insert( forincidence->uid(), forincidence );
    }
  }
}
#endif // AKONADI_PORT_DISABLED

#ifdef AKONADI_PORT_DISABLED
// If a to-do with sub-to-dos is deleted, move it's sub-to-dos to the orphan list
void CalendarBase::removeRelations( const Item &incidence )
{
  if ( !incidence ) {
    kDebug() << "Warning: incidence is 0";
    return;
  }

  QString uid = incidence->uid();
  foreach ( Incidence *i, incidence->relations() ) {
    if ( !d->mOrphanUids.contains( i->uid() ) ) {
      d->mOrphans.insert( uid, i );
      d->mOrphanUids.insert( i->uid(), i );
      i->setRelatedTo( 0 );
      i->setRelatedToUid( uid );
    }
  }

  // If this incidence is related to something else, tell that about it
  if ( incidence->relatedTo() ) {
    incidence->relatedTo()->removeRelation( incidence );
  }

  // Remove this one from the orphans list
  if ( d->mOrphanUids.remove( uid ) ) {
    // This incidence is located in the orphans list - it should be removed
    // Since the mOrphans dict might contain the same key (with different
    // child incidence pointers!) multiple times, take care that we remove
    // the correct one. So we need to remove all items with the given
    // parent UID, and readd those that are not for this item. Also, there
    // might be other entries with differnet UID that point to this
    // incidence (this might happen when the relatedTo of the item is
    // changed before its parent is inserted. This might happen with
    // groupware servers....). Remove them, too
    QStringList relatedToUids;

    // First, create a list of all keys in the mOrphans list which point
    // to the removed item
    relatedToUids << incidence->relatedToUid();
    for ( QMultiHash<QString, Incidence*>::Iterator it = d->mOrphans.begin();
          it != d->mOrphans.end(); ++it ) {
      if ( it.value()->uid() == uid ) {
        relatedToUids << it.key();
      }
    }

    // now go through all uids that have one entry that point to the incidence
    for ( QStringList::const_iterator uidit = relatedToUids.constBegin();
          uidit != relatedToUids.constEnd(); ++uidit ) {
      Incidence::List tempList;
      // Remove all to get access to the remaining entries
      QList<Incidence*> l = d->mOrphans.values( *uidit );
      d->mOrphans.remove( *uidit );
      foreach ( Incidence *i, l ) {
        if ( i != incidence ) {
          tempList.append( i );
        }
      }
      // Readd those that point to a different orphan incidence
      for ( Incidence::List::Iterator incit = tempList.begin();
            incit != tempList.end(); ++incit ) {
        d->mOrphans.insert( *uidit, *incit );
      }
    }
  }

  // Make sure the deleted incidence doesn't relate to a non-deleted incidence,
  // since that would cause trouble in CalendarLocal::close(), as the deleted
  // incidences are destroyed after the non-deleted incidences. The destructor
  // of the deleted incidences would then try to access the already destroyed
  // non-deleted incidence, which would segfault.
  //
  // So in short: Make sure dead incidences don't point to alive incidences
  // via the relation.
  //
  // This crash is tested in CalendarLocalTest::testRelationsCrash().
  incidence->setRelatedTo( 0 );
}
#endif // AKONADI_PORT_DISABLED

void CalendarBase::CalendarObserver::calendarModified( bool modified, CalendarBase *calendar )
{
  Q_UNUSED( modified );
  Q_UNUSED( calendar );
}

void CalendarBase::CalendarObserver::calendarIncidenceAdded( const Item &incidence )
{
  Q_UNUSED( incidence );
}

void CalendarBase::CalendarObserver::calendarIncidenceChanged( const Item &incidence )
{
  Q_UNUSED( incidence );
}

void CalendarBase::CalendarObserver::calendarIncidenceDeleted( const Item &incidence )
{
  Q_UNUSED( incidence );
}

void CalendarBase::registerObserver( CalendarObserver *observer )
{
  if ( !d->mObservers.contains( observer ) ) {
    d->mObservers.append( observer );
  }
  d->mNewObserver = true;
}

void CalendarBase::unregisterObserver( CalendarObserver *observer )
{
  d->mObservers.removeAll( observer );
}

bool CalendarBase::isSaving()
{
  return false;
}

void CalendarBase::setModified( bool modified )
{
  if ( modified != d->mModified || d->mNewObserver ) {
    d->mNewObserver = false;
    foreach ( CalendarObserver *observer, d->mObservers ) {
      observer->calendarModified( modified, this );
    }
    d->mModified = modified;
  }
}

bool CalendarBase::isModified() const
{
  return d->mModified;
}

void CalendarBase::incidenceUpdated( IncidenceBase *incidence )
{
  incidence->setLastModified( KDateTime::currentUtcDateTime() );
  // we should probably update the revision number here,
  // or internally in the Event itself when certain things change.
  // need to verify with ical documentation.

  // The static_cast is ok as the CalendarLocal only observes Incidence objects
#ifdef AKONADI_PORT_DISABLED
  notifyIncidenceChanged( static_cast<Incidence *>( incidence ) );
#endif
  setModified( true );
}

void CalendarBase::doSetTimeSpec( const KDateTime::Spec &timeSpec )
{
  Q_UNUSED( timeSpec );
}


void CalendarBase::notifyIncidenceAdded( const Item &i )
{
  if ( !d->mObserversEnabled ) {
    return;
  }

  foreach ( CalendarObserver *observer, d->mObservers ) {
    observer->calendarIncidenceAdded( i );
  }
}

void CalendarBase::notifyIncidenceChanged( const Item &i )
{
  if ( !d->mObserversEnabled ) {
    return;
  }

  foreach ( CalendarObserver *observer, d->mObservers ) {
    observer->calendarIncidenceChanged( i );
  }
}


void CalendarBase::notifyIncidenceDeleted( const Item &i )
{
  if ( !d->mObserversEnabled ) {
    return;
  }

  foreach ( CalendarObserver *observer, d->mObservers ) {
    observer->calendarIncidenceDeleted( i );
  }
}

void CalendarBase::customPropertyUpdated()
{
  setModified( true );
}

void CalendarBase::setProductId( const QString &id )
{
  d->mProductId = id;
}

QString CalendarBase::productId() const
{
  return d->mProductId;
}

Item::List CalendarBase::mergeIncidenceList( const Item::List &events,
                                              const Item::List &todos,
                                              const Item::List &journals )
{
  Item::List incidences;

  int i, end;
  for ( i = 0, end = events.count();  i < end;  ++i ) {
    incidences.append( events[i] );
  }

  for ( i = 0, end = todos.count();  i < end;  ++i ) {
    incidences.append( todos[i] );
  }

  for ( i = 0, end = journals.count();  i < end;  ++i ) {
    incidences.append( journals[i] );
  }

  return incidences;
}

bool CalendarBase::beginChange( const Item &incidence )
{
  Q_UNUSED( incidence );
  return true;
}

bool CalendarBase::endChange( const Item &incidence )
{
  Q_UNUSED( incidence );
  return true;
}

void CalendarBase::setObserversEnabled( bool enabled )
{
  d->mObserversEnabled = enabled;
}

void CalendarBase::appendAlarms( Alarm::List &alarms, const Item &item,
                             const KDateTime &from, const KDateTime &to )
{
  const Incidence::Ptr incidence = Akonadi::incidence( item );
  Q_ASSERT( incidence );

  KDateTime preTime = from.addSecs(-1);

  Alarm::List alarmlist = incidence->alarms();
  for ( int i = 0, iend = alarmlist.count();  i < iend;  ++i ) {
    if ( alarmlist[i]->enabled() ) {
      KDateTime dt = alarmlist[i]->nextRepetition( preTime );
      if ( dt.isValid() && dt <= to ) {
        kDebug() << incidence->summary() << "':" << dt.toString();
        alarms.append( alarmlist[i] );
      }
    }
  }
}

void CalendarBase::appendRecurringAlarms( Alarm::List &alarms,
                                      const Item &incidence,
                                      const KDateTime &from,
                                      const KDateTime &to )
{
#ifdef AKONADI_PORT_DISABLED
  KDateTime dt;
  bool endOffsetValid = false;
  Duration endOffset( 0 );
  Duration period( from, to );

  Alarm::List alarmlist = incidence->alarms();
  for ( int i = 0, iend = alarmlist.count();  i < iend;  ++i ) {
    Alarm *a = alarmlist[i];
    if ( a->enabled() ) {
      if ( a->hasTime() ) {
        // The alarm time is defined as an absolute date/time
        dt = a->nextRepetition( from.addSecs(-1) );
        if ( !dt.isValid() || dt > to ) {
          continue;
        }
      } else {
        // Alarm time is defined by an offset from the event start or end time.
        // Find the offset from the event start time, which is also used as the
        // offset from the recurrence time.
        Duration offset( 0 );
        if ( a->hasStartOffset() ) {
          offset = a->startOffset();
        } else if ( a->hasEndOffset() ) {
          offset = a->endOffset();
          if ( !endOffsetValid ) {
            endOffset = Duration( incidence->dtStart(), incidence->dtEnd() );
            endOffsetValid = true;
          }
        }

        // Find the incidence's earliest alarm
        KDateTime alarmStart =
          offset.end( a->hasEndOffset() ? incidence->dtEnd() : incidence->dtStart() );
//        KDateTime alarmStart = incidence->dtStart().addSecs( offset );
        if ( alarmStart > to ) {
          continue;
        }
        KDateTime baseStart = incidence->dtStart();
        if ( from > alarmStart ) {
          alarmStart = from;   // don't look earlier than the earliest alarm
          baseStart = (-offset).end( (-endOffset).end( alarmStart ) );
        }

        // Adjust the 'alarmStart' date/time and find the next recurrence at or after it.
        // Treate the two offsets separately in case one is daily and the other not.
        dt = incidence->recurrence()->getNextDateTime( baseStart.addSecs(-1) );
        if ( !dt.isValid() ||
             ( dt = endOffset.end( offset.end( dt ) ) ) > to ) // adjust 'dt' to get the alarm time
        {
          // The next recurrence is too late.
          if ( !a->repeatCount() ) {
            continue;
          }

          // The alarm has repetitions, so check whether repetitions of previous
          // recurrences fall within the time period.
          bool found = false;
          Duration alarmDuration = a->duration();
          for ( KDateTime base = baseStart;
                ( dt = incidence->recurrence()->getPreviousDateTime( base ) ).isValid();
                base = dt ) {
            if ( a->duration().end( dt ) < base ) {
              break;  // this recurrence's last repetition is too early, so give up
            }

            // The last repetition of this recurrence is at or after 'alarmStart' time.
            // Check if a repetition occurs between 'alarmStart' and 'to'.
            int snooze = a->snoozeTime().value();   // in seconds or days
            if ( a->snoozeTime().isDaily() ) {
              Duration toFromDuration( dt, base );
              int toFrom = toFromDuration.asDays();
              if ( a->snoozeTime().end( from ) <= to ||
                   ( toFromDuration.isDaily() && toFrom % snooze == 0 ) ||
                   ( toFrom / snooze + 1 ) * snooze <= toFrom + period.asDays() ) {
                found = true;
#ifndef NDEBUG
                // for debug output
                dt = offset.end( dt ).addDays( ( ( toFrom - 1 ) / snooze + 1 ) * snooze );
#endif
                break;
              }
            } else {
              int toFrom = dt.secsTo( base );
              if ( period.asSeconds() >= snooze ||
                   toFrom % snooze == 0 ||
                   ( toFrom / snooze + 1 ) * snooze <= toFrom + period.asSeconds() )
              {
                found = true;
#ifndef NDEBUG
                // for debug output
                dt = offset.end( dt ).addSecs( ( ( toFrom - 1 ) / snooze + 1 ) * snooze );
#endif
                break;
              }
            }
          }
          if ( !found ) {
            continue;
          }
        }
      }
      kDebug() << incidence->summary() << "':" << dt.toString();
      alarms.append( a );
    }
  }
#endif // AKONADI_PORT_DISABLED
}

#include "calendarbase.moc"
