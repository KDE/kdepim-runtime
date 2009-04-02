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

#include "abstractproxymodel.h"
#include "abstractitemmodel_p.h"

#include "entitytreemodel.h"

#include <KDebug>

class AbstractProxyModelPrivate : public AbstractItemModelPrivate
{
public:
  AbstractProxyModelPrivate(QAbstractProxyModel *proxy)
    : AbstractItemModelPrivate(proxy),
      headerDataSet(0)
  {

  }

  int headerDataSet;

};

AbstractProxyModel::AbstractProxyModel(QObject *parent)
  : QAbstractProxyModel(parent), d_ptr(new AbstractProxyModelPrivate(this))
{

}

AbstractProxyModel::~AbstractProxyModel()
{

}

void AbstractProxyModel::beginMoveRows(const QModelIndex &srcParent, int start, int end, const QModelIndex &destinationParent, int destinationRow)
{
  Q_D(AbstractProxyModel);
  return d->beginMoveRows(srcParent, start, end, destinationParent, destinationRow);
}

void AbstractProxyModel::endMoveRows()
{
  Q_D(AbstractProxyModel);
  d->endMoveRows();
}


void AbstractProxyModel::beginResetModel()
{
  Q_D(AbstractProxyModel);
  d->beginResetModel();
}

void AbstractProxyModel::endResetModel()
{
  Q_D(AbstractProxyModel);
  d->endResetModel();
}

void AbstractProxyModel::beginChangeChildOrder(const QModelIndex &index)
{
  Q_D(AbstractProxyModel);
  d->beginChangeChildOrder(index);
}

void AbstractProxyModel::endChangeChildOrder()
{
  Q_D(AbstractProxyModel);
  d->endChangeChildOrder();
}

void AbstractProxyModel::setHeaderSet(int set)
{
  Q_D(AbstractProxyModel);
  d->headerDataSet = set;
}

int AbstractProxyModel::headerSet() const
{
  Q_D(const AbstractProxyModel);
  return d->headerDataSet;
}

QVariant AbstractProxyModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  Q_D(const AbstractProxyModel);
  role += ( Akonadi::EntityTreeModel::TerminalUserRole * d->headerDataSet );
  return sourceModel()->headerData(section, orientation, role);
}

