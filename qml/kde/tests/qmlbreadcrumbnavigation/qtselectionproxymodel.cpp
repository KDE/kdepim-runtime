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

#include "qtselectionproxymodel.h"


QtSelectionProxyModelPrivate::QtSelectionProxyModelPrivate(QtSelectionProxyModel *model)
  : q_ptr(model),
    m_startWithChildTrees(false),
    m_omitChildren(false),
    m_omitDescendants(false),
    m_includeAllSelected(false),
    m_rowsRemoved(false),
    m_resetting(false)
{

}

QModelIndexList QtSelectionProxyModelPrivate::toNonPersistent(const QList<QPersistentModelIndex> &list) const
{
  QModelIndexList returnList;
  QList<QPersistentModelIndex>::const_iterator it;
  for (it = list.constBegin(); it != list.constEnd(); ++it)
    returnList << *it;

  return returnList;
}

void QtSelectionProxyModelPrivate::sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
  Q_Q(QtSelectionProxyModel);

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

void QtSelectionProxyModelPrivate::sourceLayoutAboutToBeChanged()
{
  Q_Q(QtSelectionProxyModel);

  emit q->layoutAboutToBeChanged();

  if (!m_selectionModel->hasSelection())
    return;

  QPersistentModelIndex srcPersistentIndex;
  foreach(const QPersistentModelIndex &proxyPersistentIndex, q->persistentIndexList())
  {
    m_proxyIndexes << proxyPersistentIndex;
    Q_ASSERT(proxyPersistentIndex.isValid());
    srcPersistentIndex = q->mapToSource(proxyPersistentIndex);
    Q_ASSERT(srcPersistentIndex.isValid());
    m_layoutChangePersistentIndexes << srcPersistentIndex;
  }
}

void QtSelectionProxyModelPrivate::sourceLayoutChanged()
{
  Q_Q(QtSelectionProxyModel);

  if (!m_selectionModel->hasSelection())
  {
    emit q->layoutChanged();
    return;
  }

  for(int i = 0; i < m_proxyIndexes.size(); ++i)
  {
    q->changePersistentIndex(m_proxyIndexes.at(i), q->mapFromSource(m_layoutChangePersistentIndexes.at(i)));
  }

  m_layoutChangePersistentIndexes.clear();
  m_proxyIndexes.clear();

  emit q->layoutChanged();
}

void QtSelectionProxyModelPrivate::resetInternalData()
{
  m_rootIndexList.clear();
  m_proxyChain.clear();
  m_layoutChangePersistentIndexes.clear();
  m_pendingMoves.clear();
}

void QtSelectionProxyModelPrivate::sourceModelDestroyed()
{
  Q_Q(QtSelectionProxyModel);
  // There is very little we can do here.
  resetInternalData();
  m_resetting = false;
  q->endResetModel();
}

void QtSelectionProxyModelPrivate::sourceModelAboutToBeReset()
{
  Q_Q(QtSelectionProxyModel);

  // Deselecting an index in the selectionModel will cause it to
  // be removed from m_rootIndexList, so we don't need to clear
  // the list here manually.
  m_selectionModel->clearSelection();

  q->beginResetModel();
  m_resetting = true;
}

void QtSelectionProxyModelPrivate::sourceModelReset()
{
  Q_Q(QtSelectionProxyModel);

  // No need to try to refill this. When the model is reset it doesn't have a meaningful selection anymore,
  // but when it gets one we'll be notified anyway.
  resetInternalData();
  m_selectionModel->clearSelection();
  m_resetting = false;
  createProxyChain();
  q->endResetModel();
}

QPair<int, int> QtSelectionProxyModelPrivate::getRootRange(const QModelIndex &sourceParent, int start, int end) const
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

int QtSelectionProxyModelPrivate::getProxyInitialRow(const QModelIndex &parent) const
{
  Q_Q(const QtSelectionProxyModel);
  int parentPosition = m_rootIndexList.indexOf(parent);

  QModelIndex parentAbove;
  while (parentPosition > 0)
  {
    parentPosition--;

    parentAbove = m_rootIndexList.at(parentPosition);
    Q_ASSERT(parentAbove.isValid());

    int rows = q->sourceModel()->rowCount(parentAbove);
    if ( rows > 0 )
    {
      QModelIndex sourceIndexAbove = q->sourceModel()->index(rows - 1, 0, parentAbove);
      Q_ASSERT(sourceIndexAbove.isValid());
      QModelIndex proxyChildAbove = q->mapFromSource(sourceIndexAbove);
      Q_ASSERT(proxyChildAbove.isValid());
      return proxyChildAbove.row() + 1;
    }
  }
  return 0;
}

void QtSelectionProxyModelPrivate::sourceRowsAboutToBeInserted(const QModelIndex &parent, int start, int end)
{
  Q_Q(QtSelectionProxyModel);

  if (!m_selectionModel->hasSelection() || (m_omitChildren && !m_startWithChildTrees))
    return;

  if (isInModel(parent) && !(m_startWithChildTrees && m_omitDescendants))
  {
    // The easy case.
    q->beginInsertRows(q->mapFromSource(parent), start, end);
    return;
  }

  if (m_startWithChildTrees && m_rootIndexList.contains(parent))
  {
    // Another fairly easy case.
    // The children of the indexes in m_rootIndexList are in the model, but their parents
    // are not, so we can't simply mapFromSource(parent) and get the row() to figure out
    // where the new rows are supposed to go.
    // Instead, we look at the 'higher' siblings of parent for their child items.
    // The lowest child item becomes the closest sibling of the new items.

    // sourceModel:
    // A
    // -> B
    // C
    // -> D
    // E
    // F

    // A, C, E and F are selected, then proxyModel is:
    //
    // B
    // D

    // If F gets a new child item, it must be inserted into the proxy model below D.
    // To find out what the proxyRow is, we find the proxyRow of D which is already in the model,
    // and +1 it.

    int proxyStartRow = getProxyInitialRow(parent);

    proxyStartRow += start;

    q->beginInsertRows(QModelIndex(), proxyStartRow, proxyStartRow + (end - start));
    return;
  }

}

void QtSelectionProxyModelPrivate::sourceRowsInserted(const QModelIndex &parent, int start, int end)
{
  Q_Q(QtSelectionProxyModel);
  Q_UNUSED(end);
  Q_UNUSED(start);

  if (!m_selectionModel->hasSelection() || (m_omitChildren && !m_startWithChildTrees))
    return;

  if (isInModel(parent) && !(m_startWithChildTrees && m_omitDescendants))
  {
    q->endInsertRows();
    return;
  }

  if (m_startWithChildTrees && m_rootIndexList.contains(parent))
  {
    q->endInsertRows();
    return;
  }
}

void QtSelectionProxyModelPrivate::sourceRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
  Q_Q(QtSelectionProxyModel);

  if (!m_selectionModel->hasSelection() || (m_omitChildren && !m_startWithChildTrees))
    return;

  QModelIndex firstAffectedIndex = q->sourceModel()->index(start, 0, parent);

  QModelIndex proxyParent = q->mapFromSource(parent);
  QModelIndex proxyFirstAffectedIndex = q->mapFromSource(firstAffectedIndex);
  if ( proxyParent.isValid() )
  {
    // Get the easy case out of the way first.
    if (proxyFirstAffectedIndex.isValid())
    {
      m_rowsRemoved = true;
      q->beginRemoveRows(proxyParent, start, end);
      return;
    }
  }

  QModelIndexList removedSourceIndexes;
  removedSourceIndexes << firstAffectedIndex;
  for (int row = start + 1; row <= end; row++)
  {
    QModelIndex sourceChildIndex = q->sourceModel()->index(row, 0, parent);
    removedSourceIndexes << sourceChildIndex;
  }

  int proxyStart = -1;
  int proxyEnd = -1;

  // If we are going to remove a root index and all of its descendants, we need to start
  // at the top-most affected one.
  for (int row = 0; row < m_rootIndexList.size(); ++row)
  {
    QModelIndex rootIndex = m_rootIndexList.at(row);
    if (isDescendantOf(removedSourceIndexes, rootIndex) || removedSourceIndexes.contains(rootIndex))
    {
      proxyStart = row;
      break;
    }
  }

  // proxyEnd points to the last affected index in m_rootIndexList affected by the removal.
  for (int row = m_rootIndexList.size() - 1; row >= 0; --row)
  {
    QModelIndex rootIndex = m_rootIndexList.at(row);

    if (isDescendantOf(removedSourceIndexes, rootIndex) || removedSourceIndexes.contains(rootIndex))
    {
      proxyEnd = row;
      break;
    }
  }

  if (proxyStart == -1)
  {
    if (!m_startWithChildTrees)
    {
      return;
    }
    // No index in m_rootIndexList was a descendant of a deleted row.
    // Probably children of an index in m_rootIndex are being removed.
    if (!m_rootIndexList.contains(parent))
    {
      return;
    }

    int parentPosition = -1;
    if (!parent.isValid())
    {
      proxyStart = start;
    } else {
      parentPosition = m_rootIndexList.indexOf(parent);
      proxyStart = getTargetRow(parentPosition) + start;
    }

    proxyEnd = proxyStart + (end - start);

    // Descendants of children that are being removed must also be removed if they are also selected.
    for (int position = parentPosition + 1; position < m_rootIndexList.size(); ++position)
    {
      QModelIndex nextParent = m_rootIndexList.at(position);
      if (isDescendantOf(parent, nextParent))
      {
        if (end > childOfParent(parent, nextParent).row())
        {
          // All descendants of rows to be removed are accounted for.
          break;
        }

        proxyEnd += q->sourceModel()->rowCount(nextParent);
      } else {
        break;
      }
    }
  } else {
    if (m_startWithChildTrees)
    {
      proxyStart = getTargetRow(proxyStart);

      QModelIndex lastAffectedSourceParent = m_rootIndexList.at(proxyEnd);
      QModelIndex lastAffectedSourceChild = q->sourceModel()->index(q->sourceModel()->rowCount(lastAffectedSourceParent) - 1, 0, lastAffectedSourceParent);

      QModelIndex lastAffectedProxyChild = q->mapFromSource(lastAffectedSourceChild);

      proxyEnd = lastAffectedProxyChild.row();
    }
  }

  if (proxyStart == -1 || proxyEnd == -1)
    return;

  m_rowsRemoved = true;
  q->beginRemoveRows(QModelIndex(), proxyStart, proxyEnd);
}

void QtSelectionProxyModelPrivate::sourceRowsRemoved(const QModelIndex &parent, int start, int end)
{
  Q_Q(QtSelectionProxyModel);
  Q_UNUSED(parent)
  Q_UNUSED(start)
  Q_UNUSED(end)

  if (!m_selectionModel->hasSelection() || (m_omitChildren && !m_startWithChildTrees))
    return;

  QMutableListIterator<QPersistentModelIndex> it(m_rootIndexList);
  while (it.hasNext())
  {
    QPersistentModelIndex idx = it.next();
    if (!idx.isValid())
    {
      emit q->rootIndexAboutToBeRemoved(idx);
      it.remove();
    }
  }

  if (m_rowsRemoved)
    q->endRemoveRows();
  m_rowsRemoved = false;

}

void QtSelectionProxyModelPrivate::sourceRowsAboutToBeMoved(const QModelIndex &srcParent, int srcStart, int srcEnd, const QModelIndex &destParent, int destRow)
{
  Q_Q(QtSelectionProxyModel);

  if (!m_selectionModel->hasSelection())
    return;

  bool srcInModel = (!m_startWithChildTrees && isInModel(srcParent)) || (m_startWithChildTrees && m_rootIndexList.contains(srcParent));
  bool destInModel = (!m_startWithChildTrees && isInModel(destParent)) || (m_startWithChildTrees && m_rootIndexList.contains(destParent));

  if (srcInModel)
  {
    if (destInModel)
    {
      // The easy case.
      bool allowMove = q->beginMoveRows(q->mapFromSource(srcParent), srcStart, srcEnd, q->mapFromSource(destParent), destRow);
      Q_UNUSED( allowMove ); // prevent warning in release builds.
      Q_ASSERT( allowMove );
    } else {
      // source is in the proxy, but dest isn't.
      // Emit a remove
      q->beginRemoveRows(srcParent, srcStart, srcEnd);
    }
  } else {
    if (destInModel)
    {
      // dest is in proxy, but source is not.
      // Emit an insert
      q->beginInsertRows(destParent, destRow, destRow + (srcEnd - srcStart));
    }
  }

  PendingMove pendingMove;
  pendingMove.srcInModel = srcInModel;
  pendingMove.destInModel = destInModel;
  m_pendingMoves.push(pendingMove);
}

void QtSelectionProxyModelPrivate::sourceRowsMoved(const QModelIndex &srcParent, int srcStart, int srcEnd, const QModelIndex &destParent, int destRow)
{
  Q_Q(QtSelectionProxyModel);
  Q_UNUSED(srcParent)
  Q_UNUSED(srcStart);
  Q_UNUSED(srcEnd);
  Q_UNUSED(destParent)
  Q_UNUSED(destRow);

  if (!m_selectionModel->hasSelection())
    return;

  PendingMove pendingMove = m_pendingMoves.pop();

  if (pendingMove.srcInModel)
  {
    if (pendingMove.destInModel)
    {
      // The easy case.
      q->endMoveRows();
      return;
    } else {
      q->endRemoveRows();
    }
  } else if (pendingMove.destInModel) {
    q->endInsertRows();
  }
}

bool QtSelectionProxyModelPrivate::isDescendantOf(QModelIndexList &list, const QModelIndex &idx) const
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

bool QtSelectionProxyModelPrivate::isDescendantOf(const QModelIndex &ancestor, const QModelIndex &descendant) const
{

  if (!descendant.isValid())
    return false;

  if (ancestor.isValid())
    return true;

  if (ancestor == descendant)
    return false;

  QModelIndex parent = descendant.parent();
  while (parent.isValid())
  {
    if (parent == ancestor)
      return true;

    parent = parent.parent();
  }
  return false;
}

QModelIndex QtSelectionProxyModelPrivate::childOfParent(const QModelIndex& ancestor, const QModelIndex& descendant) const
{
  QModelIndex child = descendant;
  while (child.isValid() && child.parent() != ancestor)
  {
    child = child.parent();
  }
  return child;
}


QModelIndexList QtSelectionProxyModelPrivate::getNewIndexes(const QItemSelection &selection) const
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

      Q_ASSERT(sourceNewIndex.isValid());

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

void QtSelectionProxyModelPrivate::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected )
{
  Q_Q(QtSelectionProxyModel);

  if ( !q->sourceModel() || (selected.isEmpty() && deselected.isEmpty()) )
    return;

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
        if (rowCount <= 0)
        {
          // It doesn't have any children in the model, but we need to remove it from storage anyway.
          emit q->rootIndexAboutToBeRemoved(m_rootIndexList.at(startRow));
          m_rootIndexList.removeAt(startRow);
          continue;
        }

        q->beginRemoveRows(QModelIndex(), _start, _start + rowCount - 1);
        emit q->rootIndexAboutToBeRemoved(m_rootIndexList.at(startRow));
        m_rootIndexList.removeAt(startRow);
        q->endRemoveRows();
      } else {
        q->beginRemoveRows(QModelIndex(), startRow, startRow);
        emit q->rootIndexAboutToBeRemoved(m_rootIndexList.at(startRow));
        m_rootIndexList.removeAt(startRow);
        q->endRemoveRows();
      }
    }
  }

  QItemSelection newRanges;

  if (!m_includeAllSelected)
  {
    newRanges = getRootRanges(selected);
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
  } else {
    newRanges = selected;
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
          emit q->rootIndexAboutToBeRemoved(m_rootIndexList.at(row));
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
    Q_ASSERT(idx.isValid());
    if (!newIndexes.contains(idx))
      newIndexes << idx;
  }
  if (newIndexes.size() > 0)
    insertionSort(newIndexes);
}

int QtSelectionProxyModelPrivate::getRootListRow(const QModelIndexList &list, const QModelIndex &index) const
{

  if (list.isEmpty())
    return 0;

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
  foreach(const QModelIndex &root, list)
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

  QModelIndex commonParent = index;
  QModelIndex youngestAncestor;

  int firstCommonParent = -1;
  int bestParentRow = -1;
  while (commonParent.isValid())
  {
    youngestAncestor = commonParent;
    commonParent = commonParent.parent();

    for (int i = 0; i < rootAncestors.size(); ++i )
    {
      QModelIndexList ancestorList = rootAncestors.at(i);

      int parentRow = ancestorList.indexOf(commonParent);

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

  // If @p list is non-empty, the invalid QModelIndex() will at least be found in ancestorList.
  Q_ASSERT(firstCommonParent >= 0);

  QModelIndexList firstAnsList = rootAncestors.at(firstCommonParent);

  QModelIndex eldestSibling = firstAnsList.value( bestParentRow + 1 );

  if (eldestSibling.isValid())
  {
    // firstCommonParent is a sibling of one of the ancestors of @p index.
    // It is the first index to share a common parent with one of the ancestors of @p index.
    if (eldestSibling.row() >= youngestAncestor.row())
      return firstCommonParent;
  }

  int siblingOffset = 1;

  // The same commonParent might be common to several root indexes.
  // If this is the last in the list, it's the only match. We instruct the model
  // to insert the new index after it ( + siblingOffset).
  if (rootAncestors.size() <= firstCommonParent + siblingOffset )
  {
    return firstCommonParent + siblingOffset;
  }

  // A
  // - B
  //   - C
  //   - D
  //   - E
  // F
  //
  // F is selected, then C then D. When inserting D into the model, the commonParent is B (the parent of C).
  // The next existing sibling of B is F (in the proxy model). bestParentRow will then refer to an index on
  // the level of a child of F (which doesn't exist - Boom!). If it doesn't exist, then we've already found
  // the place to insert D
  QModelIndexList ansList = rootAncestors.at(firstCommonParent + siblingOffset );
  if (ansList.size() <= bestParentRow)
  {
    return firstCommonParent + siblingOffset;
  }

  QModelIndex nextParent = ansList.at(bestParentRow);
  while (nextParent == commonParent)
  {
    if (ansList.size() < bestParentRow + 1)
      // If the list is longer, it means that at the end of it is a descendant of the new index.
      // We insert the ancestors items first in that case.
      break;

    QModelIndex nextSibling = ansList.value(bestParentRow + 1);

    if (!nextSibling.isValid())
    {
      continue;
    }

    if (youngestAncestor.row() <= nextSibling.row())
    {
      break;
    }

    siblingOffset++;

    if (rootAncestors.size() <= firstCommonParent + siblingOffset )
      break;

    ansList = rootAncestors.at(firstCommonParent + siblingOffset );

    // In the scenario above, E is selected after D, causing this loop to be entered,
    // and requiring a similar result if the next sibling in the proxy model does not have children.
    if (ansList.size() <= bestParentRow)
    {
      break;
    }

    nextParent = ansList.at(bestParentRow);
  }

  return firstCommonParent + siblingOffset;
}

QList<QPair<QModelIndex, QModelIndexList> > QtSelectionProxyModelPrivate::regroup(const QModelIndexList &list) const
{
  QList<QPair<QModelIndex, QModelIndexList> > groups;

  // TODO: implement me.
  Q_UNUSED(list);
//   foreach (const QModelIndex idx, list)
//   {
//
//   }

  return groups;
}

int QtSelectionProxyModelPrivate::getTargetRow(int rootListRow)
{
  Q_Q(QtSelectionProxyModel);
  if (!m_startWithChildTrees)
    return rootListRow;

  const int column = 0;

  --rootListRow;
  while (rootListRow >= 0)
  {
    QModelIndex idx = m_rootIndexList.at(rootListRow);
    Q_ASSERT(idx.isValid());
    int rowCount = q->sourceModel()->rowCount(idx);
    if (rowCount > 0)
    {

      QModelIndex proxyLastChild = q->mapFromSource(q->sourceModel()->index(rowCount - 1, column, idx));

      return proxyLastChild.row() + 1;
    }
    --rootListRow;
  }

  return 0;

}

void QtSelectionProxyModelPrivate::insertionSort(const QModelIndexList &list)
{

  Q_Q(QtSelectionProxyModel);

  // TODO: regroup indexes in list into contiguous ranges with the same parent.
//   QList<QPair<QModelIndex, QModelIndexList> > regroup(list);
  // where pair.first is a parent, and pair.second is a list of contiguous indexes of that parent.
  // That will allow emitting new rows in ranges rather than one at a time.

  foreach(const QModelIndex &newIndex, list)
  {
    if ( m_startWithChildTrees )
    {
      QModelIndexList list = toNonPersistent(m_rootIndexList);
      int rootListRow = getRootListRow(list, newIndex);
      Q_ASSERT(q->sourceModel() == newIndex.model());
      int rowCount = q->sourceModel()->rowCount(newIndex);
      int startRow = getTargetRow(rootListRow);
      if ( rowCount > 0 )
      {
        if (!m_resetting)
          q->beginInsertRows(QModelIndex(), startRow, startRow + rowCount - 1);
        Q_ASSERT(newIndex.isValid());
        m_rootIndexList.insert(rootListRow, newIndex);
        emit q->rootIndexAdded(newIndex);
        if (!m_resetting)
        {
          q->endInsertRows();
        }
      } else {
        // Even if the newindex doesn't have any children to put into the model yet,
        // We still need to make sure it's future children are inserted into the model.
        m_rootIndexList.insert(rootListRow, newIndex);
        emit q->rootIndexAdded(newIndex);
      }
    } else {
      QModelIndexList list = toNonPersistent(m_rootIndexList);
      int row = getRootListRow(list, newIndex);

      if (!m_resetting)
        q->beginInsertRows(QModelIndex(), row, row);
      Q_ASSERT(newIndex.isValid());
      m_rootIndexList.insert(row, newIndex);
      emit q->rootIndexAdded(newIndex);
      if (!m_resetting)
      {
        q->endInsertRows();
      }
    }
  }
}

void QtSelectionProxyModelPrivate::createProxyChain()
{
  Q_Q(QtSelectionProxyModel);

  const QAbstractItemModel *model = m_selectionModel->model();
  const QAbstractProxyModel *proxyModel = qobject_cast<const QAbstractProxyModel*>(model);

  const QAbstractProxyModel *nextProxyModel;

  if (!proxyModel)
  {
    Q_ASSERT(model == q->sourceModel());
    return;
  }

  while (proxyModel)
  {
    if (proxyModel == q->sourceModel())
      break;

    m_proxyChain << proxyModel;

    nextProxyModel = qobject_cast<const QAbstractProxyModel*>(proxyModel->sourceModel());

    if (!nextProxyModel)
    {
      // It's the final model in the chain, so it is necessarily the sourceModel.
      Q_ASSERT(qobject_cast<QAbstractItemModel*>(proxyModel->sourceModel()) == q->sourceModel());
      break;
    }
    proxyModel = nextProxyModel;
  }
}

QItemSelection QtSelectionProxyModelPrivate::getRootRanges(const QItemSelection &selection) const
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

QModelIndex QtSelectionProxyModelPrivate::selectionIndexToSourceIndex(const QModelIndex &index) const
{
  QModelIndex seekIndex = index;
  QListIterator<const QAbstractProxyModel*> i(m_proxyChain);

  while (i.hasNext())
  {
    const QAbstractProxyModel *proxy = i.next();
    Q_ASSERT(seekIndex.model() == proxy);
    seekIndex = proxy->mapToSource(seekIndex);
  }

  Q_ASSERT(seekIndex.model() == q_func()->sourceModel());
  return seekIndex;
}

bool QtSelectionProxyModelPrivate::isInModel(const QModelIndex &sourceIndex) const
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

QtSelectionProxyModel::QtSelectionProxyModel(QItemSelectionModel *selectionModel, QObject *parent)
  : QAbstractProxyModel(parent), d_ptr(new QtSelectionProxyModelPrivate(this))
{
  Q_D(QtSelectionProxyModel);

  d->m_selectionModel = selectionModel;
}

QtSelectionProxyModel::~QtSelectionProxyModel()
{
  delete d_ptr;
}

void QtSelectionProxyModel::setFilterBehavior(FilterBehavior behavior)
{
  Q_D(QtSelectionProxyModel);

  beginResetModel();

  d->m_filterBehavior = behavior;

  switch(behavior)
  {
  case SubTrees:
  {
    d->m_omitChildren = false;
    d->m_omitDescendants = false;
    d->m_startWithChildTrees = false;
    d->m_includeAllSelected = false;
    break;
  }
  case SubTreeRoots:
  {
    d->m_omitChildren = true;
    d->m_startWithChildTrees = false;
    d->m_includeAllSelected = false;
    break;
  }
  case SubTreesWithoutRoots:
  {
    d->m_omitChildren = false;
    d->m_omitDescendants = true;
    d->m_startWithChildTrees = true;
    d->m_includeAllSelected = false;
    break;
  }
  case ExactSelection:
  {
    d->m_omitChildren = true;
    d->m_startWithChildTrees = false;
    d->m_includeAllSelected = true;
    break;
  }
  case ChildrenOfExactSelection:
  {
    d->m_omitChildren = false;
    d->m_omitDescendants = true;
    d->m_startWithChildTrees = true;
    d->m_includeAllSelected = true;
    break;
  }
  }

  endResetModel();
}

QtSelectionProxyModel::FilterBehavior QtSelectionProxyModel::filterBehavior() const
{
  Q_D(const QtSelectionProxyModel);
  return (QtSelectionProxyModel::FilterBehavior)d->m_filterBehavior;
}

void QtSelectionProxyModel::setSourceModel( QAbstractItemModel *_sourceModel )
{
  Q_D(QtSelectionProxyModel);
  if (_sourceModel == sourceModel())
    return;

  connect( d->m_selectionModel, SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
      SLOT( selectionChanged( const QItemSelection &, const QItemSelection & ) ) );


  beginResetModel();
  d->m_resetting = true;

  if (_sourceModel)
  {
    disconnect(_sourceModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),
            this, SLOT(sourceRowsAboutToBeInserted(const QModelIndex &, int, int)));
    disconnect(_sourceModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
            this, SLOT(sourceRowsInserted(const QModelIndex &, int, int)));
    disconnect(_sourceModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
            this, SLOT(sourceRowsAboutToBeRemoved(const QModelIndex &, int, int)));
    disconnect(_sourceModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
            this, SLOT(sourceRowsRemoved(const QModelIndex &, int, int)));
    disconnect(_sourceModel, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
            this, SLOT(sourceRowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
    disconnect(_sourceModel, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
            this, SLOT(sourceRowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
    disconnect(_sourceModel, SIGNAL(modelAboutToBeReset()),
            this, SLOT(sourceModelAboutToBeReset()));
    disconnect(_sourceModel, SIGNAL(modelReset()),
            this, SLOT(sourceModelReset()));
    disconnect(_sourceModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
            this, SLOT(sourceDataChanged(const QModelIndex &, const QModelIndex & )));
    disconnect(_sourceModel, SIGNAL(layoutAboutToBeChanged()),
            this, SLOT(sourceLayoutAboutToBeChanged()));
    disconnect(_sourceModel, SIGNAL(layoutChanged()),
            this, SLOT(sourceLayoutChanged()));
    disconnect(_sourceModel, SIGNAL(destroyed()),
            this, SLOT(sourceModelDestroyed()));
  }

  // Must be called before QAbstractProxyModel::setSourceModel because it emits some signals.
  d->resetInternalData();
  QAbstractProxyModel::setSourceModel(_sourceModel);
  if (_sourceModel)
  {
    d->createProxyChain();
    if (d->m_selectionModel->hasSelection())
      d->selectionChanged(d->m_selectionModel->selection(), QItemSelection());

    connect(_sourceModel, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),
            SLOT(sourceRowsAboutToBeInserted(const QModelIndex &, int, int)));
    connect(_sourceModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
            SLOT(sourceRowsInserted(const QModelIndex &, int, int)));
    connect(_sourceModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
            SLOT(sourceRowsAboutToBeRemoved(const QModelIndex &, int, int)));
    connect(_sourceModel, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
            SLOT(sourceRowsRemoved(const QModelIndex &, int, int)));
    connect(_sourceModel, SIGNAL(rowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
            SLOT(sourceRowsAboutToBeMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
    connect(_sourceModel, SIGNAL(rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)),
            SLOT(sourceRowsMoved(const QModelIndex &, int, int, const QModelIndex &, int)));
    connect(_sourceModel, SIGNAL(modelAboutToBeReset()),
            SLOT(sourceModelAboutToBeReset()));
    connect(_sourceModel, SIGNAL(modelReset()),
            SLOT(sourceModelReset()));
    connect(_sourceModel, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
            SLOT(sourceDataChanged(const QModelIndex &, const QModelIndex & )));
    connect(_sourceModel, SIGNAL(layoutAboutToBeChanged()),
            SLOT(sourceLayoutAboutToBeChanged()));
    connect(_sourceModel, SIGNAL(layoutChanged()),
            SLOT(sourceLayoutChanged()));
    connect(_sourceModel, SIGNAL(destroyed()),
            SLOT(sourceModelDestroyed()));
  }

  d->m_resetting = false;
  endResetModel();
}

QModelIndex QtSelectionProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
  Q_D(const QtSelectionProxyModel);

  if (!proxyIndex.isValid() || !sourceModel())
    return QModelIndex();

  QModelIndex idx = d->m_map.value(proxyIndex.internalPointer());
  idx = idx.sibling(idx.row(), proxyIndex.column());
  return idx;

}

QModelIndex QtSelectionProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
  Q_D(const QtSelectionProxyModel);

  if (!sourceModel())
    return QModelIndex();

  int row = d->m_rootIndexList.indexOf( sourceIndex );
  if ( row != -1 )
  {
    if ( !d->m_startWithChildTrees )
    {
      d->m_map.insert(sourceIndex.internalPointer(), QPersistentModelIndex(sourceIndex));
      return createIndex( row, sourceIndex.column(), sourceIndex.internalPointer() );
    }
    QModelIndex sourceParent = sourceIndex.parent();
    int parentRow = d->m_rootIndexList.indexOf( sourceParent );
    if ( parentRow == -1 )
      return QModelIndex();

    int proxyRow = sourceIndex.row();
    while (parentRow > 0)
    {
      --parentRow;
      QModelIndex selectedIndexAbove = d->m_rootIndexList.at( parentRow );
      proxyRow += sourceModel()->rowCount(selectedIndexAbove);
    }

    d->m_map.insert(sourceIndex.internalPointer(), QPersistentModelIndex(sourceIndex));
    return createIndex( proxyRow, sourceIndex.column(), sourceIndex.internalPointer() );
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
    else if (d->m_startWithChildTrees)
      return QModelIndex();

    d->m_map.insert(sourceIndex.internalPointer(), QPersistentModelIndex(sourceIndex));
    return createIndex( targetRow, sourceIndex.column(), sourceIndex.internalPointer() );
  }
  return QModelIndex();
}

int QtSelectionProxyModelPrivate::childrenCount(const QModelIndexList &list) const
{
  Q_Q(const QtSelectionProxyModel);
  int count = 0;

  foreach(const QModelIndex &idx, list)
  {
    count += q->sourceModel()->rowCount(idx);
  }

  return count;
}

int QtSelectionProxyModel::rowCount(const QModelIndex &index) const
{
  Q_D(const QtSelectionProxyModel);

  if (!sourceModel())
    return 0;

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

  if ( d->m_omitChildren || (d->m_startWithChildTrees && d->m_omitDescendants) )
    return 0;

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

QModelIndex QtSelectionProxyModel::index(int row, int column, const QModelIndex &parent) const
{
  Q_D(const QtSelectionProxyModel);

  if (!hasIndex(row, column, parent) || !sourceModel())
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

QModelIndex QtSelectionProxyModel::parent(const QModelIndex &index) const
{
  Q_D(const QtSelectionProxyModel);

  if (!sourceModel() || !index.isValid())
    return QModelIndex();

  QModelIndex sourceIndex = mapToSource(index);
  if (d->m_rootIndexList.contains(sourceIndex.parent()) && ( d->m_startWithChildTrees || d->m_omitChildren ) )
  {
    return QModelIndex();
  }

  if (d->m_rootIndexList.contains(sourceIndex))
    return QModelIndex();

  QModelIndex proxyParent = mapFromSource(sourceIndex.parent());
  return proxyParent;
}

Qt::ItemFlags QtSelectionProxyModel::flags(const QModelIndex &index) const
{
  if (!index.isValid() || !sourceModel())
    return QAbstractProxyModel::flags(index);

  QModelIndex srcIndex = mapToSource(index);
  Q_ASSERT(srcIndex.isValid());
  return sourceModel()->flags(srcIndex);
}

QVariant QtSelectionProxyModel::data(const QModelIndex & index, int role) const
{
  if (!sourceModel())
    return QVariant();

  if (index.isValid())
  {
    QModelIndex idx = mapToSource(index);
    return idx.data(role);
  }
  return sourceModel()->data(index, role);
}

QVariant QtSelectionProxyModel::headerData( int section, Qt::Orientation orientation, int role  ) const
{
  if (!sourceModel())
    return QVariant();
  return sourceModel()->headerData(section, orientation, role);
}

QMimeData* QtSelectionProxyModel::mimeData( const QModelIndexList & indexes ) const
{
  if(!sourceModel())
    return QAbstractProxyModel::mimeData(indexes);
  QModelIndexList sourceIndexes;
  foreach(const QModelIndex& index, indexes)
    sourceIndexes << mapToSource(index);
  return sourceModel()->mimeData(sourceIndexes);
}

QStringList QtSelectionProxyModel::mimeTypes() const
{
  if(!sourceModel())
    return QAbstractProxyModel::mimeTypes();
  return sourceModel()->mimeTypes();
}

Qt::DropActions QtSelectionProxyModel::supportedDropActions() const
{
  if(!sourceModel())
    return QAbstractProxyModel::supportedDropActions();
  return sourceModel()->supportedDropActions();
}

bool QtSelectionProxyModel::hasChildren ( const QModelIndex & parent) const
{
  return rowCount(parent) > 0;
}

int QtSelectionProxyModel::columnCount(const QModelIndex &index) const
{
  if (!sourceModel())
    return 0;

  return sourceModel()->columnCount(mapToSource(index));
}

QItemSelectionModel *QtSelectionProxyModel::selectionModel() const
{
  Q_D(const QtSelectionProxyModel);
  return d->m_selectionModel;
}

bool QtSelectionProxyModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
  if (!sourceModel())
    return false;

  if ((row == -1) && (column == -1))
    return sourceModel()->dropMimeData(data, action, -1, -1, mapToSource(parent));

  int source_destination_row = -1;
  int source_destination_column = -1;
  QModelIndex source_parent;

  if (row == rowCount(parent)) {
    source_parent = mapToSource(parent);
    source_destination_row = sourceModel()->rowCount(source_parent);
  } else {
    QModelIndex proxy_index = index(row, column, parent);
    QModelIndex source_index = mapToSource(proxy_index);
    source_destination_row = source_index.row();
    source_destination_column = source_index.column();
    source_parent = source_index.parent();
  }
  return sourceModel()->dropMimeData(data, action, source_destination_row,
                                source_destination_column, source_parent);
}

QList<QPersistentModelIndex> QtSelectionProxyModel::sourceRootIndexes() const
{
  Q_D(const QtSelectionProxyModel);
  return d->m_rootIndexList;
}

QModelIndexList QtSelectionProxyModel::match(const QModelIndex& start, int role, const QVariant& value, int hits, Qt::MatchFlags flags) const
{
  if (role < Qt::UserRole)
    return QAbstractProxyModel::match(start, role, value, hits, flags);

  QModelIndexList list;
  QModelIndex proxyIndex;
  foreach(const QModelIndex &idx, sourceModel()->match(mapToSource(start), role, value, hits, flags))
  {
    proxyIndex = mapFromSource(idx);
    if (proxyIndex.isValid())
      list << proxyIndex;

  }
  return list;

}

#include "qtselectionproxymodel.moc"