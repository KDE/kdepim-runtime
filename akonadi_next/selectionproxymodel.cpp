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

#include "selectionproxymodel.h"

#include "entitytreemodel.h"

#include <KDebug>

using namespace Akonadi;

namespace Akonadi
{

class SelectionProxyModelPrivate
{
public:
  SelectionProxyModelPrivate(SelectionProxyModel *model)
    : q_ptr(model)
  {

  }

  Q_DECLARE_PUBLIC(SelectionProxyModel)
  SelectionProxyModel *q_ptr;

  QItemSelectionModel *m_selectionModel;
  // TODO: Make this persistent.
  QModelIndexList m_rootIndexList;

  QList<QAbstractProxyModel *> m_proxyChain;

  void sourceRowsAboutToBeInserted(const QModelIndex &, int start, int end);
  void sourceRowsInserted(const QModelIndex &, int start, int end);
  void sourceRowsAboutToBeRemoved(const QModelIndex &, int start, int end);
  void sourceRowsRemoved(const QModelIndex &, int start, int end);
//   void sourceRowsAboutToBeMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destParent, int destRow);
//   void sourceRowsMoved(const QModelIndex &parent, int start, int end, const QModelIndex &destParent, int destRow);
  void sourceModelAboutToBeReset();
  void sourceModelReset();
  void sourceLayoutAboutToBeChanged();
  void sourceLayoutChanged();
  void sourceDataChanged(const QModelIndex &,const QModelIndex &);

  void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected );


  bool isDescendantOf(QModelIndexList &list, const QModelIndex &idx);
  QPair<int, int> getRootRange(const QModelIndex &sourceParent, int start, int end);
  void createProxyChain();
  QModelIndexList getRootSelectedIndexes(const QModelIndexList &list);
  bool isInModel(const QModelIndex &sourceIndex) const;
  bool ancestorIsSelected(const QModelIndex &selectedIndex) const;
  QModelIndex selectionIndexToSourceIndex(const QModelIndex &index) const;

  QHash <QPersistentModelIndex, QList<QPersistentModelIndex> > childIndexes;

  mutable QHash< void *, QPersistentModelIndex> m_map;

};

}

void SelectionProxyModelPrivate::sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
  Q_Q(SelectionProxyModel);


  QPair<int, int> range = getRootRange(topLeft.parent(), topLeft.row(), bottomRight.row());
  
  if (isInModel(topLeft))
  {
    QModelIndex proxyTopLeft = q->mapFromSource(topLeft);
    QModelIndex proxyBottomRight = q->mapFromSource(bottomRight);
  
//     if (!proxyTopLeft.parent().isValid())
//     {
//       emit q->dataChanged(proxyTopLeft, proxyBottomRight);
// 
//       return;
//     }
  
  // If topLeft to bottomRight are not part of the selection, then if
  // topLeft is in the model, bottomRight is too.

    emit q->dataChanged(proxyTopLeft, proxyBottomRight);
  }
}


void SelectionProxyModelPrivate::sourceLayoutAboutToBeChanged()
{
  Q_Q(SelectionProxyModel);
  emit q->layoutAboutToBeChanged();
}

void SelectionProxyModelPrivate::sourceLayoutChanged()
{
  Q_Q(SelectionProxyModel);
  emit q->layoutChanged();
}

void SelectionProxyModelPrivate::sourceModelAboutToBeReset()
{
  Q_Q(SelectionProxyModel);
  q->beginResetModel();
}

void SelectionProxyModelPrivate::sourceModelReset()
{
  Q_Q(SelectionProxyModel);

  // No need to try to refill this. When the model is reset it doesn't have a meaningful selection anymore,
  // but when it gets one we'll be notified anyway.
  m_rootIndexList.clear();
  q->endResetModel();
}

QPair<int, int> SelectionProxyModelPrivate::getRootRange(const QModelIndex &sourceParent, int start, int end)
{
  int listStart = -1;
  int listEnd = -1;

  int tracker = 0;
  foreach(const QModelIndex &idx, m_rootIndexList)
  {
    if (listStart == -1)
    {
      if (idx.row() > start && idx.parent() == sourceParent)
      {
        listStart = tracker;
      }
    }
    if (idx.row() < end && m_rootIndexList.value(tracker -1).parent() == sourceParent)
    {
      listEnd = tracker -1;
      break;
    }
    tracker++;

  }
  return qMakePair(listStart, listEnd);
}

void SelectionProxyModelPrivate::sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
  Q_Q(SelectionProxyModel);

  QModelIndex sourceStart = q->sourceModel()->index(start, 0, parent);

  if (isInModel(sourceStart))
  {
    // ?? A a selected index is getting new neighbours ??
    if (m_rootIndexList.contains(sourceStart))
      return;
      
    q->beginInsertRows(q->mapFromSource(parent), start, end);    
  }
}

void SelectionProxyModelPrivate::sourceRowsInserted(const QModelIndex &parent, int start, int end)
{
  Q_Q(SelectionProxyModel);

  QModelIndex sourceStart = q->sourceModel()->index(start, 0, parent);

  if (isInModel(sourceStart))
  {
    if (m_rootIndexList.contains(sourceStart))
      return;

    q->endInsertRows();
  }
}

void SelectionProxyModelPrivate::sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
  Q_Q(SelectionProxyModel);

  QModelIndex sourceStart = q->sourceModel()->index(start, 0, parent);
  if (isInModel(sourceStart))
  {
    if (m_rootIndexList.contains(sourceStart))
    {
      QPair<int, int> range = getRootRange(parent, start, end);
      if (range.first != -1 && range.second != -1)
      {
        start = range.first;
        end = range.second;
      }
    }
    q->beginInsertRows(q->mapFromSource(parent), start, end);
  }
}

void SelectionProxyModelPrivate::sourceRowsRemoved(const QModelIndex &parent, int start, int end)
{
  Q_Q(SelectionProxyModel);

  QModelIndex sourceStart = q->sourceModel()->index(start, 0, parent);
  if (isInModel(sourceStart))
  {
    if (m_rootIndexList.contains(sourceStart))
    {
      QPair<int, int> range = getRootRange(parent, start, end);
      if ( -1 == range.first || -1 == range.second )
      {
        return;
      }
      start = range.first;
      end = range.second;
    }
    q->beginInsertRows(q->mapFromSource(parent), start, end);
  }
}

bool SelectionProxyModelPrivate::isDescendantOf(QModelIndexList &list, const QModelIndex &idx)
{
  QModelIndex parent = idx.parent();
  while (parent.isValid())
  {
    if (list.contains(parent))
      return true;
    parent = parent.parent();
  }
  return false;
}

void SelectionProxyModelPrivate::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected )
{
  Q_Q(SelectionProxyModel);

  QModelIndexList newIndexes;
  foreach(const QModelIndex &idx, getRootSelectedIndexes(selected.indexes()))
  {
    if (0 != idx.column())
      continue;

    QModelIndex srcIdx = selectionIndexToSourceIndex(idx);

    if (!m_rootIndexList.contains(srcIdx))
      newIndexes << srcIdx;
  }

  foreach (const QModelIndex &idx, getRootSelectedIndexes(deselected.indexes()))
  {
    if (0 != idx.column())
      continue;

    QModelIndex srcIdx = selectionIndexToSourceIndex(idx);

    if (isInModel(srcIdx))
    {

      int row = m_rootIndexList.indexOf(srcIdx);

      if (row < 0)
        continue;

      q->beginRemoveRows(QModelIndex(), row, row);
      m_rootIndexList.removeOne(srcIdx);
      q->endRemoveRows();
    }
  }

  // Now that the deselected indexes have been removed, some of their children may need to be reinserted
  // into the model.
  foreach(const QModelIndex &idx, m_selectionModel->selectedIndexes())
  {
    if (0 != idx.column())
      continue;

    QModelIndex srcIdx = selectionIndexToSourceIndex(idx);

    if (isDescendantOf(m_rootIndexList, idx))
      continue;
    
    if (!m_rootIndexList.contains(srcIdx) && !newIndexes.contains(srcIdx))    
      newIndexes << srcIdx;       
  }


  if (newIndexes.size() > 0)
  {
    // Remove any descendants of new indexes. They're now represented by their parents.
    QMutableListIterator<QModelIndex> i(newIndexes);
    while (i.hasNext())
    {
      QModelIndex idx = i.next();
      QModelIndex parent = idx.parent();
      while (parent.isValid())
      {
        if (m_rootIndexList.contains(parent))
        {
          int row = m_rootIndexList.indexOf(idx);

          // A child of an already selected row was selected. Do nothing.
          if (-1 == row)
          {
            i.remove();
            break;
          }
        }
        parent = parent.parent();
      }
    }

    QMutableListIterator<QModelIndex> i2(m_rootIndexList);
    while (i2.hasNext())
    {
      QModelIndex idx = i2.next();
      QModelIndex parent = idx.parent();
      while (parent.isValid())
      {
        if (newIndexes.contains(parent))
        {
          int row = m_rootIndexList.indexOf(idx);
          q->beginRemoveRows(QModelIndex(), row, row);
          i2.remove();
          q->endRemoveRows();
          break;
        }
        parent = parent.parent();
      }
    }

    if (newIndexes.size() == 0)
    {
      return;
    }
  
    // TODO: Preserve ordering from sourceModel.
    int row = m_rootIndexList.size();
    q->beginInsertRows(QModelIndex(), row, row + newIndexes.size() - 1 );
    foreach( const QModelIndex &idx, newIndexes)
    {
      m_rootIndexList << idx;
    }
    q->endInsertRows();
  }
}

void SelectionProxyModelPrivate::createProxyChain()
{
  Q_Q(SelectionProxyModel);

  QAbstractItemModel *model = const_cast<QAbstractItemModel *>(m_selectionModel->model());
  QAbstractProxyModel *nextProxyModel;
  QAbstractProxyModel *proxyModel = qobject_cast<QAbstractProxyModel*>(model);

  QAbstractItemModel *rootModel;
  while (proxyModel)
  {

    if (proxyModel == q->sourceModel())
      break;

    m_proxyChain << proxyModel;

    nextProxyModel = qobject_cast<QAbstractProxyModel*>(proxyModel->sourceModel());

    if (!nextProxyModel)
    {
      rootModel = qobject_cast<QAbstractItemModel*>(proxyModel->sourceModel());
      // It's the final model in the chain, so it is necessarily the sourceModel.
      Q_ASSERT(rootModel == q->sourceModel());
      break;
    }
    proxyModel = nextProxyModel;
  }
}

QModelIndexList SelectionProxyModelPrivate::getRootSelectedIndexes(const QModelIndexList &list)
{
  QModelIndexList retList;
  QListIterator<QModelIndex> i(list);
  bool skipToNext = false;
  while (i.hasNext())
  {
    const QModelIndex selectedIdx = i.next();
    QModelIndex idx = selectedIdx;
    while (idx.isValid())
    {
      if (retList.contains(idx))
      {
        skipToNext = true;
        break;
      }

      if (!idx.parent().isValid())
      {
        break;
      }
      
      idx = idx.parent();      
    }
    if (skipToNext)
    {
      skipToNext = false;
      continue;
    }
    retList << selectedIdx;      
  }
  return retList;
}

QModelIndex SelectionProxyModelPrivate::selectionIndexToSourceIndex(const QModelIndex &index) const
{
  QModelIndex seekIndex = index;
  QListIterator<QAbstractProxyModel*> i(m_proxyChain);
  i.toBack();
  QAbstractProxyModel *proxy;
  while (i.hasPrevious())
  {
    proxy = i.previous();
    seekIndex = proxy->mapToSource(seekIndex);
  }
  return seekIndex;
}

bool SelectionProxyModelPrivate::isInModel(const QModelIndex &sourceIndex) const
{
  if (m_rootIndexList.contains(sourceIndex))
    return true;
    
  QModelIndex seekIndex = sourceIndex;
  while (seekIndex.isValid())
  {
    if (m_rootIndexList.contains(seekIndex))
      return true;
    seekIndex = seekIndex.parent();
  }
  return false;
}

bool SelectionProxyModelPrivate::ancestorIsSelected(const QModelIndex &selectedIndex) const
{
  QModelIndex seekIndex = selectionIndexToSourceIndex(selectedIndex);
  return isInModel(seekIndex);
}

SelectionProxyModel::SelectionProxyModel(QItemSelectionModel *selectionModel, QObject *parent)
  : AbstractProxyModel(parent), d_ptr(new SelectionProxyModelPrivate(this))
{
  Q_D(SelectionProxyModel);

  d->m_selectionModel = selectionModel;

  connect( d->m_selectionModel, SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
      SLOT( selectionChanged( const QItemSelection &, const QItemSelection & ) ) );

}

SelectionProxyModel::~SelectionProxyModel()
{
  delete d_ptr;
}

void SelectionProxyModel::setSourceModel( QAbstractItemModel *sourceModel )
{
  Q_D(SelectionProxyModel);

  AbstractProxyModel::setSourceModel(sourceModel);
  d->createProxyChain();
  d->m_rootIndexList = d->getRootSelectedIndexes(d->m_selectionModel->selectedIndexes());

}

QModelIndex SelectionProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
  Q_D(const SelectionProxyModel);

  if (!proxyIndex.isValid())
    return QModelIndex();

  QModelIndex idx = d->m_map.value(proxyIndex.internalPointer());
  return idx;

}

QModelIndex SelectionProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
  Q_D( const SelectionProxyModel );
  int row = d->m_rootIndexList.indexOf( sourceIndex );
  if ( row != -1 )
  {
    d->m_map.insert(sourceIndex.internalPointer(), QPersistentModelIndex(sourceIndex));
    return createIndex( row, sourceIndex.column(), sourceIndex.internalPointer() );
  } else if ( d->isInModel( sourceIndex ) )
  {
    d->m_map.insert(sourceIndex.internalPointer(), QPersistentModelIndex(sourceIndex));
    return createIndex( sourceIndex.row(), sourceIndex.column(), sourceIndex.internalPointer() );
  }
  return QModelIndex();
}

int SelectionProxyModel::rowCount(const QModelIndex &index) const
{
  Q_D(const SelectionProxyModel);

  if (!index.isValid())
    return d->m_rootIndexList.size();

  QModelIndex srcIndex = mapToSource(index);

  if (!d->isInModel(srcIndex))
    return 0;

  return sourceModel()->rowCount(srcIndex);
}

QModelIndex SelectionProxyModel::index(int row, int column, const QModelIndex &parent) const
{
  Q_D(const SelectionProxyModel);
  if (!hasIndex(row, column, parent))
    return QModelIndex();

  if (!parent.isValid())
  {

    QModelIndex idx = d->m_rootIndexList.at( row );
    d->m_map.insert(idx.internalPointer(), idx);
    return createIndex(row, column, idx.internalPointer());
  } else
  {
    QModelIndex sourceParent = mapToSource(parent);
    QModelIndex sourceIndex = sourceModel()->index(row, column, sourceParent);
    return mapFromSource(sourceIndex);
  }
}

QModelIndex SelectionProxyModel::parent(const QModelIndex &index) const
{
  QModelIndex sourceIndex = mapToSource(index);
  QModelIndex proxyParent = mapFromSource(sourceIndex.parent());
  return proxyParent;
}

Qt::ItemFlags SelectionProxyModel::flags( const QModelIndex &index ) const
{
  if (!index.isValid())
    return 0;

  QModelIndex srcIndex = mapToSource(index);
  return sourceModel()->flags(srcIndex);
}

QVariant SelectionProxyModel::data( const QModelIndex & index, int role ) const
{
  if (index.isValid())
  {
    QModelIndex idx = mapToSource(index);
    return idx.data(role);
  }
  return QVariant();
}

QVariant SelectionProxyModel::headerData( int section, Qt::Orientation orientation, int role  ) const
{
  return sourceModel()->headerData(section, orientation, role);
}

bool SelectionProxyModel::hasChildren ( const QModelIndex & parent) const
{
  return rowCount(parent) > 0;
}

int SelectionProxyModel::columnCount(const QModelIndex &index) const
{
  return sourceModel()->columnCount(mapToSource(index));
}

#include "moc_selectionproxymodel.cpp"
