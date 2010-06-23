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

#include "kresettingproxymodel.h"

KResettingProxyModel::KResettingProxyModel(QObject* parent)
  : QSortFilterProxyModel(parent)
{

}

void KResettingProxyModel::setSourceModel(QAbstractItemModel* sourceModel)
{
  connect(sourceModel, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)), this, SLOT(slotBeginReset()));
  connect(sourceModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(slotEndReset()));
  connect(sourceModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(slotBeginReset()));
  connect(sourceModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(slotEndReset()));

  QSortFilterProxyModel::setSourceModel(sourceModel);
  disconnect(sourceModel, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)), this, SLOT(_q_sourceRowsAboutToBeInserted(QModelIndex,int,int)));
  disconnect(sourceModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(_q_sourceRowsInserted(QModelIndex,int,int)));
  disconnect(sourceModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(_q_sourceRowsAboutToBeRemoved(QModelIndex,int,int)));
  disconnect(sourceModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(_q_sourceRowsRemoved(QModelIndex,int,int)));

}

void KResettingProxyModel::slotBeginReset()
{
  QMetaObject::invokeMethod(this, "_q_sourceAboutToBeReset", Qt::DirectConnection);
}

void KResettingProxyModel::slotEndReset()
{
  QMetaObject::invokeMethod(this, "_q_sourceReset", Qt::DirectConnection);
}

#include "kresettingproxymodel.moc"
