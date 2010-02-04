/*
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

#ifndef KJOTSMIGRATOR_H
#define KJOTSMIGRATOR_H

#include "kmigratorbase.h"

#include <QDir>

#include <KJob>

#include <akonadi/collection.h>
#include <akonadi/item.h>

class QDomElement;

class KJotsMigrator : public KMigratorBase
{
  Q_OBJECT
public:
  KJotsMigrator();
  virtual ~KJotsMigrator();

  /* reimp */ void migrate();
  /* reimp */ void migrateNext();

protected:
  /* reimp */ void migrationFailed(const QString& errorMsg, const Akonadi::AgentInstance& instance = Akonadi::AgentInstance());

private slots:
  void notesResourceCreated(KJob *job);
  void bookMigrateJobFinished(KJob *job);

private:
  void migrateLegacyBook(const QString &filename);
  void parsePageXml( QDomElement&, bool, const Akonadi::Collection &parentCollection );
  void parseBookXml( QDomElement&, bool, const Akonadi::Collection &parentCollection, int depth );

  void migrationFinished();

private:
  QDir m_backupDir;
  QDir m_dataDir;
  QStringList m_bookFiles;
  bool unicode;

  Akonadi::Item::List m_items;
  QList<Akonadi::Collection::List> m_collectionLists;
  Akonadi::Collection m_resourceCollection;

};

#endif

