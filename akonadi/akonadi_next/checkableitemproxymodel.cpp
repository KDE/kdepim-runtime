/*
    This file is part of KDE.

    Copyright (c) 2010 Stephen Kelly <steveire@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/


#include "checkableitemproxymodel.h"

#include <QItemSelectionModel>

class CheckableItemProxyModelPrivate
{
  Q_DECLARE_PUBLIC(CheckableItemProxyModel)
  CheckableItemProxyModel *q_ptr;

  CheckableItemProxyModelPrivate(CheckableItemProxyModel *checkableModel)
    : q_ptr(checkableModel),
      m_itemSelectionModel(0)
  {

  }

  QItemSelectionModel *m_itemSelectionModel;

  void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

};

CheckableItemProxyModel::CheckableItemProxyModel(QObject* parent)
  : QSortFilterProxyModel(parent), d_ptr(new CheckableItemProxyModelPrivate(this))
{

}

void CheckableItemProxyModel::setSelectionModel(QItemSelectionModel* itemSelectionModel)
{
  Q_D(CheckableItemProxyModel);
  d->m_itemSelectionModel = itemSelectionModel;
  Q_ASSERT(sourceModel() ? d->m_itemSelectionModel->model() == sourceModel() : true);
  connect(itemSelectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), SLOT(selectionChanged(QItemSelection,QItemSelection)));
}

Qt::ItemFlags CheckableItemProxyModel::flags(const QModelIndex& index) const
{
  return QSortFilterProxyModel::flags(index) | Qt::ItemIsUserCheckable;
}

QVariant CheckableItemProxyModel::data(const QModelIndex& index, int role) const
{
  Q_D(const CheckableItemProxyModel);

  if (role == Qt::CheckStateRole)
  {
    if (!d->m_itemSelectionModel)
      return Qt::Unchecked;

    return d->m_itemSelectionModel->selection().contains(mapToSource(index)) ? Qt::Checked : Qt::Unchecked;
  }
  return QSortFilterProxyModel::data(index, role);
}

bool CheckableItemProxyModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  Q_D(CheckableItemProxyModel);
  if (role == Qt::CheckStateRole)
  {
    if (!d->m_itemSelectionModel)
      return false;

    Qt::CheckState state = static_cast<Qt::CheckState>(value.toInt());
    const QModelIndex srcIndex = mapToSource(index);
    return select(QItemSelection(srcIndex, srcIndex), state == Qt::Checked ? QItemSelectionModel::Select : QItemSelectionModel::Deselect);
    return true;
  }
  return QSortFilterProxyModel::setData(index, value, role);
}

void CheckableItemProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
  QSortFilterProxyModel::setSourceModel(sourceModel);
  Q_ASSERT(d_ptr->m_itemSelectionModel ? d_ptr->m_itemSelectionModel->model() == sourceModel : true);
}

void CheckableItemProxyModelPrivate::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
  Q_Q(CheckableItemProxyModel);
  foreach (const QItemSelectionRange &range, selected)
    q->dataChanged(range.topLeft(), range.bottomRight());
  foreach (const QItemSelectionRange &range, deselected)
    q->dataChanged(range.topLeft(), range.bottomRight());
}

bool CheckableItemProxyModel::select(const QItemSelection& selection, QItemSelectionModel::SelectionFlags command)
{
  Q_D(CheckableItemProxyModel);
  d->m_itemSelectionModel->select(selection, command);
  return true;
}


#include "checkableitemproxymodel.moc"

