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
  : Akonadi::TransactionSequence( parent ), m_collections( collections ), m_items( items )
{

}

void EntityTreeCreateJob::doStart()
{
  CollectionCreateJob *job;
  foreach ( const Collection::List &colList, m_collections )
  {
    foreach( const Collection &collection, colList )
    {
      job = new CollectionCreateJob( collection, this );
      job->setProperty( collectionIdMappingProperty, collection.id() );
      connect( job, SIGNAL(result(KJob*)), SLOT(collectionCreateJobDone(KJob*)) );
    }
  }
}

void EntityTreeCreateJob::collectionCreateJobDone( KJob *job )
{
  CollectionCreateJob *collectionCreateJob = qobject_cast<CollectionCreateJob *>( job );
  Collection createdCollection = collectionCreateJob->collection();

  if ( job->error() )
  {
    kDebug() << job->errorString();
    return;
  }

  int creationId = job->property( collectionIdMappingProperty ).toLongLong();

  Item::List::iterator it;
  const Item::List::iterator end = m_items.end();
  for ( it = m_items.begin(); it != end; )
  {
    if ( it->parentCollection().id() == creationId )
    {
      (void) new ItemCreateJob( *it, createdCollection, this );
      it = m_items.erase( it );
    }
    else {
      ++it;
    }
  }
  if ( m_items.isEmpty() )
    commit();
}

#include "entitytreecreatejob.moc"
