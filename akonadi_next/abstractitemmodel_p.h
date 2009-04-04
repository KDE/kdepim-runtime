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

#ifndef ABSTRACTITEMMODEL_P_H
#define ABSTRACTITEMMODEL_P_H

#include "abstractitemmodel.h"

#include <QStack>

struct MoveAction
{
  QModelIndex srcParent;
  QModelIndex destinationParent;
  int start;
  int end;
  int destinationStart;
};

class AbstractItemModelPrivate
{
public:
  AbstractItemModelPrivate(QAbstractItemModel *model);

  bool isDescendant(const QModelIndex &index1, const QModelIndex &index2);

  void beginMoveRows(const QModelIndex &srcParent, int start, int end, const QModelIndex &destinationParent, int destinationStart);

  void endMoveRows();

  void beginMoveColumns(const QModelIndex &srcParent, int start, int end, const QModelIndex &destinationParent, int destinationStart);

  void endMoveColumns();

  void beginMoveItems(const QModelIndex &srcParent, int start, int end, const QModelIndex &destinationParent, int destinationStart, Qt::Orientation orientation);

  void beginResetModel();
  void endResetModel();

  void beginChangeChildOrder(const QModelIndex &index);
  void endChangeChildOrder();

  void invalidate_Persistent_Indexes();

  bool allowMove(const QModelIndex &srcParent, int start, int end, const QModelIndex &destinationParent, int destinationStart);

  int getChange(bool sameParent, int start, int end, int currentPosition, int destinationStart, const QModelIndex &parent, const QModelIndex &srcParent );

  void movePersistentIndexes(const QModelIndex &srcParent, int start, int end, const QModelIndex &destinationParent, int destinationStart, Qt::Orientation orientation);

  Q_DECLARE_PUBLIC(AbstractItemModel)

  QAbstractItemModel *q_ptr;

  QStack<MoveAction> m_moveActions;
  QStack<QPersistentModelIndex> m_sortIndexes;

};

#endif
