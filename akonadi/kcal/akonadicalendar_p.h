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
#include <QtCore/QMap>

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

namespace KOrg {
  struct UnseenItem {
    Akonadi::Entity::Id collection;
    QString uid;

    bool operator<( const UnseenItem &other ) const {
      if ( collection != other.collection )
        return collection < other.collection;
      return uid < other.uid;
    }
  };
}

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
      Akonadi::CollectionDialog dlg;
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
      Akonadi::CollectionDialog dlg; //PENDING(AKONADI_PORT) we really need a parent here
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

    QHash<Akonadi::Item::Id, Akonadi::Item::Id> m_childToParent; // child to parent map, for already cached parents
    QHash<Akonadi::Item::Id, QVector<Akonadi::Item::Id> > m_parentToChildren; //parent to children map, for alread cached children
    QMap<UnseenItem, Akonadi::Item::Id> m_uidToItemId;

    QHash<Akonadi::Item::Id, UnseenItem> m_childToUnseenParent; // child to parent map, unknown/not cached parent items
    QMap<UnseenItem, QVector<Akonadi::Item::Id> > m_unseenParentToChildren;

    QList<Akonadi::Item::Id> m_changes; //list of item ids that are modified atm
    KCal::Incidence::Ptr m_incidenceBeingChanged; // clone of the incidence currently being modified, for rollback and to check if something actually changed

    QMultiHash<QString, Akonadi::Item> m_itemsForDate;// on start dates of non-recurring, single-day Incidences


    enum UpdateMode {
      DontCare,
      AssertExists,
      AssertNew
    };

    void updateItem( const Akonadi::Item &item, UpdateMode mode ) {
      assertInvariants();
      const bool alreadyExisted = m_itemMap.contains( item.id() );
      const Akonadi::Item::Id id = item.id();

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

      const KCal::Incidence::Ptr incidence = Akonadi::incidence( item );
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
      QMap<UnseenItem,Akonadi::Item::Id>::const_iterator parentIt = m_uidToItemId.constEnd();
      bool knowParent = false;
      bool parentNotChanged = false;
      if ( hasParent ) {
        parentItem.collection = item.storageCollectionId();
        parentItem.uid = parentUID;
        QMap<UnseenItem,Akonadi::Item::Id>::const_iterator parentIt = m_uidToItemId.constFind( parentItem );
        knowParent = parentIt != m_uidToItemId.constEnd();
      }

      if ( alreadyExisted ) {
        Q_ASSERT( m_uidToItemId.value( ui ) == item.id() );
        QHash<Akonadi::Item::Id,Akonadi::Item::Id>::Iterator oldParentIt = m_childToParent.find( id );
        if ( oldParentIt != m_childToParent.constEnd() ) {
          const Incidence::Ptr parentInc = Akonadi::incidence( m_itemMap.value( oldParentIt.value() ) );
          Q_ASSERT( parentInc );
          if ( parentInc->uid() != parentUID ) {
            //parent changed, remove old entries
            Akonadi::incidence( item )->setRelatedTo( 0 );
            QVector<Akonadi::Item::Id>& l = m_parentToChildren[oldParentIt.value()];
            l.erase( std::remove( l.begin(), l.end(), id ), l.end() );
            m_childToParent.remove( id );
          } else
            parentNotChanged = true;
        } else { //old parent not seen, maybe unseen?
          QHash<Akonadi::Item::Id,UnseenItem>::Iterator oldUnseenParentIt = m_childToUnseenParent.find( id );
          if ( oldUnseenParentIt != m_childToUnseenParent.constEnd() ) {
            if ( oldUnseenParentIt.value().uid != parentUID ) {
              //parent changed, remove old entries
              QVector<Akonadi::Item::Id>& l = m_unseenParentToChildren[oldUnseenParentIt.value()];
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
        const QVector<Akonadi::Item::Id> orphanedChildren = m_unseenParentToChildren.value( ui );
        if ( !orphanedChildren.isEmpty() )
          m_parentToChildren.insert( id, orphanedChildren );
        Q_FOREACH ( const Akonadi::Item::Id &cid, orphanedChildren )
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
        updateItem( item, AssertExists );
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
        updateItem( item, AssertExists );
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
            updateItem( item, AssertNew );
            const KCal::Incidence::Ptr incidence = item.payload<KCal::Incidence::Ptr>();
            kDebug() << "Add akonadi id=" << item.id() << "uid=" << incidence->uid() << "summary=" << incidence->summary() << "type=" << incidence->type();

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
