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


#ifndef RECURSIVEFILTERPROXYMODEL_H
#define RECURSIVEFILTERPROXYMODEL_H

#include <QtGui/QSortFilterProxyModel>

#include "akonadi_next_export.h"

class RecursiveFilterProxyModelPrivate;

class AKONADI_NEXT_EXPORT KRecursiveFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
public:
  KRecursiveFilterProxyModel(QObject* parent = 0);

  virtual ~KRecursiveFilterProxyModel();

  /* reimp */ void setSourceModel( QAbstractItemModel *model );

protected:
  /* reimp */ bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
  virtual bool acceptRow(int sourceRow, const QModelIndex &sourceParent) const;


protected:
  RecursiveFilterProxyModelPrivate * const d_ptr;

  Q_DECLARE_PRIVATE(RecursiveFilterProxyModel);

  Q_PRIVATE_SLOT(d_func(), void sourceDataChanged(const QModelIndex &source_top_left, const QModelIndex &source_bottom_right))
  Q_PRIVATE_SLOT(d_func(), void sourceRowsInserted(const QModelIndex &source_parent, int start, int end))
  Q_PRIVATE_SLOT(d_func(), void sourceRowsRemoved(const QModelIndex &source_parent, int start, int end))
};

#endif

