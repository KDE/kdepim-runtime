/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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


#include "descendantentitiesproxymodel.h"

#include "kdebug.h"

using namespace Akonadi;

namespace Akonadi
{

/**
  @internal

  Private implementation of DescendantEntitiesProxyModel.
*/
class DescendantEntitiesProxyModelPrivate
{
  public:

  DescendantEntitiesProxyModelPrivate(DescendantEntitiesProxyModel *model)
    : q_ptr(model),
      m_displayAncestorData(false)
    {

    }

  Q_DECLARE_PUBLIC( DescendantEntitiesProxyModel )
  DescendantEntitiesProxyModel *q_ptr;
  /**
  @internal

  Returns the @p row -th descendant of sourceParent.

  For example, if the source model looks like:
  @code
  -> Item 0-0 (this is row-depth)
  -> -> Item 0-1
  -> -> Item 1-1
  -> -> -> Item 0-2
  -> -> -> Item 1-2
  -> Item 1-0
  @endcode

  Then findSourceIndexForRow(3, index(Item 0-0)) will return an index for Item 1-2

  @returns The model index in the source model.

  */
  QModelIndex findSourceIndexForRow( int row, QModelIndex sourceParent) const;

  /**
  @internal

  Returns true if @p sourceIndex has a ancestor that is m_rootDescendIndex.
  Note that isDescended(m_rootDescendIndex) is false;
  @param sourceIndex The index in the source model.
  @returns Whether sourceIndex is an ancestor of rootIndex
  */
  bool isDescended(const QModelIndex &sourceIndex) const;

  /**
  @internal

  Returns the number of descendants below @p sourceIndex.

  For example, if the source model looks like:
  @code
  -> Item 0-0 (this is row-depth)
  -> -> Item 0-1
  -> -> Item 1-1
  -> -> -> Item 0-2
  -> -> -> Item 1-2
  -> Item 1-0
  @endcode

  The descendant count of the rootIndex would be 6,
  of Item 0-0 would be 4,
  of Item 1-1 would be 2,
  of Item 1-2 would be 0
  etc.

  */
  int descendantCount(const QModelIndex &sourceIndex) const;

  /**
  @internal

  Returns the row of @p sourceIndex below the rootDescendIndex.

  For example, if the source model looks like:
  @code
  -> Item 0-0 (this is row-depth)
  -> -> Item 0-1
  -> -> Item 1-1
  -> -> -> Item 0-2
  -> -> -> Item 1-2
  -> Item 1-0
  @endcode

  Then descendedRow(index(Item 0-0) would be 0,
  descendedRow(index(Item 0-1) would be 1,
  descendedRow(index(Item 0-2) would be 3,
  descendedRow(index(Item 1-0) would be 5
  etc.

  @returns The row in the proxy model where @p sourceIndex is represented.

  */
  int descendedRow(const QModelIndex &sourceIndex) const;


  enum Operation
  {
    InsertOperation,
    RemoveOperation
  };

  void insertOrRemoveRows(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, int type);

  void sourceRowsAboutToBeInserted(const QModelIndex &, int start, int end);
  void sourceRowsInserted(const QModelIndex &, int start, int end);
  void sourceRowsAboutToBeRemoved(const QModelIndex &, int start, int end);
  void sourceRowsRemoved(const QModelIndex &, int start, int end);
  void sourceModelAboutToBeReset();
  void sourceModelReset();
  void sourceLayoutAboutToBeChanged();
  void sourceLayoutChanged();
  void sourceDataChanged(QModelIndex,QModelIndex);

  QPersistentModelIndex m_rootDescendIndex;
  // Hmm, if I make this QHash<QPersistentModelIndex, int> instead then moves are
  // automatically handled. Nope, they're not. deeper levels than base would
  // still need to be updated or calculated.

  mutable QHash<qint64, int> m_descendantsCount;

  bool m_displayAncestorData;
  QString m_ancestorSeparator;

};

}

DescendantEntitiesProxyModel::DescendantEntitiesProxyModel( QObject *parent )
      : QAbstractProxyModel( parent ),// AbstractItemModel( this ),
//       : AbstractProxyModel( parent ),
        d_ptr( new DescendantEntitiesProxyModelPrivate(this) )
{
  Q_D( DescendantEntitiesProxyModel );

  d->m_rootDescendIndex = QModelIndex();
}

void DescendantEntitiesProxyModel::setRootIndex(const QModelIndex &index)
{
  // Can't assert that because:
  // a) The index might be invalid (root of source model)
  // b) Someone might set the root index before setting the model.
//   Q_ASSERT(index.model() == sourceModel());
  Q_D(DescendantEntitiesProxyModel);

  // Hmm, what if a consumer of this class does an operation in modelAboutToBeReset
  // which involves looking at m_rootDescendIndex? It would break?
  // I think I need separate beginResetModel and endResetModel methods to
  // handle things like this. Can probably put them in my abstract class.

//   begin_Reset_Model();
  d->m_rootDescendIndex = index;
  d->m_descendantsCount.clear();
//   reset_Model();
//   reset();
}

DescendantEntitiesProxyModel::~DescendantEntitiesProxyModel()
{
  Q_D(DescendantEntitiesProxyModel);
  d->m_descendantsCount.clear();
}

QModelIndex DescendantEntitiesProxyModelPrivate::findSourceIndexForRow( int row, QModelIndex idx ) const
{
    Q_Q( const DescendantEntitiesProxyModel );
    int childCount = q->sourceModel()->rowCount(idx);
    for (int childRow = 0; childRow < childCount; childRow++)
    {
      QModelIndex childIndex = q->sourceModel()->index(childRow, 0, idx);
      if (row == 0)
      {
        return childIndex;
      }
      row--;
      if (q->sourceModel()->hasChildren(childIndex))
      {
        int childDesc = descendantCount(childIndex);
        if (childDesc > row)
        {
          return findSourceIndexForRow(row, childIndex);
        }
        row -= childDesc;
      }
    }

    // This should probably never happen if you use descendantCount before calling this method.
    return QModelIndex();
}

void DescendantEntitiesProxyModel::setSourceModel(QAbstractItemModel * sourceModel)
{
  Q_D(DescendantEntitiesProxyModel);
  QAbstractProxyModel::setSourceModel( sourceModel );
  connect( sourceModel, SIGNAL(modelReset()), SLOT( sourceModelReset() ) );
  connect( sourceModel, SIGNAL(modelAboutToBeReset()), SLOT(sourceModelAboutToBeReset() ) );
  connect( sourceModel, SIGNAL(layoutChanged()), SLOT(sourceLayoutChanged()) );
  connect( sourceModel, SIGNAL(layoutAboutToBeChanged()), SLOT(sourceLayoutAboutToBeChanged()) );
  connect( sourceModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
          SLOT(sourceDataChanged(QModelIndex,QModelIndex)) );
  connect( sourceModel, SIGNAL(rowsInserted(const QModelIndex, int, int)),
          SLOT(sourceRowsInserted(const QModelIndex, int, int)) );
  connect( sourceModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex, int, int)),
          SLOT(sourceRowsAboutToBeInserted(const QModelIndex, int, int)) );
  connect( sourceModel, SIGNAL(rowsRemoved(const QModelIndex, int, int)),
          SLOT(sourceRowsRemoved(const QModelIndex, int, int)) );
  connect( sourceModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex, int, int)),
          SLOT(sourceRowsAboutToBeRemoved(const QModelIndex, int, int)) );

  d->m_descendantsCount.clear();
  reset();
}

bool DescendantEntitiesProxyModelPrivate::isDescended(const QModelIndex &sourceIndex) const
{
//     kDebug() << sourceIndex;
  Q_Q(const DescendantEntitiesProxyModel);

  if (sourceIndex == m_rootDescendIndex)
  {
    return false;
  }

  QModelIndex parentIndex = q->sourceModel()->parent(sourceIndex);

  // The invalid index is implicitly always in the model and should be considered descendend?
  // Really this should probably be outside this fn.
//   if (!parentIndex.isValid())
//     return true;


// Hmmm, this should probably be false.
  if (parentIndex == m_rootDescendIndex)
  {
    return true;
  }
  bool found = false;
//   kDebug() << parentIndex;
  while(true)
  {
    parentIndex = parentIndex.parent();
//     kDebug() << "next" << parentIndex;
    if (parentIndex == m_rootDescendIndex)
    {
      found = true;
      break;
    }
    if (!parentIndex.isValid())
      break;
  }
  return found;
}

int DescendantEntitiesProxyModelPrivate::descendedRow(const QModelIndex &sourceIndex) const
{
//   kDebug() << "sourceIndex" << sourceIndex;
  Q_Q(const DescendantEntitiesProxyModel);
  QModelIndex parentIndex = sourceIndex.parent();
  int row = sourceIndex.row();

  for (int childRow = 0; childRow < sourceIndex.row(); childRow++ )
  {
    QModelIndex childIndex = q->sourceModel()->index( childRow, 0, parentIndex );
//     kDebug() << childIndex << descendantCount(childIndex);
    if (q->sourceModel()->hasChildren(childIndex))
      row += descendantCount(childIndex);
  }
//   kDebug() << row << parentIndex << m_rootDescendIndex;

  if (parentIndex == m_rootDescendIndex)
  {
//     // Return 0 instead of -1 for an invalid index.
    if (row < 0)
    {
//       kDebug() << "returning:" << sourceIndex << "row" << 0 << "(-1)";
      return 0;
    }
//     kDebug() << "returning:" << sourceIndex << "row" << row;
    return row;
  }
  else if(!parentIndex.isValid())
  {
    // Should never happen.
    // Someone must have called this with sourceIndex outside of m_rootDescendIndex
//     kDebug() << "returning (never)" << 0;
    return 0;
  }
  else {
  int dr = descendedRow(parentIndex);
//     kDebug() << "returning:" << sourceIndex << "row" << row << "dr" << dr;
    return row + dr +1; // + 1; // for self?
  }
}

QModelIndex DescendantEntitiesProxyModel::mapFromSource(const QModelIndex & sourceIndex) const
{
//   kDebug() << "sourceIndex" << sourceIndex;

  Q_D(const  DescendantEntitiesProxyModel );

  if (sourceIndex == d->m_rootDescendIndex)
  {
    return QModelIndex();
  }

  if ( d->isDescended( sourceIndex ) )
  {
    int row = d->descendedRow( sourceIndex );
//     kDebug() << "isDescended" << row;
    if (row < 0)
      return QModelIndex();
    int column = 0;
    // TODO use col.id? or just 0?
    return createIndex( row, column, reinterpret_cast<void *>( sourceIndex.internalId() ) );
  } else {
    return QModelIndex();
//     return createIndex( sourceIndex.row(), sourceIndex.column(), reinterpret_cast<void *>( sourceIndex.internalId() ) );
  }
}

void DescendantEntitiesProxyModelPrivate::sourceRowsAboutToBeInserted(const QModelIndex &sourceParentIndex, int start, int end)
{
  insertOrRemoveRows(sourceParentIndex, start, end, InsertOperation);
}

void DescendantEntitiesProxyModelPrivate::insertOrRemoveRows(const QModelIndex &sourceParentIndex, int start, int end, int operationType)
{

  Q_Q(DescendantEntitiesProxyModel);
//   kDebug() << sourceParentIndex << start << end << sourceParentIndex.data();
//   QModelIndex sourceStartIndex = q->sourceModel()->index(start, 0, sourceParentIndex);
//   QModelIndex sourceEndIndex = q->sourceModel()->index(end, 0, sourceParentIndex);
//   kDebug() << "sourceStartIndex" << sourceStartIndex;
//   QModelIndex proxyStartIndex = q->mapFromSource(sourceStartIndex);
//   kDebug() << "proxyStartIndex" << proxyStartIndex;
//   QModelIndex proxyEndIndex = q->mapFromSource(sourceEndIndex);
//   kDebug() << "proxyEndIndex" << proxyEndIndex;
//   QModelIndex proxyParentIndex = q->mapFromSource(sourceParentIndex);

  int c = descendedRow(sourceParentIndex);
//   kDebug() << c << start;
//   kDebug() << proxyParentIndex << proxyStartIndex.row(); // << proxyEndIndex.row();

// Can't simply get the descendantCount of sourceModel()->index(start, 0, sourceParent),
// because start might not exist as a child of sourceParent yet.
// Maybe I should special case (!sourceParent.hasChildren) instead.
  for (int childRow = 0; childRow < start; childRow++)
  {
    QModelIndex childIndex = q->sourceModel()->index( childRow, 0, sourceParentIndex );
//     kDebug() << childIndex << descendantCount(childIndex);
    if (q->sourceModel()->hasChildren(childIndex))
      c += descendantCount(childIndex);
    // Account for self too.
//     c++;
// Nope. Don't. That's already represented by @p start.
  }

//   @code
//   -> Item 0-0 (this is row-depth)
//   -> -> Item 0-1
//   -> -> Item 1-1
//   -> -> -> Item 0-2
//   -> -> -> Item 1-2
//   -> Item 1-0
//   @endcode
//
// If the sourceModel reports that a row is inserted between Item 0-2 Item 1-2,
// this methods recieves a sourceParent of Item 1-1, and a start of 1.
// It has a descendedRow of 2. The proxied start is the number of rows above parent,
// and the start below parent. The parent itself must also be accounted for if it
// is part of the model.

// Check if it's descended instead?

  int proxy_start = c + start; // + 1; //proxyStartIndex.row();
  int proxy_end = c + end;
//   kDebug() << (sourceParentIndex != m_rootDescendIndex) << isDescended(sourceParentIndex);
//   if (sourceParentIndex != m_rootDescendIndex)

  if (isDescended(sourceParentIndex))
  {
    proxy_start++;
    proxy_end++;
  }
//   kDebug() << m_rootDescendIndex << proxy_start << proxy_end;
//   q->beginInsertRows(proxyParentIndex, proxy_start, proxy_end);
  if (operationType == InsertOperation)
    q->beginInsertRows(m_rootDescendIndex, proxy_start, proxy_end);
  else if (operationType == RemoveOperation)
  {
    // need to notify that we're also removing the descendants.
    for (int childRow = start; childRow <= end; childRow++)
    {
      QModelIndex childIndex = q->sourceModel()->index(childRow,0,sourceParentIndex);
      if (q->sourceModel()->hasChildren(childIndex))
        proxy_end += descendantCount(childIndex);
    }

    q->beginRemoveRows(m_rootDescendIndex, proxy_start, proxy_end);
  }
}

void DescendantEntitiesProxyModelPrivate::sourceRowsInserted(const QModelIndex &sourceParentIndex, int start, int end)
{
  Q_Q(DescendantEntitiesProxyModel);

//   if ( isDescended( sourceParentIndex ) || ( sourceParentIndex == m_rootDescendIndex ) )
//   {
//     int difference = (end - start) + 1;
//     m_descendantsCount[ sourceParentIndex.internalId() ] += difference;
//     QModelIndex parentIndex = sourceParentIndex.parent();
//     while (parentIndex != m_rootDescendIndex)
//     {
//       m_descendantsCount[ parentIndex.internalId() ] += difference;
//       parentIndex = parentIndex.parent();
//     }
//   }

  // Remove this instead of trying to keep it up to date.
  // We were told how many rows were inserted into sourceParentIndex,
  // But those rows could themselves have had descendants.
  // Oh, that means I have to descend and remove more from the hash...
  // Also, I have to ascend and remove the difference from higher entries.
  // It's probably easier to just clear it.
//   m_descendantsCount.remove( sourceParentIndex.internalId() );

// Disable this and only clear on layout change.
  // This is a rare operation, so clearing it should not be too expensive.
  m_descendantsCount.clear();


//   kDebug() << sourceParentIndex.data();
//   if (sourceParentIndex.data() == "Bar Book")
//   {
//     kDebug() << "per" << q->mapFromSource(sourceParentIndex);
//     q->pmi = QPersistentModelIndex(q->mapFromSource(sourceParentIndex));
//   }
//   kDebug() << q->persistentIndexList();
  q->endInsertRows();
}

void DescendantEntitiesProxyModelPrivate::sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
  insertOrRemoveRows(parent, start, end, RemoveOperation);
}

void DescendantEntitiesProxyModelPrivate::sourceRowsRemoved(const QModelIndex &sourceParentIndex, int start, int end)
{
  Q_Q(DescendantEntitiesProxyModel);
//     kDebug();
//   if (isDescended(sourceParentIndex) || ( sourceParentIndex == m_rootDescendIndex ) )
//   {
//     int difference = 0;
//     m_descendantsCount[ sourceParentIndex.internalId()] -= difference;
//     QModelIndex parentIndex = sourceParentIndex.parent();
//     while (parentIndex != m_rootDescendIndex)
//     {
// //       m_descendantsCount[ parentIndex.internalId()] -= difference;
//     }
//   }
  // Remove this instead of trying to keep it up to date.
  // We were told how many rows were removed  from sourceParentIndex,
  // But those rows could themselves have had descendants.
  // Oh, that means I have to descend and remove more from the hash...
//   m_descendantsCount.remove( sourceParentIndex.internalId() );


  m_descendantsCount.clear();
  q->endRemoveRows();

}

void DescendantEntitiesProxyModelPrivate::sourceModelAboutToBeReset()
{
  Q_Q(DescendantEntitiesProxyModel);

  kDebug();
//   begin_Reset_Model();

// This doesn't work because it's a private method.
//   q->modelAboutToBeReset();
  // We reset the proxy model when the source model is about to be reset.
  // This way, consumers of the proxy model can react to the modelAboutToBeReset
  // signal before the sourcemodel is fully reset.

  // The alternative might be to use QMetaObject::invokeMethod to emit the private
  // signal if they really do need to be emitted in concert
  // between the source and the proxy model.

//   QMetaObject::invokeMethod(modelAboutToBeReset);
//   q->reset();

//   m_descendantsCount.clear();
//   q->model_About_To_Be_Reset();
}

void DescendantEntitiesProxyModelPrivate::sourceModelReset()
{
  Q_Q(DescendantEntitiesProxyModel);
//   QMetaObject::invokeMethod(modelReset);

  m_descendantsCount.clear();

//   q->model_Reset();
//   q->reset();
}

void DescendantEntitiesProxyModelPrivate::sourceLayoutAboutToBeChanged()
{
  Q_Q(DescendantEntitiesProxyModel);

  q->layoutAboutToBeChanged();
}

void DescendantEntitiesProxyModelPrivate::sourceLayoutChanged()
{
  Q_Q(DescendantEntitiesProxyModel);

  kDebug() << "clear ###";
  m_descendantsCount.clear();
  q->layoutChanged();
}

QModelIndex DescendantEntitiesProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
//     kDebug() << proxyIndex;
  Q_D(const DescendantEntitiesProxyModel );
//   Hmm. If this proxy shows only descendant items this could be much simpler.

  if (!proxyIndex.isValid())
    return d->m_rootDescendIndex;

  if (proxyIndex.column() >= sourceModel()->columnCount())
    return QModelIndex();

  QModelIndex idx = d->findSourceIndexForRow( proxyIndex.row(), d->m_rootDescendIndex );
//   kDebug() << idx;
  return idx;
//   QModelIndex idx = findSourceIndexForRow( proxyIndex.row(), m_rootDescendIndex );

//   if (!idx.isValid())
//   {
//     return m_rootDescendIndex;
//   } else {
//     return idx;
//   }

//   Can't do that obviously
//   Collection col = sourceModel()->data(proxyIndex, EntityTreeModel::CollectionRole).value<Collection>();

//   while (parentCol.isValid())
//   if (col.parent() == m_rootDescendIndex.internalId())
//   {
//     int childCount = sourceModel()->rowCount(m_rootDescendIndex);
//     for (int childRow = 0; childRow < childCount; childRow++)
//     {
//       QModelIndex childIndex = sourceModel()->index(childRow, 0, m_rootDescendIndex);
//       int desc = descendantCount(childIndex);
//     }
//   }




//   kDebug() << "proxyIndex" << proxyIndex;
//   if (!proxyIndex.isValid())
//     return QModelIndex();
//
//   Collection parCol;
//   int entityType = m_clientSideEntityStorage->entityTypeForInternalIdentifier( proxyIndex.internalId() );
//   if ( ClientSideEntityStorage::CollectionType == entityType )
//   {
//     Collection col = m_clientSideEntityStorage->getCollection( proxyIndex.internalId() );
//     parCol = m_clientSideEntityStorage->getParentCollection( col );
//   } else if ( ClientSideEntityStorage::ItemType == entityType )
//   {
//     Item item = m_clientSideEntityStorage->getItem( proxyIndex.internalId() );
//     parCol = m_clientSideEntityStorage->getParentCollection( item );
//   }
//
//   // TODO: the root of this model should be a member of this class for versatility.
// //   Collection root = m_clientSideEntityStorage->rootCollection();
//
//   int ansec = getAncestorTotal( m_rootDescendCollection, proxyIndex.internalId() );
//
//   int column = 0;
// //   int row = proxyIndex.row() - ansec;
//   int row = ansec;
// //   qint64 id = parCol.childAt( parCol.id(), entIndex );
//
//   QModelIndex idx = createIndex( row, column, reinterpret_cast<void *>( proxyIndex.internalId() ) );
//   kDebug() << "idx" << idx;
//   return idx;

}

QVariant DescendantEntitiesProxyModel::data(const QModelIndex & index, int role) const
{
  Q_D(const DescendantEntitiesProxyModel);
//   if ( role == Qt::DisplayRole ) {
//     kDebug() << index;
    QModelIndex sourceIndex = mapToSource( index );
//     kDebug() << sourceIndex << sourceIndex.data();

  if ((d->m_displayAncestorData) && ( role == Qt::DisplayRole ) )
  {
    if (!sourceIndex.isValid())
    {
      return QVariant();
    }
    QString displayData = sourceIndex.data().toString();
    sourceIndex = sourceIndex.parent();
    while (sourceIndex.isValid())
    {
      displayData.prepend(d->m_ancestorSeparator);
      displayData.prepend(sourceIndex.data().toString());
      sourceIndex = sourceIndex.parent();
    }
    return displayData;
  } else {
    return sourceIndex.data(role);
  }

//     QString name = sourceIndex.data( role ).toString();
//     sourceIndex = sourceIndex.parent();
//     while ( sourceIndex.isValid() ) {
//       name.prepend( sourceIndex.data( role ).toString() + QLatin1String( "/" ) );
//       sourceIndex = sourceIndex.parent();
//     }
//     return name;
//   }
//   return QAbstractProxyModel::data( index, role );
}

bool DescendantEntitiesProxyModel::hasChildren ( const QModelIndex & parent ) const
{
//   kDebug() << parent;
  return rowCount(parent) > 0;
}

int DescendantEntitiesProxyModel::rowCount(const QModelIndex & proxyIndex) const
{
//   kDebug() << "proxyIndex" << proxyIndex;

//   Collection col;
//   if (proxyIndex.isValid())
//     col = m_clientSideEntityStorage->getCollection( proxyIndex.internalId() );
//   else
//     col = m_rootDescendCollection;
//   kDebug() << col.isValid() << col.id();
//   int ansec = getAncestorTotal( col );


  Q_D( const DescendantEntitiesProxyModel );
  QModelIndex sourceIndex = mapToSource(proxyIndex);

  if (sourceIndex == d->m_rootDescendIndex)
  {
    int c = d->descendantCount(sourceIndex);
//     kDebug() << sourceIndex << c;
    return c;
  }
  return 0;
//   kDebug() << sourceIndex;

//   if (!sourceIndex.isValid())
//     return 0;

//   kDebug() << sourceModel()->hasChildren(sourceIndex)
//            << sourceModel()->rowCount(sourceIndex);

//   if (sourceIndex != d->m_rootDescendIndex )
  if(sourceModel()->hasChildren(sourceIndex))
  {
    int c = d->descendantCount(sourceIndex);
//     kDebug() << c;
    return c;
  }
  return 0;

//   if (isDescended(sourceIndex))
//   {
//     return descendantCount(sourceIndex);
//   }
//   else {
//     return 0;
//     // We're not listing the descendants of this index.
//     return sourceModel()->rowCount(sourceIndex);
//
//   }

//   int ansec = getAncestorTotal(sourceIndex);

//   int num = QAbstractProxyModel::rowCount(proxyIndex);
//   kDebug() << num << ansec;
//   return ansec;
//   return num;
}

void DescendantEntitiesProxyModelPrivate::sourceDataChanged(QModelIndex,QModelIndex)
{

}

int DescendantEntitiesProxyModelPrivate::descendantCount(const QModelIndex &sourceIndex) const
{

//   Q_ASSERT(sourceIndex.model() == sourceModel());

//     kDebug();

  Q_Q( const DescendantEntitiesProxyModel );
  if (m_descendantsCount.contains(sourceIndex.internalId()))
  {
    return m_descendantsCount.value(sourceIndex.internalId());
  }

  int sourceIndexRowCount = q->sourceModel()->rowCount(sourceIndex);
//   kDebug() << "sourceIndexRowCount" << sourceIndexRowCount << sourceIndex;
  if (sourceIndexRowCount == 0)
    return 0;
  int c = 0;
  c += sourceIndexRowCount;
//   kDebug() << "(start) c:" << c << sourceIndex;
//   kDebug() << "c:" << c;
  int childRow = 0;
  QModelIndex childIndex = q->sourceModel()->index(childRow, 0, sourceIndex);
  while (childIndex.isValid())
  {
    c += descendantCount(childIndex);
//     kDebug() << "(while) c:" << c << childIndex << "childRow" << childRow << "sourceIndex" << sourceIndex << "sourceRowCount" << q->sourceModel()->rowCount(sourceIndex);
// Taking this out of the index() fixed the loop. Figure out why.
// Duh. It's obvious. ++childIndex would also have worked. I handled the first row already out of the loop, and I was processing it a second time.
    childRow++;
//     kDebug() << "next row" << childRow;
    childIndex = q->sourceModel()->index(childRow, 0, sourceIndex);
//     kDebug() << "next" << childIndex;
  }

//   kDebug() << "(final) c:" << c;

  m_descendantsCount.insert( sourceIndex.internalId(), c );

  return c;

//   ClientSideEntityStorage::Iterator i(colId);
//   while (i.hasNext())
//   {
//     qint64 entId = i.next();
//     int entityType = m_clientSideEntityStorage->entityTypeForInternalIdentifier(entId);
//     if ( ClientSideEntityStorage::CollectionType == entityType )
//     {
//       c += descendantCount(entId);
//     }
//   }


}

// bool DescendantEntitiesProxyModel::hasChildren(const QModelIndex &idx) const
// {
//   return rowCount(idx) > 0;
// }

QModelIndex DescendantEntitiesProxyModel::index(int r, int c, const QModelIndex& parent) const
{
//   kDebug() << r << c << parent;
//   if (!idx.isValid())
//     return QModelIndex();
Q_D( const DescendantEntitiesProxyModel );

  if ( (r < 0) || (c < 0) || (c > 0) )
    return QModelIndex();

  if ( r > d->descendantCount(parent) )
    return QModelIndex();

  // TODO: Use is decended instead.
  if (parent.isValid())
    return QModelIndex();

  return createIndex(r, c);
  QModelIndex sourceParentIndex = mapToSource(parent);
//   kDebug() << sourceParentIndex;

//   if (!sourceIndex.isValid())
//     return QModelIndex();

//   if (d->isDescended(sourceParentIndex))
//   {
//     // If we're descending into this index, then it should not have
//     // any children. It's children will become its siblings.
//     return QModelIndex();
//   }

  // ???
  // If we're descending into this index, then it should not have
  // any children. It's children will become its siblings.
  // Only the root descend index has child indexes.
//   kDebug() << sourceParentIndex << d->m_rootDescendIndex;
  if (sourceParentIndex != d->m_rootDescendIndex)
    return QModelIndex();

int dc = d->descendantCount(sourceParentIndex);
int dr = d->descendedRow(sourceParentIndex);
//   kDebug() << r << dc << dr;
  if (r >= dc ) //d->descendantCount(sourceParentIndex) )
    return QModelIndex();

//   if (!isDescended( sourceIndex ) )
//   {
//     return mapFromSource(sourceModel()->index(r, c, sourceIndex));
//   }

//   bool ok;
  QModelIndex idx = d->findSourceIndexForRow(r, sourceParentIndex);
//   kDebug() << "idx" << idx;
  if (!idx.isValid())
    return QModelIndex();
  // EntityId role?
//   Collection col = sourceModel()->data(idx, EntityTreeModel::CollectionRole).value<Collection>();
//   kDebug() << ok << createIndex(r, c, reinterpret_cast<void*>(col.id()));
//   if (ok)
//     kDebug() << r << c << col.id() << col.name();
//     kDebug() << createIndex(r, c, reinterpret_cast<void*>(col.id()));
    return createIndex(r, c, reinterpret_cast<void*>(idx.internalId()));
//   return QModelIndex();
}

QModelIndex DescendantEntitiesProxyModel::parent(const QModelIndex& proxyIndex) const
{

//   QModelIndex sourceIndex = mapToSource(proxyIndex);
//
//   if (isDescended(sourceIndex))
//   {
//     // TODO: make sure m_rootDescendIndex is persistant.
//     return m_rootDescendIndex;
//   } else {
//     return QModelIndex();
// //     return mapFromSource(sourceModel()->parent(sourceIndex));
//   }

//     kDebug() << proxyIndex;
  return QModelIndex();
//   int column = 0;
//   int row =
//   return createIndex( row, column, reinterpret_cast<void *>( m_rootDescendCollection.id() ) );

}

int DescendantEntitiesProxyModel::columnCount(const QModelIndex&) const
{
//     kDebug();
  return 1;
}


void DescendantEntitiesProxyModel::setDisplayAncestorData(bool display, const QString &sep)
{
  Q_D(DescendantEntitiesProxyModel);
  d->m_displayAncestorData = display;
  d->m_ancestorSeparator = sep;
}

bool DescendantEntitiesProxyModel::displayAncestorData() const
{
  Q_D(const DescendantEntitiesProxyModel);
  return d->m_displayAncestorData;
}

QString DescendantEntitiesProxyModel::ancestorSeparator() const
{
  Q_D(const DescendantEntitiesProxyModel);
  if (!d->m_displayAncestorData)
    return QString();
  return d->m_ancestorSeparator;
}


#include "moc_descendantentitiesproxymodel.cpp"

