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

#ifndef BREADCRUMBNAVIGATION_H
#define BREADCRUMBNAVIGATION_H

#include <QItemSelectionModel>

#include "kselectionproxymodel.h"

// Copied from kdeui/tests/proxymodeltestapp/breadcrumbnavigationwidget
// A version of these might be somewhere stable in the future.

class KBreadcrumbNavigationProxyModel : public KSelectionProxyModel
{
  Q_OBJECT
public:
  KBreadcrumbNavigationProxyModel(QItemSelectionModel* selectionModel, QObject* parent = 0);

  void setShowHiddenAscendantData(bool showHiddenAscendantData);
  bool showHiddenAscendantData() const;

  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

private:
  bool m_showHiddenAscendantData;

};

class KForwardingItemSelectionModel;

class KNavigatingProxyModel : public KSelectionProxyModel
{
  Q_OBJECT
public:
  KNavigatingProxyModel(KForwardingItemSelectionModel* selectionModel, QObject* parent = 0);

  virtual void setSourceModel(QAbstractItemModel* sourceModel);

private slots:
  void updateNavigation();
  void navigationSelectionChanged( const QItemSelection &, const QItemSelection & );
  void _sourceRowsInserted( const QModelIndex &parent, int start, int end );
  void _sourceRowsRemoved( const QModelIndex &parent, int start, int end );

private:
  void silentSelect(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command);

private:
  using KSelectionProxyModel::setFilterBehavior;

  KForwardingItemSelectionModel *m_selectionModel;

};

class KForwardingItemSelectionModel : public QItemSelectionModel
{
  Q_OBJECT
public:
  enum Direction
  {
    Forward,
    Reverse
  };
  KForwardingItemSelectionModel(QAbstractItemModel* model, QItemSelectionModel *selectionModel, QObject *parent = 0);
  KForwardingItemSelectionModel(QAbstractItemModel* model, QItemSelectionModel *selectionModel, Direction direction, QObject *parent = 0);

  virtual void select(const QModelIndex& index, SelectionFlags command);
  virtual void select(const QItemSelection& selection, SelectionFlags command);

signals:
  void resetNavigation();

private slots:
  void navigationSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private:
  QItemSelectionModel *m_selectionModel;
  Direction m_direction;
};

#endif
