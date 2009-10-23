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

AkonadiCalendar::Private::Private( QAbstractItemModel *model, AkonadiCalendar *q )
  : q( q )
  , m_model( model )
  , m_monitor( new Akonadi::Monitor() )
  , m_session( new Akonadi::Session( QCoreApplication::instance()->applicationName().toUtf8() + QByteArray("-AkonadiCal-") + QByteArray::number(qrand()) ) )
{
  m_monitor->itemFetchScope().fetchFullPayload();
  m_monitor->itemFetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );

  connect( m_monitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray> )),
           this, SLOT(itemChanged(Akonadi::Item,QSet<QByteArray> )) );
  connect( m_monitor, SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection )),
           this, SLOT(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection ) ) );
  connect( m_monitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection )),
           this, SLOT(itemAdded(Akonadi::Item )) );
  connect( m_monitor, SIGNAL(itemRemoved(Akonadi::Item )),
           this, SLOT(itemRemoved(Akonadi::Item )) );
  /*
  connect( m_monitor, SIGNAL(itemLinked(const Akonadi::Item,Akonadi::Collection)),
           this, SLOT(itemAdded(const Akonadi::Item,Akonadi::Collection)) );
  connect( m_monitor, SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection )),
           this, SLOT(itemRemoved(Akonadi::Item,Akonadi::Collection )) );
  */
}

AkonadiCalendar::Private::~Private()
{
  delete m_monitor;
  delete m_session;
}


void AkonadiCalendar::Private::assertInvariants() const
{
}

void AkonadiCalendar::Private::updateItem( const Item &item, UpdateMode mode ) {
  assertInvariants();
  const bool alreadyExisted = m_itemMap.contains( item.id() );
  const Item::Id id = item.id();

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
  }
  assertInvariants();
}

void AkonadiCalendar::Private::listingDone( KJob *job )
{
    kDebug();
    ItemFetchJob *fetchjob = static_cast<ItemFetchJob*>( job );
    if ( job->error() ) {
        kWarning( 5250 ) << "Item query failed:" << job->errorString();
        emit q->signalErrorMessage( job->errorString() );
        return;
    }
    itemsAdded( fetchjob->items() );
}

void AkonadiCalendar::Private::agentCreated( KJob *job )
{
    kDebug();
    AgentInstanceCreateJob *createjob = dynamic_cast<AgentInstanceCreateJob*>( job );
    if ( createjob->error() ) {
        kWarning( 5250 ) << "Agent create failed:" << createjob->errorString();
        emit q->signalErrorMessage( createjob->errorString() );
        return;
    }
    AgentInstance instance = createjob->instance();
    //instance.setName( CalendarName );
    QDBusInterface iface( QString::fromLatin1("org.freedesktop.Akonadi.Resource.%1").arg( instance.identifier() ), QLatin1String("/Settings") );
    if( ! iface.isValid() ) {
        kWarning( 5250 ) << "Failed to obtain D-Bus interface for remote configuration.";
        emit q->signalErrorMessage( i18n("Failed to obtain D-Bus interface for remote configuration.") );
        return;
    }
    QString path = createjob->property( "path" ).toString();
    Q_ASSERT( ! path.isEmpty() );
    iface.call(QLatin1String("setPath"), path);
    instance.reconfigure();
}

void AkonadiCalendar::Private::itemChanged( const Item& item )
{
  assertInvariants();
  Q_ASSERT( item.isValid() );
  const Incidence::ConstPtr incidence = Akonadi::incidence( item );
  if ( !incidence )
    return;
  updateItem( item, AssertExists );
  q->notifyIncidenceChanged( item );
  q->setModified( true );
  emit q->calendarChanged();
  assertInvariants();
}

void AkonadiCalendar::Private::itemChanged( const Item& item, const QSet<QByteArray>& )
{
  itemChanged( item );
}

void AkonadiCalendar::Private::itemMoved( const Item &item, const Collection& colSrc, const Collection& colDst )
{
    kDebug();
    if( m_collectionMap.contains(colSrc.id()) && ! m_collectionMap.contains(colDst.id()) )
        itemRemoved( item );
    else if( m_collectionMap.contains(colDst.id()) && ! m_collectionMap.contains(colSrc.id()) )
        itemAdded( item );
}

void AkonadiCalendar::Private::itemsAdded( const Item::List &items )
{
    kDebug();
    assertInvariants();
    foreach( const Item &item, items ) {
        if ( !m_collectionMap.contains( item.parentCollection().id() ) )  // collection got deselected again in the meantime
          continue;
        Q_ASSERT( item.isValid() );
        if ( !Akonadi::hasIncidence( item ) )
          continue;
        updateItem( item, AssertNew );
        const Incidence::Ptr incidence = item.payload<Incidence::Ptr>();
        kDebug() << "Add akonadi id=" << item.id() << "uid=" << incidence->uid() << "summary=" << incidence->summary() << "type=" << incidence->type();

        incidence->registerObserver( q );
        q->notifyIncidenceAdded( item );
    }
    q->setModified( true );
    emit q->calendarChanged();
    assertInvariants();
}

void AkonadiCalendar::Private::itemAdded( const Item &item )
{
    kDebug();
    Q_ASSERT( item.isValid() );
    if( ! m_itemMap.contains( item.id() ) ) {
      itemsAdded( Item::List() << item );
    }
}

void AkonadiCalendar::Private::itemsRemoved( const Item::List &items )
{
    assertInvariants();
    //kDebug()<<items.count();
    foreach(const Item& item, items) {
        Q_ASSERT( item.isValid() );
        Item ci( m_itemMap.take( item.id() ) );
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
    q->setModified( true );
    emit q->calendarChanged();
    assertInvariants();
}

void AkonadiCalendar::Private::itemRemoved( const Item &item )
{
    kDebug();
    itemsRemoved( Item::List() << item );
}


AkonadiCalendar::AkonadiCalendar( QAbstractItemModel *model, const KDateTime::Spec &timeSpec )
  : KOrg::CalendarBase( timeSpec )
  , d( new AkonadiCalendar::Private( model, this ) )
{
}

AkonadiCalendar::~AkonadiCalendar()
{
  delete d;
}

bool AkonadiCalendar::hasCollection( const Collection &collection ) const
{
  return d->m_collectionMap.contains( collection.id() );
}

void AkonadiCalendar::addCollection( const Collection &collection )
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

void AkonadiCalendar::removeCollection( const Collection &collection )
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


bool AkonadiCalendar::addAgent( const KUrl &url )
{
  kDebug()<< url;
  AgentType type = AgentManager::self()->type( QLatin1String("akonadi_ical_resource") );
  AgentInstanceCreateJob *job = new AgentInstanceCreateJob( type, d->m_session );
  job->setProperty("path", url.path());
  connect( job, SIGNAL( result( KJob * ) ), d, SLOT( agentCreated( KJob * ) ) );
  job->start();
  return true;
}

// This method will be called probably multiple times if a series of changes where done. One finished the endChange() method got called.
void AkonadiCalendar::incidenceUpdated( IncidenceBase *incidence )
{
  KDateTime nowUTC = KDateTime::currentUtcDateTime();
  incidence->setLastModified( nowUTC );
  Incidence* i = dynamic_cast<Incidence*>( incidence );
  Q_ASSERT( i );
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
