/*
    This file is part of Akonadi.

    Copyright (c) 2009 KDAB
    Authors: Sebastian Sauer <sebsauer@kdab.net>
             Till Adam <till@kdab.net>

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

#ifndef AKONADICALENDAR_P_H
#define AKONADICALENDAR_P_H

#include "akonadicalendar.h"
#include "utils.h"

#include <QObject>
#include <QCoreApplication>
#include <QDBusInterface>

#include <akonadi/entity.h>
#include <akonadi/collection.h>
#include <akonadi/collectionview.h>
#include <akonadi/collectionfilterproxymodel.h>
#include <akonadi/collectionmodel.h>
#include <akonadi/collectiondialog.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/agentinstance.h>
#include <akonadi/agentmanager.h>
#include <akonadi/agenttype.h>
#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>

#include <KCal/Incidence>

#include <KLocalizedString>

using namespace boost;
using namespace KCal;
using namespace KOrg;

class AkonadiCalendarCollection : public QObject
{
    Q_OBJECT
  public:
    AkonadiCalendar *m_calendar;
    Akonadi::Collection m_collection;

    AkonadiCalendarCollection(AkonadiCalendar *calendar, const Akonadi::Collection &collection)
      : QObject()
      , m_calendar(calendar)
      , m_collection(collection)
    {
    }

    ~AkonadiCalendarCollection()
    {
    }
};

class KOrg::AkonadiCalendar::Private : public QObject
{
    Q_OBJECT
  public:
    explicit Private(AkonadiCalendar *q)
      : q(q)
      , m_monitor( new Akonadi::Monitor() )
      , m_session( new Akonadi::Session( QCoreApplication::instance()->applicationName().toUtf8() + QByteArray("-AkonadiCal-") + QByteArray::number(qrand()) ) )
      , m_incidenceBeingChanged()
    {
      m_monitor->itemFetchScope().fetchFullPayload();
      m_monitor->itemFetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::Parent );
      m_monitor->ignoreSession( m_session );

      connect( m_monitor, SIGNAL(itemChanged( const Akonadi::Item&, const QSet<QByteArray>& )),
               this, SLOT(itemChanged( const Akonadi::Item&, const QSet<QByteArray>& )) );
      connect( m_monitor, SIGNAL(itemMoved( const Akonadi::Item&, const Akonadi::Collection&, const Akonadi::Collection& )),
               this, SLOT(itemMoved( const Akonadi::Item&, const Akonadi::Collection&, const Akonadi::Collection& ) ) );
      connect( m_monitor, SIGNAL(itemAdded( const Akonadi::Item&, const Akonadi::Collection& )),
               this, SLOT(itemAdded( const Akonadi::Item& )) );
      connect( m_monitor, SIGNAL(itemRemoved( const Akonadi::Item& )),
               this, SLOT(itemRemoved( const Akonadi::Item& )) );
      /*
      connect( m_monitor, SIGNAL(itemLinked(const Akonadi::Item&, const Akonadi::Collection&)),
               this, SLOT(itemAdded(const Akonadi::Item&, const Akonadi::Collection&)) );
      connect( m_monitor, SIGNAL(itemUnlinked( const Akonadi::Item&, const Akonadi::Collection& )),
               this, SLOT(itemRemoved( const Akonadi::Item&, const Akonadi::Collection& )) );
      */
    }

    ~Private()
    {
      delete m_monitor;
      delete m_session;
    }

    /*
    void clear()
    {
      kDebug();
      mEvents.clear();
      mTodos.clear();
      mJournals.clear();
      m_map.clear();
      qDeleteAll(m_itemMap);
      qDeleteAll(m_collectionMap);
    }
    */

    bool addIncidence( Incidence *incidence )
    {
      kDebug();
      Akonadi::CollectionDialog dlg( 0 );
      dlg.setMimeTypeFilter( QStringList() << QString::fromLatin1( "text/calendar" ) );
      if ( ! dlg.exec() ) {
        return false;
      }
      const Akonadi::Collection collection = dlg.selectedCollection();
      Q_ASSERT( collection.isValid() );
      //Q_ASSERT( m_collectionMap.contains( collection.id() ) ); //we can add items to collections we don't show yet

      Akonadi::Item item;
      //the sub-mimetype of text/calendar as defined at kdepim/akonadi/kcal/kcalmimetypevisitor.cpp
      item.setMimeType( QString::fromLatin1("application/x-vnd.akonadi.calendar.%1").arg(QLatin1String(incidence->type().toLower())) );
      KCal::Incidence::Ptr incidencePtr( incidence ); //no clone() needed
      item.setPayload<KCal::Incidence::Ptr>( incidencePtr );
      Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob( item, collection, m_session );
      connect( job, SIGNAL( result( KJob* ) ), this, SLOT( createDone( KJob* ) ) );
      return true;
    }

    bool addIncidence( const Incidence::Ptr &incidence )
    {
      kDebug();
      Akonadi::CollectionDialog dlg( 0 ); //PENDING(AKONADI_PORT) we really need a parent here
      dlg.setMimeTypeFilter( QStringList() << QLatin1String( "text/calendar" ) );
      if ( ! dlg.exec() ) {
        return false;
      }
      const Akonadi::Collection collection = dlg.selectedCollection();
      Q_ASSERT( collection.isValid() );
      //Q_ASSERT( m_collectionMap.contains( collection.id() ) ); //we can add items to collections we don't show yet

      Akonadi::Item item;
      item.setPayload( incidence );
      //the sub-mimetype of text/calendar as defined at kdepim/akonadi/kcal/kcalmimetypevisitor.cpp
      item.setMimeType( QString::fromLatin1("application/x-vnd.akonadi.calendar.%1").arg(QLatin1String(incidence->type().toLower())) ); //PENDING(AKONADI_PORT) shouldn't be hardcoded?
      Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob( item, collection, m_session );
      connect( job, SIGNAL( result( KJob* ) ), this, SLOT( createDone( KJob* ) ) );
      return true;
    }

    bool deleteIncidence( const Akonadi::Item &item )
    {
      kDebug();
      m_changes.removeAll( item.id() ); //abort changes to this incidence cause we will just delete it
      Akonadi::ItemDeleteJob *job = new Akonadi::ItemDeleteJob( item, m_session );
      connect( job, SIGNAL( result( KJob* ) ), this, SLOT( deleteDone( KJob* ) ) );
      return true;
    }

    void assertInvariants() const
    {
    }

    AkonadiCalendar *q;
    Akonadi::Monitor *m_monitor;
    Akonadi::Session *m_session;
    QHash<Akonadi::Entity::Id, AkonadiCalendarCollection*> m_collectionMap;
    QHash<Akonadi::Item::Id, Akonadi::Item> m_itemMap; // akonadi id to items
    QList<Akonadi::Item::Id> m_changes; //list of item ids that are modified atm
    KCal::Incidence::Ptr m_incidenceBeingChanged; // clone of the incidence currently being modified, for rollback and to check if something actually changed

    //CalFormat *mFormat;                    // calendar format
    //QHash<QString, Event *>mEvents;        // hash on uids of all Events
    QMultiHash<QString, Akonadi::Item> m_itemsForDate;// on start dates of non-recurring, single-day Incidences

    //QMultiHash<QString, Event *>mEventsForDate;// on start dates of non-recurring, single-day Events
    //QHash<QString, Todo *>mTodos;          // hash on uids of all Todos
//QMultiHash<QString, Todo*>mTodosForDate;// on due dates for all Todos
    //QHash<QString, Journal *>mJournals;    // hash on uids of all Journals
//QMultiHash<QString, Journal *>mJournalsForDate; // on dates of all Journals
    //Incidence::List mDeletedIncidences;    // list of all deleted Incidences

  public Q_SLOTS:
  
    void listingDone( KJob *job )
    {
        kDebug();
        Akonadi::ItemFetchJob *fetchjob = static_cast<Akonadi::ItemFetchJob*>( job );
        if ( job->error() ) {
            kWarning( 5250 ) << "Item query failed:" << job->errorString();
            emit q->signalErrorMessage( job->errorString() );
            return;
        }
        itemsAdded( fetchjob->items() );
    }

    void agentCreated( KJob *job )
    {
        kDebug();
        Akonadi::AgentInstanceCreateJob *createjob = dynamic_cast<Akonadi::AgentInstanceCreateJob*>( job );
        if ( createjob->error() ) {
            kWarning( 5250 ) << "Agent create failed:" << createjob->errorString();
            emit q->signalErrorMessage( createjob->errorString() );
            return;
        }
        Akonadi::AgentInstance instance = createjob->instance();
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

    void createDone( KJob *job )
    {
        kDebug();
        if ( job->error() ) {
            kWarning( 5250 ) << "Item create failed:" << job->errorString();
            emit q->signalErrorMessage( job->errorString() );
            return;
        }
        Akonadi::ItemCreateJob *createjob = static_cast<Akonadi::ItemCreateJob*>( job );
        if ( m_collectionMap.contains( createjob->item().parentCollection().id() ) ) {
          // yes, adding to an un-viewed collection happens
          itemAdded( createjob->item() );
        } else {
          // FIXME show dialog indicating that the creation worked, but the incidence will
          // not show, since the collection isn't
          kWarning() << "Collection with id=" << createjob->item().parentCollection() << " not in m_collectionMap";
        }
    }

    void deleteDone( KJob *job )
    {
        kDebug();
        if ( job->error() ) {
            kWarning( 5250 ) << "Item delete failed:" << job->errorString();
            emit q->signalErrorMessage( job->errorString() );
            return;
        }
        Akonadi::ItemDeleteJob *deletejob = static_cast<Akonadi::ItemDeleteJob*>( job );
        itemsRemoved( deletejob->deletedItems() );
    }

    void modifyDone( KJob *job )
    {
        // we should probably update the revision number here,or internally in the Event
        // itself when certain things change. need to verify with ical documentation.

        assertInvariants();
        Akonadi::ItemModifyJob *modifyjob = static_cast<Akonadi::ItemModifyJob*>( job );
        if ( modifyjob->error() ) {
            kWarning( 5250 ) << "Item modify failed:" << job->errorString();
            emit q->signalErrorMessage( job->errorString() );
            return;
        }
        Akonadi::Item item = modifyjob->item();
        Q_ASSERT( item.hasPayload<KCal::Incidence::Ptr>() );
        const KCal::Incidence::Ptr incidence = item.payload<KCal::Incidence::Ptr>();
        Q_ASSERT( incidence );
        const Akonadi::Item::Id id = item.id();
        //kDebug()<<"Old storageCollectionId="<<m_itemMap[id]->m_item.storageCollectionId();
        kDebug() << "Item modify done item id=" << id << "storageCollectionId=" << item.storageCollectionId();
        Q_ASSERT( m_itemMap.contains(id) );
        Q_ASSERT( item.storageCollectionId() == m_itemMap.value(id).storageCollectionId() ); // there was once a bug that resulted in items forget there collectionId...
        m_itemMap.insert( id, item );
        q->notifyIncidenceChanged( item );
        q->setModified( true );
        emit q->calendarChanged();
        assertInvariants();
    }

    void itemChanged( const Akonadi::Item& item, const QSet<QByteArray>& )
    {
        assertInvariants();
        Q_ASSERT( item.isValid() );
        const Akonadi::Item::Id id = item.id();
        const Incidence::ConstPtr incidence = Akonadi::incidence( item );
        if ( !incidence )
          return;
        kDebug() << "Item changed item id=" << id << "summary=" << incidence->summary() << "type=" << incidence->type() << "storageCollectionId=" << item.storageCollectionId();
        Q_ASSERT( m_itemMap.contains(id) );
        m_itemMap.insert( id, item );
        q->notifyIncidenceChanged( item );
        q->setModified( true );
        emit q->calendarChanged();
        assertInvariants();
    }

    void itemMoved( const Akonadi::Item &item, const Akonadi::Collection& colSrc, const Akonadi::Collection& colDst )
    {
        kDebug();
        if( m_collectionMap.contains(colSrc.id()) && ! m_collectionMap.contains(colDst.id()) )
            itemRemoved( item );
        else if( m_collectionMap.contains(colDst.id()) && ! m_collectionMap.contains(colSrc.id()) )
            itemAdded( item );
    }

    void itemsAdded( const Akonadi::Item::List &items )
    {
        kDebug();
        assertInvariants();
        foreach( const Akonadi::Item &item, items ) {
            if ( !m_collectionMap.contains( item.parentCollection().id() ) )  // collection got deselected again in the meantime
              continue;
            Q_ASSERT( item.isValid() );
            if ( !Akonadi::hasIncidence( item ) )
              continue;
            const KCal::Incidence::Ptr incidence = item.payload<KCal::Incidence::Ptr>();
            kDebug() << "Add akonadi id=" << item.id() << "uid=" << incidence->uid() << "summary=" << incidence->summary() << "type=" << incidence->type();
            const Akonadi::Item::Id id = item.id();
            Q_ASSERT( ! m_itemMap.contains( id ) ); //uh, 2 incidences with the same item id?
            if( const Event::Ptr e = dynamic_pointer_cast<Event>( incidence ) ) {
              if ( !e->recurs() && !e->isMultiDay() ) {
                m_itemsForDate.insert( e->dtStart().date().toString(), item );
              }
            } else if( const Todo::Ptr t = dynamic_pointer_cast<Todo>( incidence ) ) {
              if ( t->hasDueDate() ) {
                m_itemsForDate.insert( t->dtDue().date().toString(), item );
              }
            } else if( const Journal::Ptr j = dynamic_pointer_cast<Journal>( incidence ) ) {
                m_itemsForDate.insert( j->dtStart().date().toString(), item );
            } else {
              Q_ASSERT(false);
              continue;
            }
    
            m_itemMap.insert( id, item );
            m_itemsForDate.insert( incidence->dtStart().date().toString(), item );
            assertInvariants();
            incidence->registerObserver( q );
            q->notifyIncidenceAdded( item );
        }
        q->setModified( true );
        emit q->calendarChanged();
        assertInvariants();
    }

    void itemAdded( const Akonadi::Item &item )
    {
        kDebug();
        Q_ASSERT( item.isValid() );
        if( ! m_itemMap.contains( item.id() ) ) {
          itemsAdded( Akonadi::Item::List() << item );
        }
    }

    void itemsRemoved( const Akonadi::Item::List &items )
    {
        assertInvariants();
        //kDebug()<<items.count();
        foreach(const Akonadi::Item& item, items) {
            Q_ASSERT( item.isValid() );
            Akonadi::Item ci( m_itemMap.take( item.id() ) );
            Q_ASSERT( ci.hasPayload<KCal::Incidence::Ptr>() );
            const KCal::Incidence::Ptr incidence = ci.payload<KCal::Incidence::Ptr>();
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

    void itemRemoved( const Akonadi::Item &item )
    {
        kDebug();
        itemsRemoved( Akonadi::Item::List() << item );
    }
  
};

#endif
