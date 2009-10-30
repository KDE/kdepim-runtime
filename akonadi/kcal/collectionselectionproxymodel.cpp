/*
  This file is part of Akonadi.

  Copyright (C) 2009 KDAB Author: Sebastian Sauer <sebsauer@kdab.net>
                                  Frank Osterfeld <frank@kdab.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "collectionselectionproxymodel.h"
#include "calendarmodel.h"
#include "utils.h"

#include <QItemSelectionModel>
#include <KDebug>

using namespace Akonadi;

class CollectionSelectionProxyModel::Private
{
public:
    explicit Private() : selectionModel( 0 ) {}
    QItemSelectionModel *selectionModel;
};

CollectionSelectionProxyModel::CollectionSelectionProxyModel( QObject *parent ) : QSortFilterProxyModel( parent ), d( new Private ) {}

CollectionSelectionProxyModel::~CollectionSelectionProxyModel()
{
  kDebug() << this;
  delete d;
}

QItemSelectionModel *CollectionSelectionProxyModel::selectionModel() const
{
  return d->selectionModel;
}

void CollectionSelectionProxyModel::setSelectionModel( QItemSelectionModel *selectionModel )
{
  d->selectionModel = selectionModel;
}

Qt::ItemFlags CollectionSelectionProxyModel::flags( const QModelIndex &index ) const
{
    if ( !index.isValid() )
        return QSortFilterProxyModel::flags( index );
    const Akonadi::Collection collection = Akonadi::collectionFromIndex( index );
    if ( collection.contentMimeTypes().isEmpty() )
        return QSortFilterProxyModel::flags( index ) | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    return QSortFilterProxyModel::flags(index) | Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
}

QVariant CollectionSelectionProxyModel::data( const QModelIndex &index, int role ) const
{
  if ( !index.isValid() )
      return QVariant();
  if ( d->selectionModel ) {
    if ( role == Qt::CheckStateRole ) {
      if ( index.column() != CalendarModel::CollectionTitle )
        return QVariant();
      const Akonadi::Collection collection = Akonadi::collectionFromIndex( index );
      if ( collection.contentMimeTypes().isEmpty() )
          return QVariant();
      return d->selectionModel->isSelected( index ) ? Qt::Checked : Qt::Unchecked;
    }
  }
  return QSortFilterProxyModel::data(index, role);
}

bool CollectionSelectionProxyModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
  if ( d->selectionModel && role == Qt::CheckStateRole ) {
    if ( index.column() != CalendarModel::CollectionTitle )
        return false;
    const Akonadi::Collection collection = Akonadi::collectionFromIndex( index );
    Q_ASSERT( collection.isValid() );
    const bool checked = value.toInt() == Qt::Checked;
    d->selectionModel->select( index, checked ? QItemSelectionModel::Select : QItemSelectionModel::Deselect );
    return true;
  }
  return QSortFilterProxyModel::setData(index, value, role);
}

bool CollectionSelectionProxyModel::filterAcceptsColumn( int source_column, const QModelIndex& source_parent ) const
{
    return source_column == CalendarModel::CollectionTitle;
}
