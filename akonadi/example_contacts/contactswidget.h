/*
    This file is part of the Akonadi Contacts example.

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

#ifndef CONTACTSWIDGET_H
#define CONTACTSWIDGET_H

class QTextBrowser;
class QModelIndex;
class QItemSelection;

namespace Akonadi
{
class EntityTreeModel;
class EntityTreeView;
class EntityFilterProxyModel;
class DescendantEntitiesProxyModel;
}

#include <QWidget>
#include <QModelIndexList>

class ContactsWidget : public QWidget
{
  Q_OBJECT

public:
  ContactsWidget( QWidget *parent = 0, Qt::WindowFlags f = 0 );
  ~ContactsWidget();


protected slots:
  void treeSelectionChanged ( const QItemSelection & selected, const QItemSelection & deselected );
  void listSelectionChanged ( const QItemSelection & selected, const QItemSelection & deselected );
  void modelDataChanged(const QModelIndex &, const QModelIndex &);

private:
  QTextBrowser   *browser;
  Akonadi::EntityTreeModel *etm;
  Akonadi::EntityTreeView *treeview;
  Akonadi::EntityTreeView *listView;
  Akonadi::EntityFilterProxyModel *collectionTree;
  Akonadi::DescendantEntitiesProxyModel *descList;
  Akonadi::EntityFilterProxyModel *itemList;
};

#endif
