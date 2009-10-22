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
private:
  AkonadiCalendar* const q;

public:
  explicit Private( AkonadiCalendar *q );
  ~Private();

  bool addIncidence( const KCal::Incidence::Ptr & );
  bool deleteIncidence( const Akonadi::Item & );

  enum UpdateMode {
    DontCare,
    AssertExists,
    AssertNew
  };

  void updateItem( const Akonadi::Item &item, UpdateMode mode );
  void itemChanged( const Akonadi::Item& item );

  void assertInvariants() const;

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

public Q_SLOTS:  
  void listingDone( KJob *job );
  void agentCreated( KJob *job );
  void createDone( KJob *job );
  void deleteDone( KJob *job );
  void modifyDone( KJob *job );
  void itemChanged( const Akonadi::Item& item, const QSet<QByteArray>& );
  void itemMoved( const Akonadi::Item &item, const Akonadi::Collection& colSrc, const Akonadi::Collection& colDst );
  void itemsAdded( const Akonadi::Item::List &items );
  void itemAdded( const Akonadi::Item &item );
  void itemsRemoved( const Akonadi::Item::List &items );
  void itemRemoved( const Akonadi::Item &item );
};

#endif
