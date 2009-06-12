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
    : q_ptr(model),
      m_startWithChildTrees(false),
      m_omitDescendants(false),
      m_includeAllSelected(false),
      m_rowBlocksToRemove(0)
  {

  }

  Q_DECLARE_PUBLIC(SelectionProxyModel)
  SelectionProxyModel *q_ptr;

  QItemSelectionModel *m_selectionModel;
  QList<QPersistentModelIndex> m_rootIndexList;

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

  QModelIndexList toNonPersistent(const QList<QPersistentModelIndex> &list) const;

  /**
    Return true if @p idx is a descendant of one of the indexes in @p list.
    Note that this returns false if @p list contains @p idx.
  */
  bool isDescendantOf(QModelIndexList &list, const QModelIndex &idx) const;

  /**
    Returns the range in the proxy model corresponding to the range in the source model
    covered by @sourceParent, @p start and @p end.
  */
  QPair<int, int> getRootRange(const QModelIndex &sourceParent, int start, int end) const;

  /**
  Traverses the proxy models between the selectionModel and the sourceModel. Creating a chain as it goes.
  */
  void createProxyChain();

  /**
  Returns a selection in which no descendants of selected indexes are also themselves selected.
  For example,
  @code
    A
    - B
    C
    D
  @endcode
  If A, B and D are selected in @p selection, the returned selection contains only A and D.
  */
  QItemSelection getRootRanges(const QItemSelection &selection) const;

  /**
    Returns the indexes in @p selection which are not already part of the proxy model.
  */
  QModelIndexList getNewIndexes(const QItemSelection &selection) const;

  /**
    Determines the correct location to insert @p index into @p list.
  */
  int getTargetRow(const QModelIndexList &list, const QModelIndex &index) const;

  /**
    Regroups @p list into contiguous groups with the same parent.
  */
  QList<QPair<QModelIndex, QModelIndexList> > regroup(const QModelIndexList &list) const;

  /**
    Inserts the indexes in @p list into the proxy model.
  */
  void insertionSort(const QModelIndexList &list);

  /**
    Returns true if @p sourceIndex or one of its ascendants is already part of the proxy model.
  */
  bool isInModel(const QModelIndex &sourceIndex) const;

  /**
  Converts an index in the selection model to an index in the source model.
  */
  QModelIndex selectionIndexToSourceIndex(const QModelIndex &index) const;

  /**
    Returns the total number of children (but not descendants) of all of the indexes in @p list.
  */
  int childrenCount(const QModelIndexList &list) const;

  // Used to map children of indexes in the source model to indexes in the proxy model.
  // TODO: Find out if this breaks when indexes are modified because of higher siblings move/insert/remove
  mutable QHash< void *, QPersistentModelIndex> m_map;

  bool m_startWithChildTrees;
  bool m_omitDescendants;
  bool m_includeAllSelected;

  // Number of separate blocks that need to be removed as a result of sourceRowsRemoved.
  int m_rowBlocksToRemove;
};

}

QModelIndexList SelectionProxyModelPrivate::toNonPersistent(const QList<QPersistentModelIndex> &list) const
{
  QModelIndexList returnList;
  QList<QPersistentModelIndex>::const_iterator it;
  for (it = list.constBegin(); it != list.constEnd(); ++it)
    returnList << *it;

  return returnList;
}

void SelectionProxyModelPrivate::sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
  Q_Q(SelectionProxyModel);

  QModelIndexList list = toNonPersistent(m_rootIndexList);
  if (!m_rootIndexList.contains(topLeft) && isInModel(topLeft))
  {
    // The easy case. A contiguous block not at the root of our model.
    QModelIndex proxyTopLeft = q->mapFromSource(topLeft);
    QModelIndex proxyBottomRight = q->mapFromSource(bottomRight);
    // If we're showing only chilren in our model and a grandchild is
    // changed, this will be invalid.
    if (!proxyTopLeft.isValid())
      return;
    emit q->dataChanged(proxyTopLeft, proxyBottomRight);
    return;
  }

  // We're not showing the m_rootIndexList, so we don't care if they change.
  if (m_startWithChildTrees)
    return;

  // The harder case. Parts of the reported changed range are part of
  // the model if they are in m_rootIndexList. Emit signals in blocks.

  const int leftColumn = topLeft.column();
  const int rightColumn = bottomRight.column();
  const QModelIndex parent = topLeft.parent();
  int startRow = topLeft.row();
  for (int row = startRow; row <= bottomRight.row(); ++row)
  {
    QModelIndex idx = q->sourceModel()->index(row, leftColumn, parent);
    if (m_rootIndexList.contains(idx))
    {
      startRow = row;
      ++row;
      idx = q->sourceModel()->index(row, leftColumn, parent);
      while(m_rootIndexList.contains(idx))
      {
        ++row;
        idx = q->sourceModel()->index(row, leftColumn, parent);
      }
      --row;
      QModelIndex sourceTopLeft = q->sourceModel()->index(startRow, leftColumn, parent);
      QModelIndex sourceBottomRight = q->sourceModel()->index(row, rightColumn, parent);
      QModelIndex proxyTopLeft = q->mapFromSource(sourceTopLeft);
      QModelIndex proxyBottomRight = q->mapFromSource(sourceBottomRight);

      emit q->dataChanged(proxyTopLeft, proxyBottomRight);
    }
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

QPair<int, int> SelectionProxyModelPrivate::getRootRange(const QModelIndex &sourceParent, int start, int end) const
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

  if (isInModel(parent))
  {
    // The easy case.
    q->beginInsertRows(q->mapFromSource(parent), start, end);
    return;
  }

  QModelIndex sourceStart = q->sourceModel()->index(start, 0, parent);
  if (m_startWithChildTrees && m_rootIndexList.contains(sourceStart))
  {
    // Another fairly easy case.
    const int proxyStartRow = q->mapFromSource(sourceStart).row();
    q->beginInsertRows(QModelIndex(), proxyStartRow, proxyStartRow + (end - start));
    return;
  }

}

void SelectionProxyModelPrivate::sourceRowsInserted(const QModelIndex &parent, int start, int end)
{
  Q_Q(SelectionProxyModel);
  Q_UNUSED(end);

  if (isInModel(parent))
  {
    q->endInsertRows();
  }

  QModelIndex sourceStart = q->sourceModel()->index(start, 0, parent);
  if (m_startWithChildTrees && m_rootIndexList.contains(sourceStart))
  {
    q->endInsertRows();
  }
}

void SelectionProxyModelPrivate::sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
  Q_Q(SelectionProxyModel);

  if (!parent.isValid())
  {
    // Might have to remove some stuff from our model.
    int startRow;
    const int column = 0;
    for (int row = start; row <= end; ++row)
    {
      QModelIndex idx = q->sourceModel()->index(row, column);
      if (m_rootIndexList.contains(idx))
      {
        startRow = row;
        ++row;
        idx = q->sourceModel()->index(row, column);
        while(m_rootIndexList.contains(idx))
        {
          ++row;
          idx = q->sourceModel()->index(row, column);
        }
        --row;
        int proxyStartRow = q->mapFromSource(q->sourceModel()->index(startRow, column)).row();
        m_rowBlocksToRemove++;
        q->beginRemoveRows(QModelIndex(), proxyStartRow, proxyStartRow + (row - startRow));
      }
    }
  }

  if (isInModel(parent))
  {
    q->beginRemoveRows(q->mapFromSource(parent), start, end);
    return;
  }

  QModelIndex sourceStart = q->sourceModel()->index(start, 0, parent);
  if (m_startWithChildTrees && m_rootIndexList.contains(sourceStart))
  {
    const int proxyStartRow = q->mapFromSource(sourceStart).row();
    q->beginRemoveRows(QModelIndex(), proxyStartRow, proxyStartRow + (end - start));
    return;
  }
}

void SelectionProxyModelPrivate::sourceRowsRemoved(const QModelIndex &parent, int start, int end)
{
  Q_Q(SelectionProxyModel);
  Q_UNUSED(end)

  if (!parent.isValid())
  {
    // Rows to remove are now invalid indexes.
    QMutableListIterator<QPersistentModelIndex> it(m_rootIndexList);
    while (it.hasNext())
    {
      QPersistentModelIndex idx = it.next();
      if (!idx.isValid())
      {
        it.remove();
      }
    }
    if (m_rowBlocksToRemove > 0)
    {
      --m_rowBlocksToRemove;
      q->endRemoveRows();
    }
  }

  if(isInModel(parent))
  {
    q->endRemoveRows();
    return;
  }

  QModelIndex sourceStart = q->sourceModel()->index(start, 0, parent);
  if (m_startWithChildTrees && m_rootIndexList.contains(sourceStart))
  {
    q->endRemoveRows();
    return;
  }
}

bool SelectionProxyModelPrivate::isDescendantOf(QModelIndexList &list, const QModelIndex &idx) const
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

QModelIndexList SelectionProxyModelPrivate::getNewIndexes(const QItemSelection &selection) const
{
  QModelIndexList indexes;
  const int column = 0;

  foreach( const QItemSelectionRange &range, selection )
  {
    QModelIndex newIndex = range.topLeft();

    if (newIndex.column() != 0)
      continue;

    for(int row = newIndex.row(); row <= range.bottom(); ++row)
    {
      newIndex = newIndex.sibling(row, column);

      QModelIndex sourceNewIndex = selectionIndexToSourceIndex(newIndex);

      int startRow = m_rootIndexList.indexOf(sourceNewIndex);
      if ( startRow > 0 )
      {
        continue;
      }

      indexes << sourceNewIndex;
    }
  }
  return indexes;
}

void SelectionProxyModelPrivate::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected )
{
  Q_Q(SelectionProxyModel);

  // Any deselected indexes in the m_rootIndexList are removed. Then, any
  // indexes in the selected range which are not a descendant of one of the already selected indexes
  // are inserted into the model.
  //
  // All ranges from the selection model need to be split into individual rows. Ranges which are contiguous in
  // the selection model may not be contiguous in the source model if there's a sort filter proxy model in the chain.
  //
  // Some descendants of deselected indexes may still be selected. The ranges in m_selectionModel->selection()
  // are examined. If any of the ranges are descendants of one of the
  // indexes in deselected, they are added to the ranges to be inserted into the model.
  //
  // The new indexes are inserted in sorted order.

  QModelIndexList removeIndexes;

  const int column = 0;
  foreach( const QItemSelectionRange &range, deselected )
  {
    QModelIndex removeIndex = range.topLeft();

    if (removeIndex.column() != 0)
      continue;

    for(int row = removeIndex.row(); row <= range.bottom(); ++row)
    {
      removeIndex = removeIndex.sibling(row, column);

      removeIndexes << removeIndex;

      QModelIndex sourceRemoveIndex = selectionIndexToSourceIndex(removeIndex);

      int startRow = m_rootIndexList.indexOf(sourceRemoveIndex);

      if ( startRow < 0 )
        continue;

      if(m_startWithChildTrees)
      {
        int _start = 0;
        for (int i = 0 ; i < startRow; ++i)
        {
          _start += q->sourceModel()->rowCount(m_rootIndexList.at(i));
        }
        int rowCount = q->sourceModel()->rowCount(m_rootIndexList.at(startRow));

        q->beginRemoveRows(QModelIndex(), _start, _start + rowCount - 1);
        m_rootIndexList.removeAt(startRow);
        q->endRemoveRows();
      } else {
        q->beginRemoveRows(QModelIndex(), startRow, startRow);
        m_rootIndexList.removeAt(startRow);
        q->endRemoveRows();
      }
    }
  }

  QItemSelection newRanges = getRootRanges(selected);

  if (!m_includeAllSelected)
  {
    QMutableListIterator<QItemSelectionRange> i(newRanges);
    while (i.hasNext())
    {
      QItemSelectionRange range = i.next();
      QModelIndex topLeft = range.topLeft();
      QModelIndexList list = toNonPersistent(m_rootIndexList);
      if (isDescendantOf(list, topLeft))
      {
        i.remove();
      }
    }
  }

  QModelIndexList newIndexes;

  newIndexes << getNewIndexes(newRanges);

  QItemSelection additionalRanges;
  if ( !m_includeAllSelected )
  {
    foreach( const QItemSelectionRange &range, m_selectionModel->selection() )
    {
      QModelIndex topLeft = range.topLeft();
      if (isDescendantOf(removeIndexes, topLeft))
      {
        QModelIndexList list = toNonPersistent(m_rootIndexList);
        if ( !isDescendantOf(list, topLeft) && !isDescendantOf(newIndexes, topLeft) )
          additionalRanges << range;
      }

      int row = m_rootIndexList.indexOf(topLeft);
      if (row >= 0)
      {
        if (isDescendantOf(newIndexes, topLeft))
        {
          q->beginRemoveRows(QModelIndex(), row, row);
          m_rootIndexList.removeAt(row);
          q->endRemoveRows();
        }
      }
    }
    // A
    // - B
    // - - C
    // - - D
    // - E
    // If A, B, C and E are selected, and A is deselected, B and E need to be inserted into the model, but not C.
    // Currently B, C and E are in additionalRanges. Ranges which are descendant of other ranges in the list need
    // to be removed.

    additionalRanges = getRootRanges(additionalRanges);

  }

  // TODO: Use QSet<QModelIndex> instead to simplify this stuff.
  foreach(const QModelIndex &idx, getNewIndexes(additionalRanges))
  {
    if (!newIndexes.contains(idx))
      newIndexes << idx;
  }
  if (newIndexes.size() > 0)
    insertionSort(newIndexes);
}

int SelectionProxyModelPrivate::getTargetRow(const QModelIndexList &list, const QModelIndex &index) const
{
  // What's going on?
  // Consider a tree like
  //
  // A
  // - B
  // - - C
  // - - - D
  // - E
  // - F
  // - - G
  // - - - H
  // - I
  // - - J
  // - K
  //
  // If D, E and J are already selected, and H is newly selected, we need to put H between E and J in the proxy model.
  // To figure that out, we create a list for each already selected index of its ancestors. Then,
  // we climb the ancestors of H until we reach an index with siblings which have a descendant
  // selected (F above has siblings B, E and I which have descendants which are already selected).
  // Those child indexes are traversed to find the right sibling to put F beside.
  //
  // i.e., new items are inserted in the expected location.


  QList<QModelIndexList> rootAncestors;
  foreach(const QModelIndex root, list)
  {
    QModelIndexList ancestors;
    ancestors << root;
    QModelIndex parent = root.parent();
    while (parent.isValid())
    {
      ancestors.prepend(parent);
      parent = parent.parent();
    }
    ancestors.prepend(QModelIndex());
    rootAncestors << ancestors;
  }

  QModelIndex parent = index;
  QModelIndex prevParent;

  int firstCommonParent = -1;
  int bestParentRow = -1;
  while (parent.isValid())
  {
    prevParent = parent;
    parent = parent.parent();
    for (int i = 0; i < rootAncestors.size(); ++i )
    {
      QModelIndexList ancestorList = rootAncestors.at(i);

      int parentRow = ancestorList.indexOf(parent);

      if (parentRow < 0)
        continue;

      if (parentRow > bestParentRow)
      {
        firstCommonParent = i;
        bestParentRow = parentRow;
      }
    }

    if (firstCommonParent >= 0)
      break;
  }

  if (firstCommonParent < 0)
    return 0;

  if ( m_includeAllSelected )
    return firstCommonParent;

  // commonParent is now the index A in the example above.
  QModelIndex commonParent = rootAncestors.at(firstCommonParent).at( bestParentRow + 1 );

  if ( prevParent.row() < commonParent.row() )
    return firstCommonParent;

  int siblingOffset = 1;

  if (rootAncestors.size() > firstCommonParent + siblingOffset )
  {
    QModelIndex nextParent = rootAncestors.at(firstCommonParent + siblingOffset ).at(bestParentRow);
    while (nextParent == commonParent.parent())
    {
      QModelIndex nextSibling = rootAncestors.at(firstCommonParent + siblingOffset ).at(bestParentRow + 1);
      if (prevParent.row() < nextSibling.row())
      {
        break;
      }
      siblingOffset++;
      if (rootAncestors.size() <= firstCommonParent + siblingOffset )
        break;
      nextParent = rootAncestors.at(firstCommonParent + siblingOffset ).at(bestParentRow);
    }
  }
  return firstCommonParent + siblingOffset;
}

QList<QPair<QModelIndex, QModelIndexList> > SelectionProxyModelPrivate::regroup(const QModelIndexList &list) const
{
  QList<QPair<QModelIndex, QModelIndexList> > groups;

  // TODO: implement me.
//   foreach (const QModelIndex idx, list)
//   {
//
//   }

  return groups;
}

void SelectionProxyModelPrivate::insertionSort(const QModelIndexList &list)
{
  Q_Q(SelectionProxyModel);

  // TODO: regroup indexes in list into contiguous ranges with the same parent.
//   QList<QPair<QModelIndex, QModelIndexList> > regroup(list);
  // where pair.first is a parent, and pair.second is a list of contiguous indexes of that parent.
  // That will allow emitting new rows in ranges rather than one at a time.

  foreach(const QModelIndex &newIndex, list)
  {
    if ( m_startWithChildTrees )
    {
      QModelIndexList list = toNonPersistent(m_rootIndexList);
      int row = getTargetRow(list, newIndex);
      int rowCount = q->sourceModel()->rowCount(newIndex);
      if ( rowCount > 0 )
      {
        q->beginInsertRows(QModelIndex(), row, row + rowCount - 1);
        m_rootIndexList.insert(row, newIndex);
        q->endInsertRows();
      }
    } else {
      QModelIndexList list = toNonPersistent(m_rootIndexList);
      int row = getTargetRow(list, newIndex);
      q->beginInsertRows(QModelIndex(), row, row);
      m_rootIndexList.insert(row, newIndex);
      q->endInsertRows();
    }
  }
  return;
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

QItemSelection SelectionProxyModelPrivate::getRootRanges(const QItemSelection &selection) const
{
  QModelIndexList parents;
  QItemSelection rootSelection;
  QListIterator<QItemSelectionRange> i(selection);
  while (i.hasNext())
  {
    parents << i.next().topLeft();
  }

  i.toFront();

  while (i.hasNext())
  {
    QItemSelectionRange range = i.next();
    if (isDescendantOf(parents, range.topLeft()))
      continue;
    rootSelection << range;
  }
  return rootSelection;
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
  {
    if ( m_startWithChildTrees )
      return false;
    return true;
  }

  QModelIndex seekIndex = sourceIndex;
  while (seekIndex.isValid())
  {
    if (m_rootIndexList.contains(seekIndex))
    {
      return true;
    }

    seekIndex = seekIndex.parent();
  }
  return false;
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

void SelectionProxyModel::setOmitDescendants(bool omitDescendants)
{
  Q_D(SelectionProxyModel);

  d->m_omitDescendants = omitDescendants;
}

void SelectionProxyModel::setStartWithChildTrees(bool startWithChildTrees)
{
  Q_D(SelectionProxyModel);

  d->m_startWithChildTrees = startWithChildTrees;
}

void SelectionProxyModel::setIncludeAllSelected(bool includeAllSelected)
{
  Q_D(SelectionProxyModel);

  // TODO: Fix ordering in this configuration.

  if ( includeAllSelected && d->m_omitDescendants && d->m_startWithChildTrees )
  {
    d->m_includeAllSelected = includeAllSelected;
  }
}

void SelectionProxyModel::setSourceModel( QAbstractItemModel *sourceModel )
{
  Q_D(SelectionProxyModel);

  AbstractProxyModel::setSourceModel(sourceModel);
  d->createProxyChain();
  d->selectionChanged(d->m_selectionModel->selection(), QItemSelection());

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
    if ( !d->m_startWithChildTrees )
    {
      d->m_map.insert(sourceIndex.internalPointer(), QPersistentModelIndex(sourceIndex));
      return createIndex( row, sourceIndex.column(), sourceIndex.internalPointer() );
    }
    return QModelIndex();
  } else if ( d->isInModel( sourceIndex ) )
  {
    int targetRow = sourceIndex.row();
    if ( ( d->m_rootIndexList.contains( sourceIndex.parent() ) )
        && ( d->m_startWithChildTrees ) )
    {
      // loop over m_rootIndexList before sourceIndex, counting children.
      targetRow = 0;
      foreach(const QModelIndex &idx, d->m_rootIndexList)
      {
        if (idx == sourceIndex.parent())
          break;
        targetRow += sourceModel()->rowCount(idx);
      }
      targetRow += sourceIndex.row();
    }
    d->m_map.insert(sourceIndex.internalPointer(), QPersistentModelIndex(sourceIndex));
    return createIndex( targetRow, sourceIndex.column(), sourceIndex.internalPointer() );
  }
  return QModelIndex();
}

int SelectionProxyModelPrivate::childrenCount(const QModelIndexList &list) const
{
  Q_Q(const SelectionProxyModel);
  int count = 0;

  foreach(const QModelIndex &idx, list)
  {
    count += q->sourceModel()->rowCount(idx);
  }

  return count;
}

int SelectionProxyModel::rowCount(const QModelIndex &index) const
{
  Q_D(const SelectionProxyModel);

  if (!index.isValid())
  {
    if ( !d->m_startWithChildTrees )
    {
      return d->m_rootIndexList.size();
    } else {

      QModelIndexList list = d->toNonPersistent(d->m_rootIndexList);
      return d->childrenCount(list);
    }
  }

  QModelIndex srcIndex = mapToSource(index);

  if (!d->isInModel(srcIndex))
    return 0;

  if ( d->m_omitDescendants )
  {
    if ( d->m_startWithChildTrees )
      return 0;

    if (d->m_rootIndexList.contains(srcIndex.parent()))
      return 0;
  }

  return sourceModel()->rowCount(srcIndex);
}

QModelIndex SelectionProxyModel::index(int row, int column, const QModelIndex &parent) const
{
  Q_D(const SelectionProxyModel);
  if (!hasIndex(row, column, parent))
    return QModelIndex();

  if (!parent.isValid())
  {
    if (!d->m_startWithChildTrees)
    {
      QModelIndex idx = d->m_rootIndexList.at( row );
      d->m_map.insert(idx.internalPointer(), idx);
      return createIndex(row, column, idx.internalPointer());
    }
    int _row = row;
    foreach(const QModelIndex &idx, d->m_rootIndexList)
    {
      int idxRowCount = sourceModel()->rowCount(idx);
      if ( _row < idxRowCount )
      {
        QModelIndex childIndex = sourceModel()->index(_row, column, idx);
        d->m_map.insert( childIndex.internalPointer(), QPersistentModelIndex( childIndex ) );
        return createIndex(row, childIndex.column(), childIndex.internalPointer());
      }
      _row -= idxRowCount;
    }

    return QModelIndex();
  } else
  {
    QModelIndex sourceParent = mapToSource(parent);
    QModelIndex sourceIndex = sourceModel()->index(row, column, sourceParent);
    return mapFromSource(sourceIndex);
  }
}

QModelIndex SelectionProxyModel::parent(const QModelIndex &index) const
{
  Q_D(const SelectionProxyModel);

  QModelIndex sourceIndex = mapToSource(index);
  if (d->m_rootIndexList.contains(sourceIndex.parent()) && d->m_startWithChildTrees)
  {
    return QModelIndex();
  }

  if (d->m_rootIndexList.contains(sourceIndex))
    return QModelIndex();

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
