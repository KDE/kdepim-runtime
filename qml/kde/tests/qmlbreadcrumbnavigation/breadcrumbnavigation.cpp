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

#include "breadcrumbnavigation.h"

#include <QStringList>

KBreadcrumbNavigationProxyModel::KBreadcrumbNavigationProxyModel(QItemSelectionModel* selectionModel, QObject* parent)
  : KSelectionProxyModel(selectionModel, parent)
{

}

QVariant KBreadcrumbNavigationProxyModel::data(const QModelIndex& index, int role) const
{
  if (rowCount() > 2 && index.row() == 0 && role == Qt::DisplayRole)
  {
    QModelIndex sourceIndex = mapToSource(index);
    QStringList dataList;
    while (sourceIndex.isValid())
    {
      dataList.prepend(sourceIndex.data().toString());
      sourceIndex = sourceIndex.parent();
    }
    return dataList.join(" > ");
  }
  return KSelectionProxyModel::data(index, role);
}

void KBreadcrumbNavigationProxyModel::setShowHiddenAscendantData(bool showHiddenAscendantData)
{
  m_showHiddenAscendantData = showHiddenAscendantData;
}

bool KBreadcrumbNavigationProxyModel::showHiddenAscendantData() const
{
  return m_showHiddenAscendantData;
}

KNavigatingProxyModel::KNavigatingProxyModel(KForwardingItemSelectionModel* selectionModel, QObject* parent)
  : KSelectionProxyModel(selectionModel, parent), m_selectionModel(selectionModel)
{
}

void KNavigatingProxyModel::silentSelect(const QItemSelection& selection, QItemSelectionModel::SelectionFlags command)
{
  disconnect( m_selectionModel, SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
      this, SLOT( navigationSelectionChanged( const QItemSelection &, const QItemSelection & ) ) );
  m_selectionModel->select( selection, command);
  connect( m_selectionModel, SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
      SLOT( navigationSelectionChanged( const QItemSelection &, const QItemSelection & ) ) );
}

void KNavigatingProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
  connect( m_selectionModel, SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
      SLOT( navigationSelectionChanged( const QItemSelection &, const QItemSelection & ) ) );
  connect( m_selectionModel, SIGNAL(resetNavigation()), SLOT(updateNavigation()) );

  disconnect(sourceModel, SIGNAL(modelReset()), this, SLOT(updateNavigation()));
  disconnect(sourceModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(_sourceRowsInserted(QModelIndex,int,int)));
  disconnect(sourceModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(_sourceRowsRemoved(QModelIndex,int,int)));

  QtSelectionProxyModel::setSourceModel(sourceModel);
  updateNavigation();

  connect(sourceModel, SIGNAL(modelReset()), SLOT(updateNavigation()));
  connect(sourceModel, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(_sourceRowsInserted(QModelIndex,int,int)));
  connect(sourceModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), SLOT(_sourceRowsRemoved(QModelIndex,int,int)));
}

void KNavigatingProxyModel::_sourceRowsInserted(const QModelIndex& parent, int start, int end)
{
  if (filterBehavior() != ExactSelection || parent.isValid())
    return;
  QItemSelection sel( sourceModel()->index(start, 0, parent),
                      sourceModel()->index(end, sourceModel()->columnCount(parent) - 1, parent));

  silentSelect(sel, QItemSelectionModel::Select);
}

void KNavigatingProxyModel::_sourceRowsRemoved(const QModelIndex& parent, int start, int end)
{
  if (filterBehavior() != ExactSelection || parent.isValid())
    return;

  m_selectionModel->select(QItemSelection( sourceModel()->index(start, 0, parent),
                                           sourceModel()->index(end, sourceModel()->columnCount(parent), parent)), QItemSelectionModel::Deselect);

  if ( m_selectionModel->selection().isEmpty() )
    updateNavigation();
}

void KNavigatingProxyModel::navigationSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  updateNavigation();
}

void KNavigatingProxyModel::updateNavigation()
{
  beginResetModel();
  if (!sourceModel())
  {
    setFilterBehavior(KSelectionProxyModel::ChildrenOfExactSelection);
    endResetModel();
    return;
  }

  if (m_selectionModel->selection().isEmpty())
  {
    setFilterBehavior(KSelectionProxyModel::ExactSelection);
    QModelIndex top = sourceModel()->index(0, 0);
    QModelIndex bottom = sourceModel()->index(sourceModel()->rowCount() - 1, 0);
    silentSelect(QItemSelection(top, bottom), QItemSelectionModel::Select);
  } else if (filterBehavior() != KSelectionProxyModel::ChildrenOfExactSelection) {
    setFilterBehavior(KSelectionProxyModel::ChildrenOfExactSelection);
  }
  endResetModel();
}

KForwardingItemSelectionModel::KForwardingItemSelectionModel(QAbstractItemModel* model, QItemSelectionModel* selectionModel, QObject *parent)
  : QItemSelectionModel(model, parent), m_selectionModel(selectionModel), m_direction(Forward)
{
  Q_ASSERT(model == selectionModel->model());
  connect(selectionModel, SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
          SLOT(navigationSelectionChanged(const QItemSelection&,const QItemSelection&)));
}

KForwardingItemSelectionModel::KForwardingItemSelectionModel(QAbstractItemModel* model, QItemSelectionModel* selectionModel, Direction direction, QObject *parent)
  : QItemSelectionModel(model, parent), m_selectionModel(selectionModel), m_direction(direction)
{
  Q_ASSERT(model == selectionModel->model());
  if (m_direction == Forward)
    connect(selectionModel, SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),
            SLOT(navigationSelectionChanged(const QItemSelection&,const QItemSelection&)));
}

void KForwardingItemSelectionModel::select(const QModelIndex& index, QItemSelectionModel::SelectionFlags command)
{
  if (m_direction == Reverse)
    m_selectionModel->select(index, command);
  else
    QItemSelectionModel::select(index, command);
}

void KForwardingItemSelectionModel::select(const QItemSelection& selection, QItemSelectionModel::SelectionFlags command)
{
  if (m_direction == Reverse)
    m_selectionModel->select(selection, command);
  else
    QItemSelectionModel::select(selection, command);
}

void KForwardingItemSelectionModel::navigationSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  select(selected, ClearAndSelect);
  if (selected == selection())
    resetNavigation();
}

#include "breadcrumbnavigation.moc"
