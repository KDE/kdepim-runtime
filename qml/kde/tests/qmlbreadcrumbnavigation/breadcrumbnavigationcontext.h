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

#ifndef BREADCRUMBNAVIGATIONFACTORY_H
#define BREADCRUMBNAVIGATIONFACTORY_H

#include <QObject>

class QAbstractItemModel;
class QItemSelectionModel;
class QDeclarativeContext;


class KBreadcrumbNavigationFactoryPrivate;

class KBreadcrumbNavigationFactory : public QObject
{
  Q_OBJECT
public:
  KBreadcrumbNavigationFactory(QObject* parent = 0);

  void createBreadcrumbContext(QAbstractItemModel *model, QObject* parent = 0);

  void setBreadcrumbDepth(int depth);
  int breadcrumbDepth() const;

  QItemSelectionModel *breadcrumbSelectionModel() const;
  QItemSelectionModel *selectionModel() const;
  QItemSelectionModel *childSelectionModel() const;

  QAbstractItemModel *breadcrumbItemModel() const;
  QAbstractItemModel *selectedItemModel() const;
  QAbstractItemModel *unfilteredChildItemModel() const;
  QAbstractItemModel *childItemModel() const;

public slots:
  void selectBreadcrumb( int row );
  void selectChild( int row );

protected:
  virtual QAbstractItemModel* getBreadcrumbNavigationModel(QAbstractItemModel *model);
  virtual QAbstractItemModel* getChildItemsModel(QAbstractItemModel *model);

private:
  Q_DECLARE_PRIVATE(KBreadcrumbNavigationFactory)
  KBreadcrumbNavigationFactoryPrivate * const d_ptr;

};

#endif
