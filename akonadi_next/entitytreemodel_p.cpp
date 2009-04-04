/*
    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

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

#include "entitytreemodel_p.h"
#include "entitytreemodel.h"

#include <KUrl>
#include <KIconLoader>

#include <akonadi/collectionstatistics.h>
#include <akonadi/collectionstatisticsjob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/monitor.h>
#include "collectionchildorderattribute.h"
#include <akonadi/entitydisplayattribute.h>

#include <akonadi/session.h>
#include <kdebug.h>

using namespace Akonadi;

EntityTreeModelPrivate::EntityTreeModelPrivate( EntityTreeModel *parent )
    : q_ptr( parent ),
      m_showRootCollection(false),
      m_collectionFetchStrategy(EntityTreeModel::FetchCollectionsRecursive),
      m_itemPopulation(EntityTreeModel::ImmediatePopulation)
{
}

bool EntityTreeModelPrivate::mimetypeMatches( const QStringList &mimetypes, const QStringList &other )
{
  QStringList::const_iterator constIterator;
  bool found = false;

  for ( constIterator = mimetypes.constBegin(); constIterator != mimetypes.constEnd(); ++constIterator ) {
    if ( other.contains(( *constIterator ) ) ) {
      found = true;
      break;
    }
  }
  return found;
}

void EntityTreeModelPrivate::fetchItems( Collection parent, int retrieveDepth )
{
  Q_Q( EntityTreeModel );
  Akonadi::ItemFetchJob *itemJob = new Akonadi::ItemFetchJob( parent, m_session );
  itemJob->setFetchScope( m_monitor->itemFetchScope() );

  // ### HACK: itemsReceivedFromJob needs to know which collection items were added to.
  // That is not provided by akonadi, so we attach it in a property.
  itemJob->setProperty( ItemFetchCollectionId(), QVariant( parent.id() ) );

  q->connect( itemJob, SIGNAL( itemsReceived( Akonadi::Item::List ) ),
           q, SLOT( itemsFetched( Akonadi::Item::List ) ) );
  q->connect( itemJob, SIGNAL( result( KJob* ) ),
           q, SLOT( fetchJobDone( KJob* ) ) );
}

void EntityTreeModelPrivate::fetchCollections( Collection col, CollectionFetchJob::Type type )
{
  Q_Q( EntityTreeModel );
  CollectionFetchJob *job = new CollectionFetchJob( col, type, m_session );
  job->includeUnsubscribed( m_includeUnsubscribed );
  q->connect( job, SIGNAL( collectionsReceived( Akonadi::Collection::List ) ),
           q, SLOT( collectionsFetched( Akonadi::Collection::List ) ) );
  q->connect( job, SIGNAL( result( KJob* ) ),
           q, SLOT( fetchJobDone( KJob* ) ) );
}


void EntityTreeModelPrivate::collectionsFetched( const Akonadi::Collection::List& cols )
{
  Q_Q( EntityTreeModel );
  QHash<Collection::Id, Collection> newCollections;
  QHash< Collection::Id, QList< Collection::Id > > newChildCollections;
  foreach( Collection col, cols ) {
    if ( m_collections.contains( col.id() ) ) {
      // If we already know about the collection, there is nothing left to do
      continue;
    }
    // ... otherwise we add it to the set of collections we need to handle.
    if ( passesFilter( col.contentMimeTypes() ) ) {
      newChildCollections[ col.parent()].append( col.id() );
      newCollections.insert( col.id(), col );
    }
  }


  // Insert new child collections into model.
  QHashIterator< Collection::Id, QList< Collection::Id > > i( newChildCollections );
  while ( i.hasNext() ) {
    i.next();

    // the key is the parent of new collections.
    Collection::Id parentId = i.key();

    QList< Collection::Id > newChildCols = i.value();
    int newChildCount = newChildCols.size();

    if ( m_collections.contains( parentId ) ) {
      int startRow = 0; // Prepend collections.

  //       TODO: account for ordering.
        QModelIndex parentIndex = q->indexForCollection(m_collections.value(parentId));
        q->beginInsertRows(parentIndex, startRow, startRow + newChildCount - 1 );
        foreach( Collection::Id id, newChildCols ) {
          Collection c = newCollections.value( id );
          m_collections.insert( id, c );
          m_childEntities[ parentId ].prepend( id );
        }
        q->endInsertRows();

      foreach( Collection::Id id, newChildCols ) {
        Collection col = newCollections.value( id );
        // Fetch the next level of collections if neccessary.
        if ( m_collectionFetchStrategy == EntityTreeModel::FetchCollectionsRecursive )
        {
          fetchCollections( col, CollectionFetchJob::FirstLevel );
        }
        // Fetch items if neccessary. If we don't fetch them now, we'll wait for an application
        // to request them through EntityTreeModel::fetchMore
        if ( m_itemPopulation == EntityTreeModel::ImmediatePopulation )
        {
          fetchItems( col, Base );
        }
      }
    }
    // TODO: Fetch parent again so that its entities get ordered properly. Or start a modify job?
    // Should I do this for all other cases as well instead of using transactions?
    // Could be a way around the fact that monitor could notify me of things out of order. a parent could
    // be 'changed' for its entities to be reordered before one of its entities is in the model yet.
  }
}

void EntityTreeModelPrivate::itemsFetched( const Akonadi::Item::List& list )
{
  Q_Q( EntityTreeModel );
  QObject *job = q->sender();
  if ( job ) {
    Collection::Id colId = job->property( ItemFetchCollectionId() ).value<Collection::Id>();
    Item::List itemsToInsert;
    Item::List itemsToUpdate;

    foreach( Item item, list ) {
      if ( m_items.contains( item.id() * -1 ) ) {
        itemsToUpdate << item;
      } else {
        if ( passesFilter( QStringList() << item.mimeType() ) ) {
          itemsToInsert << item;
        }
      }
    }
    if ( itemsToInsert.size() > 0 ) {
      int startRow = m_childEntities.value( colId ).size();

      QModelIndex parentIndex = q->indexForCollection(m_collections.value(colId));
      q->beginInsertRows(parentIndex, startRow, startRow + itemsToInsert.size() - 1);
      foreach( Item item, itemsToInsert ) {
        qint64 itemId = item.id() * -1;
        m_items.insert( itemId, item );
        m_childEntities[ colId ].append( itemId );
      }
      q->endInsertRows();
    }
  }
}

void EntityTreeModelPrivate::monitoredCollectionAdded( const Akonadi::Collection& collection, const Akonadi::Collection& parent )
{
  Q_Q( EntityTreeModel );
  if ( !passesFilter( collection.contentMimeTypes() ) )
    return;

  // TODO: Use order attribute of parent if available
  // Otherwise prepend collections and append items. Currently this prepends all collections.

  // Or I can prepend and append for single signals, then 'change' the parent.

//   QList<qint64> childCols = m_childEntities.value( parent.id() );
//   int row = childCols.size();
//   int numChildCols = childCollections.value(parent.id()).size();
  int row = 0;

  QModelIndex parentIndex = q->indexForCollection(parent);
  q->beginInsertRows(parentIndex, row, row);
  m_collections.insert( collection.id(), collection );
  m_childEntities[ parent.id()].prepend( collection.id() );
  q->endInsertRows();

}

void EntityTreeModelPrivate::monitoredCollectionRemoved( const Akonadi::Collection& collection )
{
  Q_Q( EntityTreeModel );

  int row = m_childEntities.value(collection.parent()).indexOf(collection.id());

  QModelIndex parentIndex = q->indexForCollection(m_collections.value(collection.parent()));

  q->beginRemoveRows(parentIndex, row, row);

  // TODO: Also need to handle all descendant collections and items here.

  m_collections.remove( collection.id() );                 // Remove deleted collection.
  m_childEntities.remove( collection.id() );               // Remove children of deleted collection.
  m_childEntities[ collection.parent() ].removeAt( row );  // Remove deleted collection from its parent.

  q->endRemoveRows();
}

void EntityTreeModelPrivate::monitoredCollectionMoved( const Akonadi::Collection& col, const Akonadi::Collection& src, const Akonadi::Collection& dest)
{
  Q_Q( EntityTreeModel );

  int srcRow = m_childEntities.value(src.id()).indexOf(col.id());

  QModelIndex srcParentIndex = q->indexForCollection(src);
  QModelIndex destParentIndex = q->indexForCollection(dest);

  int destRow = 0; // Prepend collections

  q->beginMoveRows(srcParentIndex, srcRow, srcRow, destParentIndex, destRow);
  m_childEntities[ src.id()].removeAt( srcRow );
  m_childEntities[ dest.id() ].prepend( col.id() );
  q->endMoveRows();

}

void EntityTreeModelPrivate::monitoredCollectionChanged( const Akonadi::Collection &col)
{
  Q_Q( EntityTreeModel );

  if (m_collections.contains(col.id()))
  {
     m_collections[ col.id() ] = col;
  }

  QModelIndex idx = q->indexForCollection(col);
  q->dataChanged(idx, idx);
}

void EntityTreeModelPrivate::monitoredCollectionStatisticsChanged( Akonadi::Collection::Id collection, const Akonadi::CollectionStatistics &statistics )
{
// kDebug();
}

void EntityTreeModelPrivate::monitoredItemAdded( const Akonadi::Item& item, const Akonadi::Collection& col)
{
  Q_Q( EntityTreeModel );

  if ( !passesFilter( QStringList() << item.mimeType() ) )
    return;

  int row = m_childEntities.value(col.id()).size();

  QModelIndex parentIndex = q->indexForCollection(m_collections.value(col.id()));

  q->beginInsertRows(parentIndex, row, row);
  m_items.insert( item.id() * -1, item );
  m_childEntities[ col.id()].append( item.id() * -1 );
  q->endInsertRows();

}

void EntityTreeModelPrivate::monitoredItemRemoved( const Akonadi::Item &item )
{
  Q_Q( EntityTreeModel );

  Collection col = getParentCollection( item );

  int row = m_childEntities.value(col.id()).indexOf(item.id() * -1);

  QModelIndex parentIndex = q->indexForCollection(m_collections.value(col.id()));

  q->beginInsertRows(parentIndex, row, row);
  m_items.remove( item.id() * -1 );
  m_childEntities[ col.id() ].removeAt( row );
  q->endInsertRows();
}

void EntityTreeModelPrivate::monitoredItemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_Q( EntityTreeModel );
  m_items[ item.id() * -1 ] = item;

  QModelIndex idx = q->indexForItem(item);
  q->dataChanged(idx, idx);
}

void EntityTreeModelPrivate::monitoredItemMoved( const Akonadi::Item& item,
                  const Akonadi::Collection& src, const Akonadi::Collection& dest )
{
  Q_Q( EntityTreeModel );

  qint64 itemId = item.id() * -1;

  int srcRow = m_childEntities.value(src.id()).indexOf( itemId );

  QModelIndex srcIndex = q->indexForCollection(src);
  QModelIndex destIndex = q->indexForCollection(dest);

  // Where should it go? Always append items and prepend collections and reorganize them with separate reactions to Attributes?

  int destRow = q->rowCount(destIndex);

  q->beginMoveRows(srcIndex, srcRow, srcRow, destIndex, destRow);
  m_childEntities[src.id()].removeAt(srcRow);
  m_childEntities[dest.id()].append(itemId);
  q->endMoveRows();
}

bool EntityTreeModelPrivate::passesFilter( const QStringList &mimetypes )
{
  if ( m_monitor->mimeTypesMonitored().size() == 0 )
    return true;

  foreach(const QString &mime, mimetypes)
  {
    if ( m_monitor->mimeTypesMonitored().contains( mime ) )
      return true;
  }
  return false;

}


void EntityTreeModelPrivate::fetchJobDone( KJob *job )
{
  if ( job->error() ) {
    kWarning( 5250 ) << "Job error: " << job->errorString() << endl;
  }
}

void EntityTreeModelPrivate::updateJobDone( KJob *job )
{
  if ( job->error() ) {
    // TODO: handle job errors
    kWarning( 5250 ) << "Job error:" << job->errorString();
  } else {
    // TODO: Is this trying to do the job of collectionstatisticschanged?
//     CollectionStatisticsJob *csjob = static_cast<CollectionStatisticsJob*>( job );
//     Collection result = csjob->collection();
//     collectionStatisticsChanged( result.id(), csjob->statistics() );
  }
}



void EntityTreeModelPrivate::startFirstListJob()
{
  Q_Q(EntityTreeModel);
  Collection rootCollection;
  // Even if the root collection is the invalid collection, we still need to start
  // the first list job with Collection::root.
  if (m_showRootCollection)
  {
    rootCollection = Collection::root();
    // Notify the outside that we're putting collection::root into the model.
    q->beginInsertRows(QModelIndex(), 0, 0);
    m_collections.insert( rootCollection.id(), rootCollection );
    m_childEntities[-1].append(rootCollection.id());
    q->endInsertRows();
  } else {
    // Otherwise store it silently because it's not part of the usable model.
    rootCollection = m_rootCollection;
    m_collections.insert( rootCollection.id(), rootCollection );
  }

//   kDebug() << "inserting" << rootCollection.id();

  // Includes recursive trees. Lower levels are fetched in the onRowsInserted slot if
  // necessary.
  if ( ( m_collectionFetchStrategy == EntityTreeModel::FetchFirstLevelChildCollections)
    || ( m_collectionFetchStrategy == EntityTreeModel::FetchCollectionsRecursive ) )
  {
    fetchCollections( rootCollection, CollectionFetchJob::FirstLevel );
  }
  // If the root collection is not collection::root, then it could have items, and they will need to be
  // retrieved now.

  if ( m_itemPopulation != EntityTreeModel::NoItemPopulation )
  {
    if (rootCollection != Collection::root())
      fetchItems( rootCollection, Base );
  }
}

Collection EntityTreeModelPrivate::getParentCollection( qint64 id ) const
{
  QHashIterator< Collection::Id, QList<qint64> > iter( m_childEntities );
  while ( iter.hasNext() ) {
    iter.next();
    if ( iter.value().contains( id ) ) {
      return m_collections.value( iter.key() );
    }
  }
  return Collection();
}

Collection EntityTreeModelPrivate::getParentCollection( Item item ) const
{
  return getParentCollection( item.id() * -1);
}

Collection EntityTreeModelPrivate::getParentCollection( Collection col ) const
{
  return m_collections.value( col.parent() );
}

qint64 EntityTreeModelPrivate::childAt(Collection::Id id, int position, bool *ok) const
{
  QList<qint64> list = m_childEntities.value( id );
  if (list.size() <= position )
  {
    *ok = false;
    return 0;
  }
  *ok = true;
  return list.at( position );
}


int EntityTreeModelPrivate::indexOf( Collection::Id parent, Collection::Id col ) const
{
  return m_childEntities.value(parent).indexOf(col);
}

Item EntityTreeModelPrivate::getItem(qint64 id) const
{
  if (id > 0) id *= -1;
  return m_items.value(id);
}

