/*
  This file is part of KOrganizer.

  Copyright (C) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>

  Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
  Author: Sergio Martins, <sergio.martins@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/
#ifndef INCIDENCECHANGER_P_H
#define INCIDENCECHANGER_P_H

#include "incidencechanger.h"

#include <kcalcore/incidence.h>

#include <Akonadi/Item>

#include <QObject>

namespace Akonadi {

class IncidenceChanger::Private : public QObject {
  Q_OBJECT
public:

  struct Change {
    KCalCore::Incidence::Ptr oldInc;
    Akonadi::Item newItem;
    IncidenceChanger::WhatChanged action;
    QWidget *parent;
  };

  Private( Entity::Id defaultCollectionId, Calendar *calendar ) :
           mDefaultCollectionId( defaultCollectionId ),
           mGroupware( 0 ),
           mDestinationPolicy( IncidenceChanger::ASK_DESTINATION ),
           mCalendar( calendar )
           { }

  ~Private() {}
  void queueChange( Change * );
  bool performChange( Change * );

  /*
   * Called when deleting an incidence, queued changes are canceled.
   * */
  void cancelChanges( Item::Id id );

  bool myAttendeeStatusChanged( const KCalCore::Incidence::Ptr newInc,
                                const KCalCore::Incidence::Ptr oldInc );

  Entity::Id mDefaultCollectionId;

  Groupware *mGroupware;
  DestinationPolicy mDestinationPolicy;

  // Changes waiting for a job on the same item.id() to end
  QHash<Item::Id,Change*> mQueuedChanges;

  // Current changes with modify jobs still running
  QHash<Item::Id, Change*> mCurrentChanges;

  QHash<Akonadi::Item::Id, int> mLatestRevisionByItemId;


  QList<Akonadi::Item::Id> mDeletedItemIds;

  Calendar *mCalendar;
  
public slots:
  void performNextChange( Akonadi::Item::Id );

private slots:
  void changeIncidenceFinished( KJob* job );

signals:
  void incidenceChangeFinished( const Akonadi::Item &oldinc,
                                const Akonadi::Item &newInc,
                                Akonadi::IncidenceChanger::WhatChanged,
                                bool );

};

} // namespace
#endif
