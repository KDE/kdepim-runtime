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

#include "abstractcollectionmigrator.h"

#include <akonadi/agentinstance.h>
#include <akonadi/agentmanager.h>
#include <akonadi/collection.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/monitor.h>

#include <KLocale>

#include <QHash>
#include <QQueue>
#include <QTimer>

using namespace Akonadi;

typedef QHash<Collection::Id, Collection> CollectionHash;
typedef QQueue<Collection> CollectionQueue;

class AbstractCollectionMigrator::Private
{
  AbstractCollectionMigrator *const q;

  public:
    enum Status
    {
      Idle,
      Waiting,
      Scheduling,
      Running,
      Failed,
      Finished
    };

    Private( AbstractCollectionMigrator *parent, const AgentInstance &resource )
      : q( parent ), mResource( resource ), mStatus( Idle ), mMonitor( 0 ),
        mProcessedCollectionsCount( 0 ), mExplicitFetchStatus( Idle )
    {
    }

  public:
    AgentInstance mResource;
    Status mStatus;

    Monitor *mMonitor;

    CollectionHash mCollectionsById;
    CollectionQueue mCollectionQueue;
    int mProcessedCollectionsCount;

    Status mExplicitFetchStatus;

  public: // slots
    void collectionAdded( const Collection &collection );
    void fetchResult( KJob *job );
    void processNextCollection();
    void recheckBrokenResource();
    void resourceStatusChanged( const AgentInstance &instance );
};

void AbstractCollectionMigrator::Private::collectionAdded( const Collection &collection )
{
  if ( mStatus == Waiting ) {
    mStatus = Idle;
  }

  if ( !mCollectionsById.contains( collection.id() ) ) {
    mCollectionQueue.enqueue( collection );
    mCollectionsById.insert( collection.id(), collection );
  }

  if ( mStatus == Idle ) {
    mStatus = Scheduling;
    QMetaObject::invokeMethod( q, "processNextCollection", Qt::QueuedConnection );
  }
}

void AbstractCollectionMigrator::Private::fetchResult( KJob *job )
{
  mExplicitFetchStatus = Finished;

  CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob*>( job );
  Q_ASSERT( fetchJob != 0 );

  if ( fetchJob->error() != 0 ) {
    q->migrationCancelled( i18nc( "@info:status", "Resource '%1' didn't create any folders",
                                  mResource.name() ) );
  } else {
    const Collection::List collections = fetchJob->collections();
    Q_FOREACH( const Collection &collection, collections ) {
      collectionAdded( collection );
    }

    if ( mStatus == Idle ) {
      Q_ASSERT( mCollectionQueue.isEmpty() );
      q->migrationDone();
    }
  }
}

void AbstractCollectionMigrator::Private::processNextCollection()
{
  if ( mStatus == Failed || mStatus == Finished ) {
    return;
  }

  if ( mCollectionQueue.isEmpty() ) {
    if ( mResource.status() == AgentInstance::Idle && mExplicitFetchStatus != Running ) {
      q->migrationDone();
    } else {
      mStatus = Idle;
    }
  } else {
    mStatus = Running;
    ++mProcessedCollectionsCount;
    q->migrateCollection( mCollectionQueue.dequeue() );
  }
}

void AbstractCollectionMigrator::Private::recheckBrokenResource()
{
  if ( mStatus == Waiting ) {
    q->migrationCancelled( i18nc( "@info:status", "Resource '%1' it not working correctly",
                                  mResource.name() ) );
  }
}

void AbstractCollectionMigrator::Private::resourceStatusChanged( const Akonadi::AgentInstance &instance )
{
  if ( instance.identifier() != mResource.identifier() ) {
    return;
  }

  const AgentInstance::Status oldStatus = mResource.status();
  mResource = instance;

  if ( oldStatus != AgentInstance::Idle && mResource.status() == AgentInstance::Idle && mExplicitFetchStatus == Idle ) {
    mExplicitFetchStatus = Running;

    CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );

    CollectionFetchScope colScope;
    colScope.setResource( mResource.identifier() );
    colScope.setAncestorRetrieval( CollectionFetchScope::All );
    job->setFetchScope( colScope );

    connect( job, SIGNAL( result( KJob* ) ), q, SLOT( fetchResult( KJob* ) ) );
  }

  if ( mStatus == Waiting ) {
    mStatus = Idle;
  }
}

AbstractCollectionMigrator::AbstractCollectionMigrator( const AgentInstance &resource, QObject *parent )
  : QObject( parent ), d( new Private( this, resource ) )
{
  CollectionFetchScope colScope;
  colScope.setResource( d->mResource.identifier() );
  colScope.setAncestorRetrieval( CollectionFetchScope::All );

  d->mMonitor = new Monitor( this );
  d->mMonitor->setResourceMonitored( d->mResource.identifier().toAscii(), true );
  d->mMonitor->fetchCollection( true );
  d->mMonitor->setCollectionFetchScope( colScope );

  connect( d->mMonitor, SIGNAL( collectionAdded( Akonadi::Collection, Akonadi::Collection ) ), SLOT( collectionAdded( Akonadi::Collection ) ) );

  if ( d->mResource.status() != AgentInstance::Broken ) {
    d->mExplicitFetchStatus = Private::Running;
    CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
    job->setFetchScope( colScope );

    connect( job, SIGNAL( result( KJob* ) ), this, SLOT( fetchResult( KJob* ) ) );
  } else {
    // if resource is "Broken", it could still become idle after fully processing its new config
    // wait for at most one minute
    QTimer::singleShot( 60000, this, SLOT( recheckBrokenResource() ) );
  }

  connect( AgentManager::self(), SIGNAL( instanceStatusChanged( Akonadi::AgentInstance ) ),
           this, SLOT( resourceStatusChanged( Akonadi::AgentInstance  ) ) );
}

AbstractCollectionMigrator::~AbstractCollectionMigrator()
{
  delete d;
}

void AbstractCollectionMigrator::collectionProcessed()
{
  d->mStatus = Private::Scheduling;
  QMetaObject::invokeMethod( this, "processNextCollection", Qt::QueuedConnection );
}

void AbstractCollectionMigrator::migrationDone()
{
  kDebug() << "processed" << d->mProcessedCollectionsCount << "collection"
           << "seen" << d->mCollectionsById.count();
  d->mStatus = Private::Finished;
  emit migrationFinished( d->mResource, QString() );
}

void AbstractCollectionMigrator::migrationCancelled( const QString &error )
{
  kDebug() << "processed" << d->mProcessedCollectionsCount << "collection"
           << "seen" << d->mCollectionsById.count();
  d->mStatus = Private::Failed;
  emit migrationFinished( d->mResource, error );
}

const AgentInstance AbstractCollectionMigrator::resource() const
{
  return d->mResource;
}

#include "abstractcollectionmigrator.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
