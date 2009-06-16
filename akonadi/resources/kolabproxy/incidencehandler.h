/*
    Copyright (C) 2009 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Copyright (c) 2009 Andras Mantia <andras@kdab.net>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef INCIDENCEHANDLER_H
#define INCIDENCEHANDLER_H

#include "kolabhandler.h"
#include <kcal/incidence.h>
#include <kcal/calendarlocal.h>

#include <boost/shared_ptr.hpp>
typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

/**
	@author Andras Mantia <amantia@kde.org>
*/
class IncidenceHandler : public KolabHandler {
  Q_OBJECT
public:
    IncidenceHandler();

    virtual ~IncidenceHandler();

    virtual void itemAdded(const Akonadi::Item &item);
    virtual void itemDeleted(const Akonadi::Item &item);
    virtual Akonadi::Item::List translateItems(const Akonadi::Item::List & addrs);
    virtual void toKolabFormat(const Akonadi::Item &item, Akonadi::Item &imapItem);

Q_SIGNALS:
    void useGlobalMode();

protected:
  virtual KCal::Incidence* incidenceFromKolab(MessagePtr data) = 0;
  virtual QByteArray incidenceToXml(KCal::Incidence *incidence) = 0;
  void incidenceToItem(IncidencePtr e, Akonadi::Item &imapItem);

  struct StoredItem{
    StoredItem(Akonadi::Entity::Id _id, IncidencePtr _inc) : id(_id), incidence(_inc) {}
    StoredItem() : id(-1){}
    Akonadi::Entity::Id id;
    IncidencePtr incidence;
  };

  enum ConflictResolution {
    Local = 0,
    Remote,
    Both,
    Duplicate
  };

  ConflictResolution resolveConflict( IncidencePtr inc);

  KCal::CalendarLocal m_calendar;
  QMap<QString, StoredItem> m_uidMap;
};

#endif
