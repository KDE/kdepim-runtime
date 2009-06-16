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

#include "abstractitemmodel.h"
#include "abstractitemmodel_p.h"

AbstractItemModel::AbstractItemModel( QObject *parent )
  : QAbstractItemModel( parent ),
    d_ptr( new AbstractItemModelPrivate( this ) )
{
}

AbstractItemModel::~AbstractItemModel()
{
}

void AbstractItemModel::beginMoveRows( const QModelIndex &srcParent, int start, int end,
                                       const QModelIndex &destinationParent, int destinationStart )
{
  Q_D(AbstractItemModel);
  return d->beginMoveRows( srcParent, start, end, destinationParent, destinationStart );
}

void AbstractItemModel::endMoveRows()
{
  Q_D(AbstractItemModel);
  d->endMoveRows();
}

void AbstractItemModel::beginMoveColumns( const QModelIndex &srcParent, int start, int end,
                                          const QModelIndex &destinationParent, int destinationStart )
{
  Q_D(AbstractItemModel);
  return d->beginMoveColumns( srcParent, start, end, destinationParent, destinationStart );
}

void AbstractItemModel::endMoveColumns()
{
  Q_D(AbstractItemModel);
  d->endMoveColumns();
}

void AbstractItemModel::beginResetModel()
{
  Q_D(AbstractItemModel);
  d->beginResetModel();
}

void AbstractItemModel::endResetModel()
{
  Q_D(AbstractItemModel);
  d->endResetModel();
}

void AbstractItemModel::beginChangeChildOrder( const QModelIndex &index )
{
  Q_D(AbstractItemModel);
  d->beginChangeChildOrder( index );
}

void AbstractItemModel::endChangeChildOrder()
{
  Q_D(AbstractItemModel);
  d->endChangeChildOrder();
}

#include "abstractitemmodel.moc"
