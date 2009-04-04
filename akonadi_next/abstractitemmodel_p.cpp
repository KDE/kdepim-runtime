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

#include "abstractitemmodel_p.h"

#include <KDebug>

AbstractItemModelPrivate::AbstractItemModelPrivate(QAbstractItemModel *model)
  : q_ptr(model)
{

}

bool AbstractItemModelPrivate::isDescendant(const QModelIndex &index1, const QModelIndex &parent)
{
  if (!index1.isValid())
    return false;

  QModelIndex child = index1;

  forever
  {
    if (child.parent() == parent)
      return true;

    if (!child.isValid())
      return false;

    child = child.parent();

  }
  return false;
}

bool AbstractItemModelPrivate::allowMove(const QModelIndex &srcParent, int start, int end, const QModelIndex &destinationParent, int destinationStart)
{
  Q_Q(AbstractItemModel);

  // Don't move the range within itself.
  if ( ( destinationParent == srcParent )
      && ( destinationStart >= start )
      && ( destinationStart <= end ) )
    return false;

  // Can't move indexes onto their own descendants.
  for (int i = start; i < end; i++)
  {
    if (isDescendant(destinationParent, q->index(i, 0, srcParent)))
    {
      return false;
    }
  }
  return true;
}

void AbstractItemModelPrivate::beginMoveRows(const QModelIndex &srcParent, int start, int end, const QModelIndex &destinationParent, int destinationStart)
{
  beginMoveItems(srcParent, start, end, destinationParent, destinationStart, Qt::Horizontal);
}

void AbstractItemModelPrivate::beginMoveColumns(const QModelIndex &srcParent, int start, int end, const QModelIndex &destinationParent, int destinationStart)
{
  beginMoveItems(srcParent, start, end, destinationParent, destinationStart, Qt::Vertical);
}

void AbstractItemModelPrivate::beginMoveItems(const QModelIndex &srcParent, int start, int end, const QModelIndex &destinationParent, int destinationStart, Qt::Orientation orientation)
{
  Q_ASSERT(start >= 0);
  Q_ASSERT(end >= start);
  Q_ASSERT(destinationStart >= 0);

  Q_Q(AbstractItemModel);

  q->layoutAboutToBeChanged();
  if (orientation == Qt::Horizontal)
    q->rowsAboutToBeMoved(srcParent, start, end, destinationParent, destinationStart);
  else
    q->columnsAboutToBeMoved(srcParent, start, end, destinationParent, destinationStart);

  Q_ASSERT(allowMove(srcParent, start, end, destinationParent, destinationStart));

  MoveAction action;
  action.srcParent = srcParent;
  action.destinationParent = destinationParent;
  action.start = start;
  action.end = end;
  action.destinationStart = destinationStart;
  m_moveActions.push(action);

  movePersistentIndexes(srcParent, start, end, destinationParent, destinationStart, orientation);
}

int AbstractItemModelPrivate::getChange(bool sameParent, int start, int end, int currentPosition, int destinationStart, const QModelIndex &parent, const QModelIndex &srcParent )
{
  const int displacementOffset = end - start + 1;
  int change = displacementOffset;

  if (sameParent)
  {
    const bool movingUp = destinationStart < start;
    const int middlePosition = movingUp ? start : end;

    if (movingUp)
    {
      if (currentPosition >= middlePosition)
        change = destinationStart - start;
    } else if (currentPosition <= middlePosition)
    {
      change = destinationStart - end - 1;
    } else {
      change = -1 * (displacementOffset);
    }
  } else if (parent == srcParent )
  {
    if (currentPosition > end)
    {
      change = start - end - 1;
    } else {
      change = destinationStart - start;
    }
  }

  return change;
}

void AbstractItemModelPrivate::movePersistentIndexes(const QModelIndex &srcParent, int start, int end, const QModelIndex &destinationParent, int destinationStart, Qt::Orientation orientation)
{
  Q_Q(AbstractItemModel);
    QModelIndexList newList;
    QModelIndexList oldList = q->persistentIndexList();

    // I need lists of indexes and how much to move them by.
    // If src and dest are different, I need to update
    // * src children from end to rowCount (-size)
    // * dest children from destinationStart to rowcount (+size)
    // * src children between start and end (new row)
    //
    // If src and dest are the same I need to update:
    // * rows caught in the middle (+offset) offset may be negative.
    // * src children between start and end (new row) (== offset - size?)

    QModelIndexList parents;
    parents << srcParent << destinationParent;

    const bool sameParent = (srcParent == destinationParent);

    const bool movingUp = destinationStart < start;

    int firstAffectedPosition;
    if (movingUp)
      firstAffectedPosition = qMin(destinationStart, start);
    else
      firstAffectedPosition = qMin(destinationStart - 1, start);
    const int lastAffectedPosition = qMax(destinationStart - 1, end);

    QMutableListIterator<QModelIndex> i(oldList);
    while (i.hasNext())
    {
      QModelIndex idx = i.next();

      if (!parents.contains(idx.parent()))
      {
        i.remove();
        continue;
      }

      int position;
      if (orientation == Qt::Horizontal)
        position = idx.row();
      else
        position = idx.column();

      if (sameParent)
      {
        if ((position < firstAffectedPosition) || ( position > lastAffectedPosition))
        {
          i.remove();
          continue;
        }
      } else if (idx.parent() == srcParent )
      {
        if (position < start)
        {
          i.remove();
          continue;
        }
      } else if ( position < destinationStart )
      {
        i.remove();
        continue;
      }
      int change = getChange(sameParent, start, end, position, destinationStart, idx.parent(), srcParent);
      if (orientation == Qt::Horizontal)
        newList << q->createIndex(position + change, idx.column(),
                                  reinterpret_cast<void *>(idx.internalId()));
      else
        newList << q->createIndex(idx.row(), position + change,
                                  reinterpret_cast<void *>(idx.internalId()));

    }
    q->changePersistentIndexList(oldList, newList);
    return;
}

void AbstractItemModelPrivate::endMoveRows()
{
  Q_Q(AbstractItemModel);

  MoveAction a = m_moveActions.pop();

  q->rowsMoved(a.srcParent, a.start, a.end, a.destinationParent, a.destinationStart);
  q->layoutChanged();
}

void AbstractItemModelPrivate::endMoveColumns()
{
  Q_Q(AbstractItemModel);

  MoveAction a = m_moveActions.pop();

  q->columnsMoved(a.srcParent, a.start, a.end, a.destinationParent, a.destinationStart);
  q->layoutChanged();
}

void AbstractItemModelPrivate::invalidate_Persistent_Indexes()
{
  Q_Q(AbstractItemModel);
  QModelIndexList oldList = q->persistentIndexList();
  QModelIndexList newList;
  for (int i=0; i < oldList.size(); i++)
    newList << QModelIndex();
  q->changePersistentIndexList(oldList, newList);
}


void AbstractItemModelPrivate::beginResetModel()
{
  Q_Q(AbstractItemModel);
  QMetaObject::invokeMethod(q, "modelAboutToBeReset", Qt::DirectConnection);
}

void AbstractItemModelPrivate::endResetModel()
{
  Q_Q(AbstractItemModel);
  invalidate_Persistent_Indexes();
  QMetaObject::invokeMethod(q, "modelReset", Qt::DirectConnection);
}

void AbstractItemModelPrivate::beginChangeChildOrder(const QModelIndex &index)
{
  Q_Q(AbstractItemModel);
  m_sortIndexes.push(QPersistentModelIndex(index));
  q->layoutAboutToBeChanged();
  q->childOrderAboutToBeChanged(index);
}

void AbstractItemModelPrivate::endChangeChildOrder()
{
  Q_Q(AbstractItemModel);
  QModelIndex idx = m_sortIndexes.pop();
  q->childOrderChanged(idx);
  q->layoutChanged();

}
