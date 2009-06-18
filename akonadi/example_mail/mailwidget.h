/*
    This file is part of the Akonadi Mail example.

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

#ifndef MAILWIDGET_H
#define MAILWIDGET_H

class QTextBrowser;
class QModelIndex;
class QItemSelection;

namespace Akonadi
{
class EntityTreeModel;
class EntityTreeView;
class EntityFilterProxyModel;
}

#include <QWidget>
#include <QModelIndexList>

#include <akonadi/item.h>

class MailWidget : public QWidget
{
  Q_OBJECT

public:
  MailWidget( QWidget *parent = 0, Qt::WindowFlags f = 0 );
  ~MailWidget();

protected slots:
  void treeSelectionChanged ( const QItemSelection & selected, const QItemSelection & deselected );
  void listSelectionChanged ( const QItemSelection & selected, const QItemSelection & deselected );
  void modelDataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight);
  void someSlot(const Akonadi::Item &item, const QSet< QByteArray > &partIdentifiers);

  void templateChanged();

protected:
  void renderMail(const QModelIndex &idx);

private:
  QTextBrowser   *browser;
  Akonadi::EntityTreeModel *etm;
  Akonadi::EntityTreeView *treeview;
  Akonadi::EntityTreeView *listView;
  Akonadi::EntityFilterProxyModel *collectionTree;
  Akonadi::EntityFilterProxyModel *itemList;
};

#endif
