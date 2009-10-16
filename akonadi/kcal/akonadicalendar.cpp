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

#include "akonadicalendar.h"
#include "akonadicalendar_p.h"

#include <kcal/incidence.h>
#include <kcal/event.h>
#include <kcal/todo.h>
#include <kcal/journal.h>
#include <kcal/filestorage.h>
#include <kcal/comparisonvisitor.h>

#include <QtCore/QDate>
#include <QtCore/QHash>
#include <QtCore/QMultiHash>
#include <QtCore/QString>

#include <kdebug.h>
#include <kdatetime.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <akonadi/collection.h>
#include <akonadi/collectionview.h>
#include <akonadi/collectionfilterproxymodel.h>
#include <akonadi/collectionmodel.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/agentmanager.h>
#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>

using namespace Akonadi;
using namespace KCal;
using namespace KOrg;

AkonadiCalendar::AkonadiCalendar( const KDateTime::Spec &timeSpec )
  : KOrg::CalendarBase( timeSpec )
  , d( new AkonadiCalendar::Private(this) )
{
}

AkonadiCalendar::~AkonadiCalendar()
{
  close();
  delete d;
}

bool AkonadiCalendar::hasCollection( const Akonadi::Collection &collection ) const
{
  return d->m_collectionMap.contains( collection.id() );
}

void AkonadiCalendar::addCollection( const Akonadi::Collection &collection )
{
  kDebug();
  Q_ASSERT( ! d->m_collectionMap.contains( collection.id() ) );
  AkonadiCalendarCollection *c = new AkonadiCalendarCollection( this, collection );
  d->m_collectionMap[ collection.id() ] = c; //TODO remove again if failed!

  d->m_monitor->setCollectionMonitored( collection, true );

  // start a new job and fetch all items
  ItemFetchJob* job = new ItemFetchJob( collection, d->m_session );
  job->setFetchScope( d->m_monitor->itemFetchScope() );
  connect( job, SIGNAL(result(KJob*)), d, SLOT(listingDone(KJob*)) );
}

void AkonadiCalendar::removeCollection( const Akonadi::Collection &collection )
{
  kDebug();
  d->assertInvariants();
  if ( !d->m_collectionMap.contains( collection.id() ) )
    return;
  Q_ASSERT( d->m_collectionMap.contains( collection.id() ) );
  d->m_monitor->setCollectionMonitored( collection, false );
  AkonadiCalendarCollection *c = d->m_collectionMap.take( collection.id() );
  delete c;

  QHash<Item::Id, Item>::Iterator it( d->m_itemMap.begin() ), end( d->m_itemMap.end() );
  while( it != end) {
    if( it.value().storageCollectionId() == collection.id() ) {
      Item i = *it;
      it = d->m_itemMap.erase(it);
      Q_ASSERT( i.hasPayload<Incidence::Ptr>() );
      const Incidence::Ptr incidence = i.payload<Incidence::Ptr>();
      Q_ASSERT( incidence.get() );
    } else {
      ++it;
    }
  }
  d->assertInvariants();

  emit calendarChanged();
}

bool AkonadiCalendar::beginChange( const Item &item )
{
  const Incidence::Ptr incidence = Akonadi::incidence( item );
  Q_ASSERT( incidence );

  Q_ASSERT( ! d->m_changes.contains( item.id() ) ); //no nested changes, right?
  d->m_changes.push_back( item.id() );
  d->m_incidenceBeingChanged = Incidence::Ptr( incidence->clone() );
  return true;
}


bool AkonadiCalendar::endChange( const Item &item )
{
  if( ! item.isValid() ) {
    kWarning() << "Item is invalid";
    return false;
  }
  const Incidence::Ptr incidence = Akonadi::incidence( item );
  Q_ASSERT( incidence );

  const bool isModification = d->m_changes.removeAll( item.id() ) >= 1;

  if( ! isModification || !d->m_incidenceBeingChanged ) {
    // only if beginChange() with the incidence was called then this is a modification else it
    // is e.g. a new event/todo/journal that was not added yet or an existing one got deleted.
    kDebug() << "Skipping modify uid=" << incidence->uid() << "summary=" << incidence->summary() << "type=" << incidence->type();
    return false;
  }

  // check if there was an actual change to the incidence since beginChange
  // if not, don't kick off a modify job. The reason this is useful is that
  // begin/endChange is used for locking as well, so it is called quite often
  // without any actual changes happening. Nested modify jobs confuse the
  // conflict detection in Akonadi, so let's avoid them.
  ComparisonVisitor v;
  Incidence::Ptr incidencePtr( d->m_incidenceBeingChanged );
  d->m_incidenceBeingChanged = Incidence::Ptr();
  if ( v.compare( incidence.get(), incidencePtr.get() ) )
    return true;

  kDebug() << "modify uid=" << incidence->uid() << "summary=" << incidence->summary() << "type=" << incidence->type() << "storageCollectionId=" << item.storageCollectionId();
  ItemModifyJob *job = new ItemModifyJob( item, d->m_session );

  connect( job, SIGNAL(result( KJob*)), d, SLOT(modifyDone(KJob*)) );
  return true;
}

bool AkonadiCalendar::reload()
{
  kDebug();
#if 0
  const QString filename = d->mFileName;
  save();
  close();
  d->mFileName = filename;
  FileStorage storage( this, d->mFileName );
  return storage.load();
#else
  return true;
#endif
}

bool AkonadiCalendar::save()
{
  kDebug();
#if 0
  if ( d->mFileName.isEmpty() ) return false;
  if ( ! isModified() ) return true;
  FileStorage storage( this, d->mFileName, d->mFormat );
  return storage.save();
#else
  return true;
#endif
}

void AkonadiCalendar::close()
{
  kDebug();
  setObserversEnabled( false );
#if 0
  d->mFileName.clear();
  deleteAllEvents();
  deleteAllTodos();
  deleteAllJournals();
  d->mDeletedIncidences.clearAll();
#endif
  setModified( false );
  setObserversEnabled( true );
}

bool AkonadiCalendar::addAgent( const KUrl &url )
{
  kDebug()<< url;
  Akonadi::AgentType type = Akonadi::AgentManager::self()->type( QLatin1String("akonadi_ical_resource") );
  Akonadi::AgentInstanceCreateJob *job = new Akonadi::AgentInstanceCreateJob( type, d->m_session );
  job->setProperty("path", url.path());
  connect( job, SIGNAL( result( KJob * ) ), d, SLOT( agentCreated( KJob * ) ) );
  job->start();
  return true;
}


bool AkonadiCalendar::addIncidence( const Incidence::Ptr &incidence )
{
  kDebug();
  // dispatch to addEvent/addTodo/addJournal
  return CalendarBase::addIncidence( incidence );
}


bool AkonadiCalendar::deleteIncidence( const Item &incidence )
{
  kDebug();
  // dispatch to deleteEvent/deleteTodo/deleteJournal
  return CalendarBase::deleteIncidence( incidence );
}

// This method will be called probably multiple times if a series of changes where done. One finished the endChange() method got called.
void AkonadiCalendar::incidenceUpdated( IncidenceBase *incidence )
{
  KDateTime nowUTC = KDateTime::currentUtcDateTime();
  incidence->setLastModified( nowUTC );
  Incidence* i = dynamic_cast<Incidence*>( incidence );
  Q_ASSERT( i );
}

bool AkonadiCalendar::addEvent( const Event::Ptr &event )
{
  kDebug();
  return d->addIncidence(event);
}

// this is e.g. called by pimlibs/kcal/icalformat_p.cpp on import to replace
// existing events with newer ones. We probably like to just update in that
// case rather then to delete+create...
bool AkonadiCalendar::deleteEvent( const Item &event )
{
  kDebug();
  return d->deleteIncidence(event);
}

Item AkonadiCalendar::event( const Item::Id &id )
{
  const Item item = d->m_itemMap.value( id );
  if ( Akonadi::event( item ) )
    return item;
  else
    return Item();
}

bool AkonadiCalendar::addTodo( const Todo::Ptr &todo )
{
  kDebug();
  /*
  d->mTodos.insert( uid, todo );
  if ( todo->hasDueDate() ) {
    mTodosForDate.insert( todo->dtDue().date().toString(), todo );
  }
  todo->registerObserver( this );
  setupRelations( todo ); // Set up sub-to-do relations
  setModified( true );
  notifyIncidenceAdded( todo );
  */
  return d->addIncidence(todo);
}

bool AkonadiCalendar::deleteTodo( const Item &todo )
{
  kDebug();
  /*
  // Handle orphaned children
  removeRelations( todo );
  if ( d->mTodos.remove( todo->uid() ) ) {
    setModified( true );
    notifyIncidenceDeleted( todo );
    d->mDeletedIncidences.append( todo );
    if ( todo->hasDueDate() ) {
      removeIncidenceFromMultiHashByUID<Todo *>( d->mTodosForDate, todo->dtDue().date().toString(), todo->uid() );
    }
    return true;
  } else {
    kWarning() << "AkonadiCalendar::deleteTodo(): Todo not found.";
    return false;
  }
  */
  return d->deleteIncidence(todo);
}


Item AkonadiCalendar::todo( const Item::Id &id )
{
  const Item item = d->m_itemMap.value( id );
  if ( Akonadi::todo( item ) )
    return item;
  else
    return Item();
}

Item::List AkonadiCalendar::rawTodos( TodoSortField sortField, SortDirection sortDirection )
{
  kDebug()<<sortField<<sortDirection;
  Item::List todoList;
  QHashIterator<Item::Id, Item> i( d->m_itemMap );
  while ( i.hasNext() ) {
    i.next();
    if( Akonadi::todo( i.value() ) )
      todoList.append( i.value() );
  }
  return sortTodos( todoList, sortField, sortDirection );
}



Item::List AkonadiCalendar::rawTodosForDate( const QDate &date )
{
  kDebug()<<date.toString();
  Item::List todoList;
  QString dateStr = date.toString();
  QMultiHash<QString, Item>::const_iterator it = d->m_itemsForDate.constFind( dateStr );
  while ( it != d->m_itemsForDate.constEnd() && it.key() == dateStr ) {
    if( Akonadi::todo( it.value() ) )
      todoList.append( it.value() );
    ++it;
  }
  return todoList;
}

Akonadi::Item::List AkonadiCalendar::alarmsTo( const KDateTime &to )
{
  kDebug();
  return alarms( KDateTime( QDate( 1900, 1, 1 ) ), to );
}

Akonadi::Item::List AkonadiCalendar::alarms( const KDateTime &from, const KDateTime &to )
{
  kDebug();
  Akonadi::Item::List alarmList;
#if 0
  Alarm::List alarmList;
  QHashIterator<QString, Event *>ie( d->mEvents );
  Event *e;
  while ( ie.hasNext() ) {
    ie.next();
    e = ie.value();
    if ( e->recurs() ) appendRecurringAlarms( alarmList, e, from, to ); else appendAlarms( alarmList, e, from, to );
  }

  QHashIterator<QString, Todo *>it( d->mTodos );
  Todo *t;
  while ( it.hasNext() ) {
    it.next();
    t = it.value();
    if (! t->isCompleted() ) appendAlarms( alarmList, t, from, to );
  }
#else
  kWarning()<<"TODO";
#endif
  return alarmList;
}

Item::List AkonadiCalendar::rawEventsForDate( const QDate &date, const KDateTime::Spec &timespec, EventSortField sortField, SortDirection sortDirection )
{
  kDebug()<<date.toString();
  Item::List eventList;
  // Find the hash for the specified date
  QString dateStr = date.toString();
  // Iterate over all non-recurring, single-day events that start on this date
  QMultiHash<QString, Item>::const_iterator it = d->m_itemsForDate.constFind( dateStr );
  KDateTime::Spec ts = timespec.isValid() ? timespec : timeSpec();
  KDateTime kdt( date, ts );
  while ( it != d->m_itemsForDate.constEnd() && it.key() == dateStr ) {
    if( Event::Ptr ev = Akonadi::event( it.value() ) ) {
      KDateTime end( ev->dtEnd().toTimeSpec( ev->dtStart() ) );
      if ( ev->allDay() )
        end.setDateOnly( true ); else end = end.addSecs( -1 );
      if ( end >= kdt )
        eventList.append( it.value() );
    }
    ++it;
  }
  // Iterate over all events. Look for recurring events that occur on this date
  QHashIterator<Item::Id, Item> i( d->m_itemMap );
  while ( i.hasNext() ) {
    i.next();
    if( Event::Ptr ev = Akonadi::event( i.value() ) ) {
      if ( ev->recurs() ) {
        if ( ev->isMultiDay() ) {
          int extraDays = ev->dtStart().date().daysTo( ev->dtEnd().date() );
          for ( int j = 0; j <= extraDays; ++j ) {
            if ( ev->recursOn( date.addDays( -j ), ts ) ) {
              eventList.append( i.value() );
              break;
            }
          }
        } else {
          if ( ev->recursOn( date, ts ) )
            eventList.append( i.value() );
        }
      } else {
        if ( ev->isMultiDay() ) {
          if ( ev->dtStart().date() <= date && ev->dtEnd().date() >= date )
            eventList.append( i.value() );
        }
      }
    }
  }
  return sortEvents( eventList, sortField, sortDirection );
}

Item::List AkonadiCalendar::rawEvents( const QDate &start, const QDate &end, const KDateTime::Spec &timespec, bool inclusive )
{
  kDebug()<<start.toString()<<end.toString()<<inclusive;
  Item::List eventList;
  KDateTime::Spec ts = timespec.isValid() ? timespec : timeSpec();
  KDateTime st( start, ts );
  KDateTime nd( end, ts );
  KDateTime yesterStart = st.addDays( -1 );
  // Get non-recurring events
  QHashIterator<Item::Id, Item> i( d->m_itemMap );
  while ( i.hasNext() ) {
    i.next();
    if( Event::Ptr event = Akonadi::event( i.value() ) ) {
      KDateTime rStart = event->dtStart();
      if ( nd < rStart ) continue;
      if ( inclusive && rStart < st ) continue;
      if ( !event->recurs() ) { // non-recurring events
        KDateTime rEnd = event->dtEnd();
        if ( rEnd < st ) continue;
        if ( inclusive && nd < rEnd ) continue;
      } else { // recurring events
        switch( event->recurrence()->duration() ) {
        case -1: // infinite
          if ( inclusive ) continue;
          break;
        case 0: // end date given
        default: // count given
          KDateTime rEnd( event->recurrence()->endDate(), ts );
          if ( !rEnd.isValid() ) continue;
          if ( rEnd < st ) continue;
          if ( inclusive && nd < rEnd ) continue;
          break;
        } // switch(duration)
      } //if(recurs)
      eventList.append( i.value() );
    }
  }
  return eventList;
}

Item::List AkonadiCalendar::rawEventsForDate( const KDateTime &kdt )
{
  kDebug();
  return rawEventsForDate( kdt.date(), kdt.timeSpec() );
}

Item::List AkonadiCalendar::rawEvents( EventSortField sortField, SortDirection sortDirection )
{
  kDebug()<<sortField<<sortDirection;
  Item::List eventList;
  QHashIterator<Item::Id, Item> i( d->m_itemMap );
  while ( i.hasNext() ) {
    i.next();
    if( Akonadi::event( i.value() ) )
      eventList.append( i.value() );
  }
  return sortEvents( eventList, sortField, sortDirection );
}

bool AkonadiCalendar::addJournal( const Journal::Ptr &journal )
{
  kDebug();
  return d->addIncidence(journal);
}

bool AkonadiCalendar::deleteJournal( const Item &journal )
{
  kDebug();
  return d->deleteIncidence(journal);
}


Item AkonadiCalendar::journal( const Item::Id &id )
{
  const Item item = d->m_itemMap.value( id );
  if ( Akonadi::journal( item ) )
    return item;
  else
    return Item();
}

Item::List AkonadiCalendar::rawJournals( JournalSortField sortField, SortDirection sortDirection )
{
  kDebug()<<sortField<<sortDirection;
  Item::List journalList;
  QHashIterator<Item::Id, Item> i( d->m_itemMap );
  while ( i.hasNext() ) {
    i.next();
    if( Akonadi::journal( i.value() ) )
      journalList.append( i.value() );
  }
  return sortJournals( journalList, sortField, sortDirection );
}

Item::List AkonadiCalendar::rawJournalsForDate( const QDate &date )
{
  kDebug()<<date.toString();
  Item::List journalList;
  QString dateStr = date.toString();
  QMultiHash<QString, Item>::const_iterator it = d->m_itemsForDate.constFind( dateStr );
  while ( it != d->m_itemsForDate.constEnd() && it.key() == dateStr ) {
    if( Akonadi::journal( it.value() ) )
      journalList.append( it.value() );
    ++it;
  }
  return journalList;
}

Item AkonadiCalendar::findParent( const Item &child ) const
{
  return d->m_itemMap.value( d->m_childToParent.value( child.id() ) );
}

Item::List AkonadiCalendar::findChildren( const Item &parent ) const {
  Item::List l;
  Q_FOREACH( const Item::Id &id, d->m_parentToChildren.value( parent.id() ) )
      l.push_back( d->m_itemMap.value( id ) );
  return l;
}

bool AkonadiCalendar::isChild( const Item &parent, const Item &child ) const {
  return d->m_childToParent.value( child.id() ) == parent.id();
}

Akonadi::Item::Id AkonadiCalendar::itemIdForIncidenceUid(const QString &uid) const {
  //AKONADI_PORT_DISABLED
  //TODO implement this method. it's used in e.g. KOAlarmClient to migrate
  // previous remembered incidences to Akonadi Item's.
  return -1;
}
