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

#ifndef DIMAPCACHECOLLECTIONMIGRATOR_H
#define DIMAPCACHECOLLECTIONMIGRATOR_H

#include "abstractcollectionmigrator.h"

class DImapCacheCollectionMigrator : public AbstractCollectionMigrator
{
  Q_OBJECT

  public:
    explicit DImapCacheCollectionMigrator( const Akonadi::AgentInstance &resource, QObject *parent = 0 );

    ~DImapCacheCollectionMigrator();

    bool migrationOptionsEnabled() const;

  protected:
    void migrateCollection( const Akonadi::Collection &collection, const QString &folderId );

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

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
