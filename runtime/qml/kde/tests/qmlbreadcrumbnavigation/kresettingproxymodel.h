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

#ifndef KRESETTINGPROXYMODEL
#define KRESETTINGPROXYMODEL

#include <QSortFilterProxyModel>

/**
 * This class is a workaround for buggy handling of insert/remove signals in QML:
 * http://bugreports.qt.nokia.com/browse/QTBUG-11644
 */
class KResettingProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
public:
  KResettingProxyModel(QObject* parent = 0);

  virtual void setSourceModel(QAbstractItemModel* sourceModel);

private slots:
  void slotBeginReset();
  void slotEndReset();

};

#endif
