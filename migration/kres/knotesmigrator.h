/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>
    Copyright (c) 2010 Stephen Kelly <steveire@gmail.com>

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

#ifndef KNOTESMIGRATOR_H
#define KNOTESMIGRATOR_H

#include "kresmigrator.h"

#include <kcal/resourcecalendar.h>
#include <kcal/calendarlocal.h>
#include <akonadi/collection.h>

#include <QHash>

#include <kresources/resource.h>

class KJob;

/**
 * Migrate KNotes resources to Akonadi
 */
class KNotesMigrator : public KResMigrator<KRES::Resource>
{
  Q_OBJECT
  public:
    KNotesMigrator();

    bool migrateResource( KRES::Resource *res );

  private slots:
    void notesResourceCreated( KJob* job );
    void syncDone(KJob *job);
    void rootFetchFinished( KJob *job );
    void rootCollectionsRecieved( const Akonadi::Collection::List &list );
    void newResourceFilled( KJob *job );

  private:
    void startMigration();

  private:
    Akonadi::Collection m_resourceCollection;
    AgentInstance m_agentInstance;

    KCal::CalendarLocal *m_notesResource;
};

#endif
