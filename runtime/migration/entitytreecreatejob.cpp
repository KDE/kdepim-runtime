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


#include "entitytreecreatejob.h"

#include <QVariant>
#include <QStringList>

#include <Akonadi/CollectionCreateJob>
#include <Akonadi/ItemCreateJob>

using namespace Akonadi;

static const char collectionIdMappingProperty[] = "collectionIdMappingProperty";

EntityTreeCreateJob::EntityTreeCreateJob( QList< Akonadi::Collection::List > collections, Akonadi::Item::List items, QObject* parent )
  : Akonadi::TransactionSequence( parent ), m_collections( collections ), m_items( items ), m_pendingJobs( 0 )
{

}

void EntityTreeCreateJob::doStart()
{
  if ( !m_collections.isEmpty() )
    createNextLevelOfCollections();
  createReadyItems();
}

void EntityTreeCreateJob::createNextLevelOfCollections()
{
  CollectionCreateJob *job;

  const Collection::List colList = m_collections.takeFirst();
  foreach( const Collection &collection, colList )
  {
    ++m_pendingJobs;
    job = new CollectionCreateJob( collection, this );
    job->setProperty( collectionIdMappingProperty, collection.id() );
    connect( job, SIGNAL(result(KJob*)), SLOT(collectionCreateJobDone(KJob*)) );
  }
}

void EntityTreeCreateJob::createReadyItems()
{
  Item::List::iterator it;
  for ( it = m_items.begin(); it != m_items.end(); )
  {
    Collection parentCollection = ( *it ).parentCollection();
    if ( parentCollection.isValid() )
    {
      (void) new ItemCreateJob( *it, parentCollection, this );
      it = m_items.erase( it );
    } else {
      ++it;
    }
  }
  if ( m_items.isEmpty() && m_collections.isEmpty() )
    commit();
}

void EntityTreeCreateJob::collectionCreateJobDone( KJob *job )
{
  Q_ASSERT( m_pendingJobs > 0 );
  --m_pendingJobs;
  CollectionCreateJob *collectionCreateJob = qobject_cast<CollectionCreateJob *>( job );
  Collection createdCollection = collectionCreateJob->collection();

  if ( job->error() )
  {
    kDebug() << job->errorString();
    return;
  }

  const Collection::Id creationId = job->property( collectionIdMappingProperty ).toLongLong();

  Item::List::iterator it;
  const Item::List::iterator end = m_items.end();
  for ( it = m_items.begin(); it != end; ++it )
  {
    kDebug() << "updating items";
    if ( it->parentCollection().id() == creationId )
    {
      it->setParentCollection( createdCollection );
    }
  }

  createReadyItems();

  if ( !m_collections.isEmpty() )
  {
    Collection::List::iterator col_it;
    const Collection::List::iterator col_end = m_collections[0].end();
    for ( col_it = m_collections[0].begin(); col_it != col_end; ++col_it )
    {
      if ( col_it->parentCollection().id() == creationId )
      {
        col_it->setParentCollection( createdCollection );
      }
    }
    if ( m_pendingJobs == 0 )
        createNextLevelOfCollections();
  }

  if ( m_items.isEmpty() && m_collections.isEmpty() )
    commit();
}

#include "entitytreecreatejob.moc"
