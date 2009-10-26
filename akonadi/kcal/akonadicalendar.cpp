/*
    This file is part of Akonadi.

    Copyright (c) 2009 KDAB
    Author: Sebastian Sauer <sebsauer@kdab.net>
            Frank Osterfeld <frank@kdab.net>

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
#include <akonadi/agentbase.h>
#include <kcal/incidence.h>
#include <kcal/event.h>
#include <kcal/todo.h>
#include <kcal/journal.h>
#include <kcal/filestorage.h>

#include <QtCore/QDate>
#include <QtCore/QHash>
#include <QtCore/QMultiHash>
#include <QtCore/QString>
#include <QAbstractItemModel>

#include <KDebug>
#include <KDateTime>
#include <KLocale>
#include <KMessageBox>

#include <Akonadi/Collection>
#include <Akonadi/EntityTreeModel>

using namespace Akonadi;
using namespace KCal;
using namespace KOrg;

AkonadiCalendar::Private::Private( QAbstractItemModel *model, AkonadiCalendar *qq )
  : q( qq ),
    mTimeZones( new ICalTimeZones ),
    mNewObserver( false ),
    mObserversEnabled( true ),
    mDefaultFilter( new CalFilter ),
    m_model( model )
{
  // Setup default filter, which does nothing
  mDefaultFilter->setEnabled( false );
  m_filterProxy = new CalFilterProxyModel( q );
  m_filterProxy->setSourceModel( m_model );
  m_filterProxy->setFilter( mDefaultFilter );


  // user information...
  mOwner.setName( i18n( "Unknown Name" ) );
  mOwner.setEmail( i18n( "unknown@nowhere" ) );

  connect( m_model, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(dataChanged(QModelIndex,QModelIndex)) );
  connect( m_model, SIGNAL(layoutChanged()), this, SLOT(layoutChanged()) );
  connect( m_model, SIGNAL(modelReset()), this, SLOT(modelReset()) );
  connect( m_model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(rowsInserted(QModelIndex,int,int)) );
  connect( m_model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(rowsAboutToBeRemoved(QModelIndex,int,int)) );

  /*
  connect( m_monitor, SIGNAL(itemLinked(const Akonadi::Item,Akonadi::Collection)),
           this, SLOT(itemAdded(const Akonadi::Item,Akonadi::Collection)) );
  connect( m_monitor, SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection )),
           this, SLOT(itemRemoved(Akonadi::Item,Akonadi::Collection )) );
  */
}


void AkonadiCalendar::Private::rowsInserted( const QModelIndex&, int start, int end ) {
  itemsAdded( itemsFromModel( m_model, start, end ) );
}

void AkonadiCalendar::Private::rowsAboutToBeRemoved( const QModelIndex&, int start, int end ) {
  itemsRemoved( itemsFromModel( m_model, start, end ) );
}

void AkonadiCalendar::Private::layoutChanged() {

}

void AkonadiCalendar::Private::modelReset() {
  clear();
  readFromModel();
}

void AkonadiCalendar::Private::clear() {
  itemsRemoved( m_itemMap.values() );
  Q_ASSERT( m_itemMap.isEmpty() );
  m_childToParent.clear();
  m_parentToChildren.clear();
  m_childToUnseenParent.clear();
  m_unseenParentToChildren.clear();
  m_itemsForDate.clear();
}

void AkonadiCalendar::Private::readFromModel() {
  itemsAdded( itemsFromModel( m_model ) );
}

void AkonadiCalendar::Private::dataChanged( const QModelIndex& topLeft, const QModelIndex& bottomRight ) {
  Q_ASSERT( topLeft.row() <= bottomRight.row() );
  const int endRow = bottomRight.row();
  QModelIndex i( topLeft );
  int row = i.row();
  while ( row <= endRow ) {
    updateItem( itemFromIndex( i ), AssertExists );
    ++row;
    i = i.sibling( row, topLeft.column() );
  }
}


AkonadiCalendar::Private::~Private()
{
  delete mTimeZones;
  delete mDefaultFilter;
}


void AkonadiCalendar::Private::assertInvariants() const
{
}

void AkonadiCalendar::Private::updateItem( const Item &item, UpdateMode mode ) {
  assertInvariants();
  const bool alreadyExisted = m_itemMap.contains( item.id() );
  const Item::Id id = item.id();

  kDebug()<<item.id()<<alreadyExisted;
  Q_ASSERT( mode == DontCare || alreadyExisted == ( mode == AssertExists ) );

  if ( alreadyExisted ) {
    Q_ASSERT( item.storageCollectionId() == m_itemMap.value( id ).storageCollectionId() ); // there was once a bug that resulted in items forget their collectionId...
    // update-only goes here
  } else {
    // new-only goes here
  }

  QDate typeSpecificDate;
  if( const Event::Ptr e = Akonadi::event( item ) ) {
    if ( !e->recurs() && !e->isMultiDay() ) {
      typeSpecificDate = e->dtStart().date();
    }
  } else if( const Todo::Ptr t = Akonadi::todo( item ) ) {
    if ( t->hasDueDate() ) {
      typeSpecificDate = t->dtDue().date();
    }
  } else if( const Journal::Ptr j = Akonadi::journal( item ) ) {
      typeSpecificDate = j->dtStart().date();
  } else {
    Q_ASSERT( false );
    return;
  }

  const Incidence::Ptr incidence = Akonadi::incidence( item );
  Q_ASSERT( incidence );

  if ( alreadyExisted ) {
    //TODO(AKONADI_PORT): for changed items, we should remove existing date entries (they might have changed)
  }

  if ( typeSpecificDate.isValid() )
    m_itemsForDate.insert( typeSpecificDate.toString(), item );
  m_itemsForDate.insert( incidence->dtStart().date().toString(), item );

  m_itemMap.insert( id, item );

  UnseenItem ui;
  ui.collection = item.storageCollectionId();
  ui.uid = incidence->uid();

  //REVIEW(AKONADI_PORT)
  //UIDs might be duplicated and thus not unique, so for now we assume that the relatedTo
  // UID refers to an item in the same collection.
  //this might break with virtual collections, so we might fall back to a global UID
  //to akonadi item mapping, and pick just any item (or the first found, or whatever strategy makes sense)
  //from the ones with the same UID
  const QString parentUID = incidence->relatedToUid();
  const bool hasParent = !parentUID.isEmpty();
  UnseenItem parentItem;
  QMap<UnseenItem,Item::Id>::const_iterator parentIt = m_uidToItemId.constEnd();
  bool knowParent = false;
  bool parentNotChanged = false;
  if ( hasParent ) {
    parentItem.collection = item.storageCollectionId();
    parentItem.uid = parentUID;
    QMap<UnseenItem,Item::Id>::const_iterator parentIt = m_uidToItemId.constFind( parentItem );
    knowParent = parentIt != m_uidToItemId.constEnd();
  }

  if ( alreadyExisted ) {
    Q_ASSERT( m_uidToItemId.value( ui ) == item.id() );
    QHash<Item::Id,Item::Id>::Iterator oldParentIt = m_childToParent.find( id );
    if ( oldParentIt != m_childToParent.constEnd() ) {
      const Incidence::Ptr parentInc = Akonadi::incidence( m_itemMap.value( oldParentIt.value() ) );
      Q_ASSERT( parentInc );
      if ( parentInc->uid() != parentUID ) {
        //parent changed, remove old entries
        Akonadi::incidence( item )->setRelatedTo( 0 );
        QVector<Item::Id>& l = m_parentToChildren[oldParentIt.value()];
        l.erase( std::remove( l.begin(), l.end(), id ), l.end() );
        m_childToParent.remove( id );
      } else
        parentNotChanged = true;
    } else { //old parent not seen, maybe unseen?
      QHash<Item::Id,UnseenItem>::Iterator oldUnseenParentIt = m_childToUnseenParent.find( id );
      if ( oldUnseenParentIt != m_childToUnseenParent.constEnd() ) {
        if ( oldUnseenParentIt.value().uid != parentUID ) {
          //parent changed, remove old entries
          QVector<Item::Id>& l = m_unseenParentToChildren[oldUnseenParentIt.value()];
          l.erase( std::remove( l.begin(), l.end(), id ), l.end() );
          m_childToUnseenParent.remove( id );
        }
        else
          parentNotChanged = true;
      }
    }

  } else {
    m_uidToItemId.insert( ui, item.id() );

    //check for already known children:
    const QVector<Item::Id> orphanedChildren = m_unseenParentToChildren.value( ui );
    if ( !orphanedChildren.isEmpty() )
      m_parentToChildren.insert( id, orphanedChildren );
    Q_FOREACH ( const Item::Id &cid, orphanedChildren )
      m_childToParent.insert( cid, id );
    m_unseenParentToChildren.remove( ui );
    m_childToUnseenParent.remove( id );
  }

  if ( hasParent && !parentNotChanged ) {
    if ( knowParent ) {
      Q_ASSERT( !m_parentToChildren.value( parentIt.value() ).contains( id ) );
      const Incidence::Ptr parentInc = Akonadi::incidence( m_itemMap.value( parentIt.value() ) );
      Q_ASSERT( parentInc );
      Akonadi::incidence( item )->setRelatedTo( parentInc.get() );
      m_parentToChildren[parentIt.value()].push_back( id );
      m_childToParent.insert( id, parentIt.value() );
    } else {
      m_childToUnseenParent.insert( id, parentItem );
      m_unseenParentToChildren[parentItem].push_back( id );
    }
  }

  if ( !alreadyExisted ) {
    incidence->registerObserver( q );
    q->notifyIncidenceAdded( item );
  } else {
    q->notifyIncidenceChanged( item );
  }
  assertInvariants();
}

void AkonadiCalendar::Private::itemChanged( const Item& item )
{
  assertInvariants();
  Q_ASSERT( item.isValid() );
  const Incidence::ConstPtr incidence = Akonadi::incidence( item );
  if ( !incidence )
    return;
  updateItem( item, AssertExists );
  emit q->calendarChanged();
  assertInvariants();
}

void AkonadiCalendar::Private::itemsAdded( const Item::List &items )
{
    kDebug() << "adding items: " << items.count();
    assertInvariants();
    foreach( const Item &item, items ) {
        Q_ASSERT( item.isValid() );
        if ( !Akonadi::hasIncidence( item ) )
          continue;
        updateItem( item, AssertNew );
        const Incidence::Ptr incidence = item.payload<Incidence::Ptr>();
        kDebug() << "Add akonadi id=" << item.id() << "uid=" << incidence->uid() << "summary=" << incidence->summary() << "type=" << incidence->type();

        incidence->registerObserver( q );
        q->notifyIncidenceAdded( item );
    }
    emit q->calendarChanged();
    assertInvariants();
}

void AkonadiCalendar::Private::itemsRemoved( const Item::List &items )
{
    assertInvariants();
    //kDebug()<<items.count();
    foreach(const Item& item, items) {
        Q_ASSERT( item.isValid() );
        Item ci( m_itemMap.take( item.id() ) );
        kDebug()<<item.id();
        Q_ASSERT( ci.hasPayload<Incidence::Ptr>() );
        const Incidence::Ptr incidence = ci.payload<Incidence::Ptr>();
        kDebug() << "Remove uid=" << incidence->uid() << "summary=" << incidence->summary() << "type=" << incidence->type();

        if( const Event::Ptr e = dynamic_pointer_cast<Event>(incidence) ) {
        if ( !e->recurs() ) {
            m_itemsForDate.remove( e->dtStart().date().toString(), item );
        }
        } else if( const Todo::Ptr t = dynamic_pointer_cast<Todo>( incidence ) ) {
          if ( t->hasDueDate() ) {
            m_itemsForDate.remove( t->dtDue().date().toString(), item );
          }
        } else if( const Journal::Ptr j = dynamic_pointer_cast<Journal>( incidence ) ) {
          m_itemsForDate.remove( j->dtStart().date().toString(), item );
        } else {
          Q_ASSERT(false);
          continue;
        }

        //incidence->unregisterObserver( q );
        q->notifyIncidenceDeleted( item );
    }
    emit q->calendarChanged();
    assertInvariants();
}

AkonadiCalendar::AkonadiCalendar( QAbstractItemModel *model, const KDateTime::Spec &timeSpec, QObject *parent )
  : QObject( parent )
  , d( new Private( model, this ) )
{
  d->mTimeSpec = timeSpec;
  d->mViewTimeSpec = timeSpec;
  d->readFromModel();
}

AkonadiCalendar::~AkonadiCalendar()
{
  delete d;
}

QAbstractItemModel* AkonadiCalendar::model() const {
  return d->m_filterProxy;
}

QAbstractItemModel* AkonadiCalendar::unfilteredModel() const {
  return d->m_model;
}

// This method will be called probably multiple times if a series of changes where done. One finished the endChange() method got called.
void AkonadiCalendar::incidenceUpdated( IncidenceBase *incidence )
{
  incidence->setLastModified( KDateTime::currentUtcDateTime() );
  // we should probably update the revision number here,
  // or internally in the Event itself when certain things change.
  // need to verify with ical documentation.

  // The static_cast is ok as the CalendarLocal only observes Incidence objects
#ifdef AKONADI_PORT_DISABLED
  notifyIncidenceChanged( static_cast<Incidence *>( incidence ) );
#endif
}

Item AkonadiCalendar::event( const Item::Id &id )
{
  const Item item = d->m_itemMap.value( id );
  if ( Akonadi::event( item ) )
    return item;
  else
    return Item();
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

Item::List AkonadiCalendar::alarmsTo( const KDateTime &to )
{
  kDebug();
  return alarms( KDateTime( QDate( 1900, 1, 1 ) ), to );
}

Item::List AkonadiCalendar::alarms( const KDateTime &from, const KDateTime &to )
{
  kDebug();
  Item::List alarmList;
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

Item::Id AkonadiCalendar::itemIdForIncidenceUid(const QString &uid) const {
  //AKONADI_PORT_DISABLED
  //TODO implement this method. it's used in e.g. KOAlarmClient to migrate
  // previous remembered incidences to Akonadi Item's.
  return -1;
}


// calendarbase.cpp


Person AkonadiCalendar::owner() const
{
  return d->mOwner;
}

void AkonadiCalendar::setOwner( const Person &owner )
{
  d->mOwner = owner;
}

void AkonadiCalendar::setTimeSpec( const KDateTime::Spec &timeSpec )
{
  d->mTimeSpec = timeSpec;
  d->mBuiltInTimeZone = ICalTimeZone();
  setViewTimeSpec( timeSpec );

  doSetTimeSpec( d->mTimeSpec );
}

KDateTime::Spec AkonadiCalendar::timeSpec() const
{
  return d->mTimeSpec;
}

void AkonadiCalendar::setTimeZoneId( const QString &timeZoneId )
{
  d->mTimeSpec = d->timeZoneIdSpec( timeZoneId, false );
  d->mViewTimeSpec = d->mTimeSpec;
  d->mBuiltInViewTimeZone = d->mBuiltInTimeZone;

  doSetTimeSpec( d->mTimeSpec );
}

//@cond PRIVATE
KDateTime::Spec AkonadiCalendar::Private::timeZoneIdSpec( const QString &timeZoneId,
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

QString AkonadiCalendar::timeZoneId() const
{
  KTimeZone tz = d->mTimeSpec.timeZone();
  return tz.isValid() ? tz.name() : QString();
}

void AkonadiCalendar::setViewTimeSpec( const KDateTime::Spec &timeSpec ) const
{
  d->mViewTimeSpec = timeSpec;
  d->mBuiltInViewTimeZone = ICalTimeZone();
}

void AkonadiCalendar::setViewTimeZoneId( const QString &timeZoneId ) const
{
  d->mViewTimeSpec = d->timeZoneIdSpec( timeZoneId, true );
}

KDateTime::Spec AkonadiCalendar::viewTimeSpec() const
{
  return d->mViewTimeSpec;
}

QString AkonadiCalendar::viewTimeZoneId() const
{
  KTimeZone tz = d->mViewTimeSpec.timeZone();
  return tz.isValid() ? tz.name() : QString();
}

void AkonadiCalendar::shiftTimes( const KDateTime::Spec &oldSpec,
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

void AkonadiCalendar::setFilter( CalFilter *filter )
{
  d->m_filterProxy->setFilter( filter ? filter : d->mDefaultFilter );
}

CalFilter *AkonadiCalendar::filter()
{
  return d->m_filterProxy->filter();
}

QStringList AkonadiCalendar::categories( AkonadiCalendar* cal )
{
  Item::List rawInc( cal->rawIncidences() );
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

Item::List AkonadiCalendar::incidences( const QDate &date )
{
  return mergeIncidenceList( events( date ), todos( date ), journals( date ) );
}

Item::List AkonadiCalendar::incidences()
{
  return itemsFromModel( d->m_filterProxy );
}

Item::List AkonadiCalendar::rawIncidences()
{
  return itemsFromModel( d->m_model );
}

Item::List AkonadiCalendar::sortEvents( const Item::List &eventList_,
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

Item::List AkonadiCalendar::events( const QDate &date,
                              const KDateTime::Spec &timeSpec,
                              EventSortField sortField,
                              SortDirection sortDirection )
{
  const Item::List el = rawEventsForDate( date, timeSpec, sortField, sortDirection );
  return Akonadi::applyCalFilter( el, filter() );
}


Item::List AkonadiCalendar::events( const KDateTime &dt )
{
  const Item::List el = rawEventsForDate( dt );
  return Akonadi::applyCalFilter( el, filter() );
}


Item::List AkonadiCalendar::events( const QDate &start, const QDate &end,
                              const KDateTime::Spec &timeSpec,
                              bool inclusive )
{
  const Item::List el = rawEvents( start, end, timeSpec, inclusive );
  return Akonadi::applyCalFilter( el, filter() );
}

Item::List AkonadiCalendar::events( EventSortField sortField,
                              SortDirection sortDirection )
{
  const Item::List el = rawEvents( sortField, sortDirection );
  return Akonadi::applyCalFilter( el, filter() );
}

Incidence::Ptr AkonadiCalendar::dissociateOccurrence( const Item &incidence,
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

Item AkonadiCalendar::incidence( const Item::Id &uid )
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


Item::List AkonadiCalendar::incidencesFromSchedulingID( const QString &sid )
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

Item AkonadiCalendar::incidenceFromSchedulingID( const QString &UID )
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

Item::List AkonadiCalendar::sortTodos( const Item::List &todoList_,
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

Item::List AkonadiCalendar::todos( TodoSortField sortField,
                            SortDirection sortDirection )
{
  const Item::List tl = rawTodos( sortField, sortDirection );
  return Akonadi::applyCalFilter( tl, filter() );
}

Item::List AkonadiCalendar::todos( const QDate &date )
{
  Item::List el = rawTodosForDate( date );
  return Akonadi::applyCalFilter( el, filter() );
}

Item::List AkonadiCalendar::sortJournals( const Item::List &journalList_,
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

Item::List AkonadiCalendar::journals( JournalSortField sortField,
                                  SortDirection sortDirection )
{
  const Item::List jl = rawJournals( sortField, sortDirection );
  return Akonadi::applyCalFilter( jl, filter() );
}

Item::List AkonadiCalendar::journals( const QDate &date )
{
  Item::List el = rawJournalsForDate( date );
  return Akonadi::applyCalFilter( el, filter() );
}

void AkonadiCalendar::beginBatchAdding()
{
  emit batchAddingBegins();
}

void AkonadiCalendar::endBatchAdding()
{
  emit batchAddingEnds();
}

#ifdef AKONADI_PORT_DISABLED

void AkonadiCalendar::setupRelations( const Item &forincidence )
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
void AkonadiCalendar::removeRelations( const Item &incidence )
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


void AkonadiCalendar::CalendarObserver::calendarIncidenceAdded( const Item &incidence )
{
  Q_UNUSED( incidence );
}

void AkonadiCalendar::CalendarObserver::calendarIncidenceChanged( const Item &incidence )
{
  Q_UNUSED( incidence );
}

void AkonadiCalendar::CalendarObserver::calendarIncidenceDeleted( const Item &incidence )
{
  Q_UNUSED( incidence );
}

void AkonadiCalendar::registerObserver( CalendarObserver *observer )
{
  if ( !d->mObservers.contains( observer ) ) {
    d->mObservers.append( observer );
  }
  d->mNewObserver = true;
}

void AkonadiCalendar::unregisterObserver( CalendarObserver *observer )
{
  d->mObservers.removeAll( observer );
}


void AkonadiCalendar::doSetTimeSpec( const KDateTime::Spec &timeSpec )
{
  Q_UNUSED( timeSpec );
}


void AkonadiCalendar::notifyIncidenceAdded( const Item &i )
{
  if ( !d->mObserversEnabled ) {
    return;
  }

  foreach ( CalendarObserver *observer, d->mObservers ) {
    observer->calendarIncidenceAdded( i );
  }
}

void AkonadiCalendar::notifyIncidenceChanged( const Item &i )
{
  if ( !d->mObserversEnabled ) {
    return;
  }

  foreach ( CalendarObserver *observer, d->mObservers ) {
    observer->calendarIncidenceChanged( i );
  }
}


void AkonadiCalendar::notifyIncidenceDeleted( const Item &i )
{
  if ( !d->mObserversEnabled ) {
    return;
  }

  foreach ( CalendarObserver *observer, d->mObservers ) {
    observer->calendarIncidenceDeleted( i );
  }
}

void AkonadiCalendar::customPropertyUpdated()
{
}

void AkonadiCalendar::setProductId( const QString &id )
{
  d->mProductId = id;
}

QString AkonadiCalendar::productId() const
{
  return d->mProductId;
}

Item::List AkonadiCalendar::mergeIncidenceList( const Item::List &events,
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

void AkonadiCalendar::setObserversEnabled( bool enabled )
{
  d->mObserversEnabled = enabled;
}

void AkonadiCalendar::appendAlarms( Alarm::List &alarms, const Item &item,
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

void AkonadiCalendar::appendRecurringAlarms( Alarm::List &alarms,
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
