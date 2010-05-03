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

#ifndef KMODELINDEXPROXYMAPPER_H
#define KMODELINDEXPROXYMAPPER_H

#include <QObject>

class QAbstractItemModel;
class QModelIndex;
class QItemSelection;
class KModelIndexProxyMapperPrivate;

class KModelIndexProxyMapper : public QObject
{
  Q_OBJECT
public:
  KModelIndexProxyMapper(QAbstractItemModel *leftModel, QAbstractItemModel *rightModel, QObject* parent = 0);

  QModelIndex mapLeftToRight(const QModelIndex &index);
  QModelIndex mapRightToLeft(const QModelIndex &index);
  QItemSelection mapSelectionLeftToRight(const QItemSelection &selection);
  QItemSelection mapSelectionRightToLeft(const QItemSelection &selection);

private:
  Q_DECLARE_PRIVATE(KModelIndexProxyMapper)
  KModelIndexProxyMapperPrivate * const d_ptr;
};

#endif
