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

#ifndef KJOTSWIDGET_H
#define KJOTSWIDGET_H

#include <QWidget>
#include <QModelIndexList>
#include "kjots_common_export.h"
#include <akonadi/item.h>

#include <grantlee/templateloader.h>

class KTextEdit;
class QTextBrowser;
class QTextCursor;
class QStackedWidget;
class QModelIndex;
class QItemSelection;
class QColumnView;
class QAbstractItemView;
class KSelectionProxyModel;

namespace Akonadi
{
class EntityTreeModel;
class EntityTreeView;
}

class KJotsPage;

using namespace Akonadi;

class KJOTS_COMMON_EXPORT KJotsWidget : public QWidget
{
  Q_OBJECT

public:
  KJotsWidget( QWidget *parent = 0, Qt::WindowFlags f = 0 );
  ~KJotsWidget();

protected:
  QString renderSelectionToHtml();
  QString getThemeFromUser();

protected slots:
  void renderSelection();
  void changeTheme();
  void exportSelection();
  void savePage(const QModelIndex &, int, int);
  void itemActivated(const QModelIndex &);
  void partFetched(const QModelIndex &index, Item item, const QByteArray &partName);
  void invalidated();

//   void dataChanged(const QModelIndex &, const QModelIndex &);

private:
  KTextEdit      *editor;
  QTextBrowser   *browser;
  QStackedWidget *stackedWidget;
  Akonadi::EntityTreeModel *etm;
  KSelectionProxyModel *selProxy;
  Grantlee::FileSystemTemplateLoader::Ptr m_loader;
//   Akonadi::EntityTreeView *treeview;
//   QColumnView *treeview;
  QAbstractItemView *treeview;
};

#endif
