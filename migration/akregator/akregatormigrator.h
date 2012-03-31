/*
    Copyright (c) 2011 Frank Osterfeld <osterfeld@kde.org>

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

#ifndef AKREGATORMIGRATOR_H
#define AKREGATORMIGRATOR_H

#include "kmigratorbase.h"

#include <QDir>

#include <KJob>

#include <akonadi/collection.h>
#include <akonadi/item.h>

class QDomElement;

class AkregatorMigrator : public KMigratorBase
{
  Q_OBJECT
public:
  AkregatorMigrator();
  ~AkregatorMigrator();

  /* reimp */ void migrate();
  /* reimp */ void migrateNext();

protected:
  /* reimp */ void migrationFailed( const QString& errorMsg, const Akonadi::AgentInstance& instance = Akonadi::AgentInstance() );

private slots:
  void resourceCreated( KJob *job );
  void rootCollectionsReceived( KJob* );
  void feedCollectionsReceived( KJob* job );
  void feedCollectionsReceived( const Akonadi::Collection::List& );
  void rootCollectionRenamed( KJob* );
  void syncDone(KJob *job);
  void itemImportDone( KJob* );

private:
  void migrationFinished();
  void startMigration();

private:
  QString m_resourceIdentifier;
  Akonadi::Collection::List m_unmigrated;
  Akonadi::Collection m_resourceCollection;

};

#endif
