/*
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>

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

#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include "blogmodel.h"
#include <QWidget>

class QTreeView;
class QListView;
class KTabBar;

namespace Akonadi
{
class CollectionFilterProxyModel;
}

class MainWindow;

class MainWidget: public QWidget
{
    Q_OBJECT

public:
    MainWidget( MainWindow *parent = 0 );

private slots:
    void collectionChange();

private:
    Akonadi::CollectionFilterProxyModel *mCollectionProxyModel;
    BlogModel *mMessageModel;
    QTreeView *mMessageList;
    QListView *m_resourcesView;
    KTabBar *m_tabBar;

    MainWindow *mMainWindow;
};

#endif
