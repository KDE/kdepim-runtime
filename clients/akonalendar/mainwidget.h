/*
    This file is part of Akonadi.

    Copyright (c) 2008 Bruno Virlet <bvirlet@kdemail.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QtGui/QTreeView>
#include <QSplitter>
#include <QHBoxLayout>

#include <akonadi/collection.h>
#include <akonadi/collectionview.h>
#include <akonadi/collectionfilterproxymodel.h>
#include <akonadi/collectionmodel.h>
#include "kcal/kcalmodel.h"
#include "kcal/kcalitembrowser.h"

class MainWindow;

class MainWidget : public QWidget
{
    Q_OBJECT

    public:
      MainWidget( MainWindow* parent = 0 );

    private slots:
      void collectionClicked( const Akonadi::Collection& collection );
      void itemActivated( const QModelIndex& index );

    private:
      MainWindow *mMainWindow;

      // Views
      Akonadi::CollectionView *mCollectionList;
      QTreeView *mIncidenceList;
      Akonadi::KCalItemBrowser *mIncidenceViewer;

      // Models
      Akonadi::CollectionModel *mCollectionModel;
      Akonadi::CollectionFilterProxyModel *mCollectionProxyModel;
      Akonadi::KCalModel *mIncidenceModel;

};

#endif

