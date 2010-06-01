/*  This file is part of the KDE project
    Copyright (C) 2010 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.net
    Author: Kevin Krammer, krake@kdab.com

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef IMAPCACHECOLLECTIONMIGRATOR_H
#define IMAPCACHECOLLECTIONMIGRATOR_H

#include "abstractcollectionmigrator.h"

class ImapCacheCollectionMigrator : public AbstractCollectionMigrator
{
  Q_OBJECT

  public:
    enum MigrationOption
    {
      ConfigOnly = 0x0,
      ImportNewMessages = 0x01,
      ImportCachedMessages = 0x02,
      RemoveDeletedMessages = 0x04
    };

    Q_DECLARE_FLAGS( MigrationOptions, MigrationOption )

    explicit ImapCacheCollectionMigrator( const Akonadi::AgentInstance &resource, QObject *parent = 0 );

    ~ImapCacheCollectionMigrator();

    void setMigrationOptions( const MigrationOptions &options );

    MigrationOptions migrationOptions() const;

    void setCacheBasePath( const QString &basePath );

  protected:
    void migrateCollection( const Akonadi::Collection &collection, const QString &folderId );

    // overridden because of own reporting
    void migrationProgress( int processedCollections, int seenCollections );

  private:
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT( d, void fetchItemsResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void processNextItem() )
    Q_PRIVATE_SLOT( d, void processNextDeletedUid() )
    Q_PRIVATE_SLOT( d, void fetchItemResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void itemCreateResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void itemDeletePhase1Result( KJob* ) )
    Q_PRIVATE_SLOT( d, void itemDeletePhase2Result( KJob* ) )
};

Q_DECLARE_OPERATORS_FOR_FLAGS( ImapCacheCollectionMigrator::MigrationOptions )

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
