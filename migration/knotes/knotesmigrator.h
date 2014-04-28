/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>
    Copyright (c) 2010 Stephen Kelly <steveire@gmail.com>
    Copyright (c) 2013 Laurent Montel <montel@kde.org>

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

#include "kmigratorbase.h"
#include <kcal/resourcecalendar.h>
#include <kcal/calendarlocal.h>
#include <akonadi/collection.h>

#include <QHash>

#include <kresources/resource.h>

class KJob;

/**
 * Migrate KNotes resources to Akonadi
 */
class KNotesMigrator : public KMigratorBase
{
    Q_OBJECT
public:
    KNotesMigrator();
    ~KNotesMigrator();

    /* reimp */ void migrate();
    /* reimp */ void migrateNext();
protected:
  /* reimp */ void migrationFailed( const QString& errorMsg, const Akonadi::AgentInstance& instance = Akonadi::AgentInstance() );

private slots:
    void notesResourceCreated( KJob* job );
    void syncDone(KJob *job);
    void rootFetchFinished( KJob *job );
    void rootCollectionsRecieved( const Akonadi::Collection::List &list );
    void newResourceFilled( KJob *job );    
    void slotCollectionModify(KJob *job);

private:
    void startMigration();
    void showDefaultCollection();

private:
    int mIndexResource;
    QStringList mUnknownTypeResources;
    Akonadi::Collection m_resourceCollection;
    Akonadi::AgentInstance m_agentInstance;

    KCal::CalendarLocal *m_notesResource;
    KConfig *mConfig;
};

#endif
