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

#include "emptyresourcecleaner.h"

#include <AkonadiCore/agentinstance.h>
#include <AkonadiCore/agentmanager.h>
#include <AkonadiCore/collection.h>
#include <AkonadiCore/collectiondeletejob.h>
#include <AkonadiCore/collectionfetchjob.h>
#include <AkonadiCore/collectionfetchscope.h>
#include <AkonadiCore/collectionstatistics.h>
#include <AkonadiCore/itemfetchjob.h>

#include "kmailmigration_debug.h"

#include <QHash>
#include <QVariant>

using namespace Akonadi;

typedef QHash<Collection::Id, Collection> CollectionHash;

class EmptyResourceCleaner::Private
{
  EmptyResourceCleaner *const q;

  public:
    Private( EmptyResourceCleaner *parent, const AgentInstance &resource )
      : q( parent ), mResource( resource ), mOptions( CheckOnly )
    {
    }

    void deleteCollections();
    void deleteResource();

    void cleaupFinished();

  public:
    AgentInstance mResource;

    CleaningOptions mOptions;

    Collection::List mAllCollections;
    CollectionHash mDeletableCollections;
    CollectionHash mDeletedCollections;

  public: // slots
    void collectionFetchResult( KJob *job );
    void collectionDeleteResult( KJob *job );
};

void EmptyResourceCleaner::Private::deleteCollections()
{
  CollectionHash::iterator it = mDeletableCollections.begin();
  while ( it != mDeletableCollections.end() ) {
    const Collection collection = it.value();
    const Collection parent = collection.parentCollection();
    if ( mDeletedCollections.contains( parent.id() ) ) {
      mDeletedCollections.insert( collection.id(), collection );
      it = mDeletableCollections.erase( it );
    } else {
      qCDebug(KMAILMIGRATION_LOG) << "Removing collection: id=" << collection.id()
                                       << "remoteId=" << collection.remoteId();
      CollectionDeleteJob *deleteJob = new CollectionDeleteJob( collection );
      deleteJob->setProperty( "collection", QVariant::fromValue<Collection>( collection ) );
      connect( deleteJob, SIGNAL(result(KJob*)), q, SLOT(collectionDeleteResult(KJob*)) );
      return;
    }
  }

  Q_ASSERT( mDeletableCollections.isEmpty() );

  if ( mOptions.testFlag( DeleteEmptyResource ) &&
       mDeletedCollections.count() == mAllCollections.count() ) {
    deleteResource();
  } else {
    emit q->cleanupFinished( mResource );
  }
}

void EmptyResourceCleaner::Private::deleteResource()
{
  qCDebug(KMAILMIGRATION_LOG) << "Removing resource" << mResource.identifier();

  AgentManager::self()->removeInstance( mResource );

  emit q->cleanupFinished( mResource );
}

void EmptyResourceCleaner::Private::collectionFetchResult( KJob *job )
{
  if ( job->error() != 0 ) {
    qCritical() << job->errorString();
    emit q->cleanupFinished( mResource );
    return;
  }

  CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob*>( job );
  Q_ASSERT( fetchJob != 0 );

  mAllCollections = fetchJob->collections();
  CollectionHash nonEmptyCollections;
  Q_FOREACH ( const Collection &collection, mAllCollections ) {
    qint64 itemCount = collection.statistics().count();
    // TODO should be async as well
    if ( itemCount == -1 ) {
      ItemFetchJob *itemFetch = new ItemFetchJob( collection );
      if ( itemFetch->exec() ) {
        itemCount = itemFetch->items().count();
      }
    }

    qCDebug(KMAILMIGRATION_LOG) << "collection" << collection.name()
                                     << "has" << itemCount
                                     << "items";
    if ( itemCount == 0 ) {
      mDeletableCollections.insert( collection.id(), collection );
    } else {
      nonEmptyCollections.insert( collection.id(), collection );
    }
  }

  qCDebug(KMAILMIGRATION_LOG) << mDeletableCollections.count()
                                   << "potenially deletable collections"
                                   << nonEmptyCollections.count() << "not empty";

  Q_FOREACH ( const Collection &collection, nonEmptyCollections ) {
    Collection parent = collection.parentCollection();
    while ( parent.isValid() && parent != Collection::root() ) {
      mDeletableCollections.remove( parent.id() );
      parent = parent.parentCollection();
    }
  }

  qCDebug(KMAILMIGRATION_LOG) << mDeletableCollections.count() << "of"
                                   << mAllCollections.count() << "are empty";

  if ( mOptions.testFlag( DeleteEmptyCollections ) ) {
    deleteCollections();
  } else if ( mOptions.testFlag( DeleteEmptyResource ) &&
              mDeletableCollections.count() == mAllCollections.count() ) {
    deleteResource();
  } else {
    emit q->cleanupFinished( mResource );
  }
}

void EmptyResourceCleaner::Private::collectionDeleteResult( KJob *job )
{
  if ( job->error() != 0 ) {
    qCritical() << job->errorString();
    emit q->cleanupFinished( mResource );
    return;
  }

  CollectionDeleteJob *deleteJob = qobject_cast<CollectionDeleteJob*>( job );
  Q_ASSERT( deleteJob != 0 );

  const Collection collection = deleteJob->property( "collection" ).value<Collection>();
  mDeletableCollections.remove( collection.id() );
  mDeletedCollections.insert( collection.id(), collection );

  deleteCollections();
}

EmptyResourceCleaner::EmptyResourceCleaner( const AgentInstance &resource, QObject *parent )
  : QObject( parent ), d( new Private( this, resource ) )
{
  qCDebug(KMAILMIGRATION_LOG) << "Creating cleaner for resource"
                                   << d->mResource.identifier();
  connect(this, &EmptyResourceCleaner::cleanupFinished, this, &EmptyResourceCleaner::deleteLater);

  CollectionFetchScope scope;
  scope.setResource( d->mResource.identifier() );
  scope.setAncestorRetrieval( CollectionFetchScope::All );
  scope.setIncludeStatistics( true );

  CollectionFetchJob *fetchJob = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
  fetchJob->setFetchScope( scope );
  connect( fetchJob, SIGNAL(result(KJob*)), SLOT(collectionFetchResult(KJob*)) );
}

EmptyResourceCleaner::~EmptyResourceCleaner()
{
  delete d;
}

void EmptyResourceCleaner::setCleaningOptions( CleaningOptions options )
{
  d->mOptions = options;
}

EmptyResourceCleaner::CleaningOptions EmptyResourceCleaner::cleaningOptions() const
{
  return d->mOptions;
}

Collection::List EmptyResourceCleaner::deletableCollections() const
{
  return d->mDeletableCollections.values();
}

bool EmptyResourceCleaner::isResourceDeletable() const
{
  return ( d->mDeletedCollections.count() + d->mDeletableCollections.count() )
           == d->mAllCollections.count();
}

#include "moc_emptyresourcecleaner.cpp"

// kate: space-indent on; indent-width 2; replace-tabs on;
