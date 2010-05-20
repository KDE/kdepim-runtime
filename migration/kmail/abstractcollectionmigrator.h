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

#ifndef ABSTRACTCOLLECTIONMIGRATOR_H
#define ABSTRACTCOLLECTIONMIGRATOR_H

#include <QObject>

namespace Akonadi {
  class AgentInstance;
  class Collection;
}

class KJob;

class AbstractCollectionMigrator : public QObject
{
  Q_OBJECT

  public:
    explicit AbstractCollectionMigrator( const Akonadi::AgentInstance &resource, QObject *parent = 0 );
    ~AbstractCollectionMigrator();

  Q_SIGNALS:
    void migrationFinished( const Akonadi::AgentInstance &resource, const QString &error );

    void message( int type, const QString &msg );

  protected:
    virtual void migrateCollection( const Akonadi::Collection &collection ) = 0;

    void collectionProcessed();
    void migrationDone();
    void migrationCancelled( const QString &error );

    const Akonadi::AgentInstance resource() const;

  private:
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT( d, void collectionAdded( Akonadi::Collection ) )
    Q_PRIVATE_SLOT( d, void fetchResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void processNextCollection() )
    Q_PRIVATE_SLOT( d, void recheckBrokenResource() )
    Q_PRIVATE_SLOT( d, void recheckIdleResource() )
    Q_PRIVATE_SLOT( d, void resourceStatusChanged( Akonadi::AgentInstance ) )
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
