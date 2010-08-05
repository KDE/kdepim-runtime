/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Stephen Kelly <stephen@kdab.com>

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

#include "qmllistselectionmodel.h"

#include <QDebug>

QMLListSelectionModel::QMLListSelectionModel(QItemSelectionModel *selectionModel, QObject* parent)
  : QObject(parent), m_selectionModel(selectionModel)
{

}

QMLListSelectionModel::QMLListSelectionModel(QAbstractItemModel* model, QObject* parent)
  : QObject(parent), m_selectionModel(new QItemSelectionModel(model, this))
{

}

QItemSelectionModel* QMLListSelectionModel::selectionModel() const
{
  return m_selectionModel;
}

QList< int > QMLListSelectionModel::selection() const
{
  QList< int > list;
  const QModelIndexList indexes = m_selectionModel->selectedRows();
  foreach (const QModelIndex &index, indexes)
    list << index.row();
  return list;
}

void QMLListSelectionModel::select(int row, int command)
{
  qDebug() << row << command;
  Q_ASSERT(row >= 0);
  static const int column = 0;
  const QModelIndex idx = m_selectionModel->model()->index(row, column);
  Q_ASSERT(idx.isValid());
  qDebug() << idx << idx.data();
  QItemSelection sel(idx, idx);
  QItemSelectionModel::SelectionFlags flags = static_cast<QItemSelectionModel::SelectionFlags>(command);
  m_selectionModel->select(sel, flags);
  emit selectionChanged();
}

void QMLListSelectionModel::clearSelection()
{
  m_selectionModel->clearSelection();
  emit selectionChanged();
}

#include "qmllistselectionmodel.moc"
