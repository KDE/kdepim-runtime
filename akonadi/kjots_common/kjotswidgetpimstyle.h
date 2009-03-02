/*
    This file is part of KJots.

    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

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

#ifndef KJOTSWIDGETPIMSTYLE_H
#define KJOTSWIDGETPIMSTYLE_H

class KTextEdit;
class QTextBrowser;
class QTextCursor;
class QStackedWidget;
// class KJotsBookshelf;
class QModelIndex;
class QItemSelection;

namespace Akonadi
{
class EntityTreeModel;
class EntityTreeView;
class EntityFilterProxyModel;
class DescendantEntitiesProxyModel;
}

class KJotsPage;

#include <QWidget>
#include <QModelIndexList>

class KJotsWidgetPimStyle : public QWidget
{
  Q_OBJECT

public:
  KJotsWidgetPimStyle( QWidget *parent = 0, Qt::WindowFlags f = 0 );
  ~KJotsWidgetPimStyle();


protected slots:
//   void setSelection( const QModelIndex &index );
  void treeSelectionChanged ( const QItemSelection & selected, const QItemSelection & deselected );
  void listSelectionChanged ( const QItemSelection & selected, const QItemSelection & deselected );
  void modelDataChanged(const QModelIndex &, const QModelIndex &);
  void editPage(const QModelIndex &idx);
  void savePage( const QModelIndex & idx );

protected:
  void renderToBrowser( const QItemSelection & selected );
  void addBook(QTextCursor *cursor, QModelIndex idx);
  void addPage(QTextCursor *cursor, KJotsPage);
  void renderSelected();
  QModelIndexList squash(QModelIndexList list);

private:
  KTextEdit      *editor;
  QTextBrowser   *browser;
  QStackedWidget *stackedWidget;
  Akonadi::EntityTreeModel *etm;
  Akonadi::EntityTreeView *treeview;
  Akonadi::EntityTreeView *itemlist;
  Akonadi::EntityFilterProxyModel *treeproxy;
  Akonadi::EntityFilterProxyModel *listproxy;
  Akonadi::DescendantEntitiesProxyModel *descList;
  //   KJotsBookshelf *bookshelf;
};

#endif
