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

#include "krecursivefilterproxymodel.h"

#include <QSignalSpy>

#include <kdebug.h>

class RecursiveFilterProxyModelPrivate
{
  Q_DECLARE_PUBLIC(KRecursiveFilterProxyModel)
  KRecursiveFilterProxyModel *q_ptr;
public:
  RecursiveFilterProxyModelPrivate(KRecursiveFilterProxyModel *model)
    : q_ptr(model),
      insertSpy(0),
      removeSpy(0),
      changeSpy(0)
  {
    qRegisterMetaType<QModelIndex>("QModelIndex");
  }

  void sourceDataChanged(const QModelIndex &source_top_left, const QModelIndex &source_bottom_right);
  void sourceRowsInserted(const QModelIndex &source_parent, int start, int end);
  void sourceRowsRemoved(const QModelIndex &source_parent, int start, int end);

  QSignalSpy *insertSpy;
  QSignalSpy *removeSpy;
  QSignalSpy *changeSpy;
};

void RecursiveFilterProxyModelPrivate::sourceDataChanged(const QModelIndex &source_top_left, const QModelIndex &source_bottom_right)
{
  Q_Q(KRecursiveFilterProxyModel);

  Q_ASSERT(changeSpy);
  changeSpy->clear();
  QMetaObject::invokeMethod(q, "_q_sourceDataChanged", Qt::DirectConnection, Q_ARG(QModelIndex, source_top_left), Q_ARG(QModelIndex, source_bottom_right));
  if (!changeSpy->isEmpty())
    return; // The changed rows were processed by QSFPM.

  // Some rows with newly updated data might match the filter.
  QModelIndex parent = source_top_left.parent();
  int start = -1;
  int end = -1;
  for (int row = source_top_left.row(); row < source_bottom_right.row(); ++row)
  {
    if (q->acceptRow(row, parent))
    {
      end = row;
      if (start == -1)
        start = row;
    } else {
      if (start != 1)
      {
        // Tell the QSFPM that the matching rows were inserted so that the filtering logic is processed.
        sourceRowsInserted(parent, start, end);
        start = -1;
        end = -1;
      }
    }
  }
  if (start == -1)
    return;

  // Tell the QSFPM that the matching rows were inserted so that the filtering logic is processed.
  sourceRowsInserted(parent, start, end);
}

void RecursiveFilterProxyModelPrivate::sourceRowsInserted(const QModelIndex &source_parent, int start, int end)
{
  Q_Q(KRecursiveFilterProxyModel);

  Q_ASSERT(insertSpy);
  insertSpy->clear();
  QMetaObject::invokeMethod(q, "_q_sourceRowsInserted", Qt::DirectConnection, Q_ARG(QModelIndex, source_parent), Q_ARG(int, start), Q_ARG(int, end));
  if (!insertSpy->isEmpty())
    return; // The proxy inserted row(s).

  bool requireRow = false;
  for (int row = start; row <= end; ++row)
  {
    if (q->filterAcceptsRow(row, source_parent))
    {
      requireRow = true;
      break;
    }
  }
  if (!requireRow)
    return;

  QModelIndex sourceAscendant = source_parent;
  int lastRow;
  while(insertSpy->isEmpty())
  {
    lastRow = source_parent.row();
    sourceAscendant = sourceAscendant.parent();
    // new rows were inserted as children of a top level index.
    QMetaObject::invokeMethod(q, "_q_sourceRowsInserted", Qt::DirectConnection,
        Q_ARG(QModelIndex, sourceAscendant),
        Q_ARG(int, lastRow),
        Q_ARG(int, lastRow));
  }

  return;
}

void RecursiveFilterProxyModelPrivate::sourceRowsRemoved(const QModelIndex &source_parent, int start, int end)
{
  Q_Q(KRecursiveFilterProxyModel);

  Q_ASSERT(removeSpy);
  removeSpy->clear();
  QMetaObject::invokeMethod(q, "_q_sourceRowsRemoved", Qt::DirectConnection, Q_ARG(QModelIndex, source_parent), Q_ARG(int, start), Q_ARG(int, end));
  if (!removeSpy->isEmpty())
    return; // Nothing to do.

  // Rows were removed. Need to check if that means we no longer care about ascendants.
  QModelIndex ascendant = source_parent.parent();
  int lastRow = source_parent.row();
  if (q->filterAcceptsRow(lastRow, ascendant))
    return;

  q->invalidate();
  return;

  // Unfortunately this does not work because though it invokes the method that a row was removed, it was not actually removed, so we get breakage.
  // We need to invalidate the filter entirely instead.
#if 0
  lastRow = ascendant.row();
  ascendant = ascendant.parent();

  while (!q->acceptRow(lastRow, ascendant))
  {
    lastRow = ascendant.row();
    ascendant = ascendant.parent();
    if (!ascendant.isValid())
      break;
  }
  QMetaObject::invokeMethod(q, "_q_sourceRowsRemoved", Qt::DirectConnection, Q_ARG(QModelIndex, ascendant), Q_ARG(int, lastRow), Q_ARG(int, lastRow));
#endif
}

KRecursiveFilterProxyModel::KRecursiveFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent), d_ptr(new RecursiveFilterProxyModelPrivate(this))
{

}

KRecursiveFilterProxyModel::~KRecursiveFilterProxyModel()
{

}

bool KRecursiveFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
  QVariant da = sourceModel()->index(sourceRow, 0, sourceParent).data();

  if (acceptRow(sourceRow, sourceParent))
    return true;

  QModelIndex source_index = sourceModel()->index(sourceRow, 0, sourceParent);
  bool accepted = false;
  for (int row = 0 ; row < sourceModel()->rowCount(source_index); ++row)
    if (filterAcceptsRow(row, source_index))
      accepted = true; // Need to do this in a loop so that all siblings in a parent get processed, not just the first.

  return accepted;
}

bool KRecursiveFilterProxyModel::acceptRow(int sourceRow, const QModelIndex& sourceParent) const
{
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

void KRecursiveFilterProxyModel::setSourceModel(QAbstractItemModel* model)
{
  Q_D(RecursiveFilterProxyModel);

  // Disconnect in the QSortFilterProxyModel. These methods will be invoked manually
  disconnect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
      this, SLOT(_q_sourceDataChanged(QModelIndex,QModelIndex)));

  disconnect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
      this, SLOT(_q_sourceRowsInserted(QModelIndex,int,int)));

  disconnect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
      this, SLOT(_q_sourceRowsRemoved(QModelIndex,int,int)));

  // Standard disconnect.
  disconnect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
      this, SLOT(sourceDataChanged(QModelIndex,QModelIndex)));

  disconnect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
      this, SLOT(sourceRowsInserted(QModelIndex,int,int)));

  disconnect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
      this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));

  QSortFilterProxyModel::setSourceModel(model);

  // Slots for manual invoking of QSortFilterProxyModel methods.
  connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
      this, SLOT(sourceDataChanged(QModelIndex,QModelIndex)));

  connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)),
      this, SLOT(sourceRowsInserted(QModelIndex,int,int)));

  connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)),
      this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));

  d->insertSpy = new QSignalSpy(this, SIGNAL(rowsInserted(QModelIndex,int,int)));
  d->removeSpy = new QSignalSpy(this, SIGNAL(rowsRemoved(QModelIndex,int,int)));
  d->changeSpy = new QSignalSpy(this, SIGNAL(dataChanged(QModelIndex,QModelIndex)));
}

#include "krecursivefilterproxymodel.moc"
