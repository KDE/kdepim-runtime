/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef BROWSERWIDGET_H
#define BROWSERWIDGET_H

#include <ui_browserwidget_itemview.h>
#include <ui_browserwidget_contentview.h>

#include <libakonadi/item.h>

#include <QtGui/QWidget>

class QModelIndex;

class KJob;


namespace Akonadi {
class CollectionView;
class CollectionModel;
class ItemModel;
class Job;
}

class BrowserWidget: public QWidget
{
  Q_OBJECT

  public:
    BrowserWidget( QWidget *parent = 0 );

  private slots:
    void collectionActivated( const QModelIndex &index );
    void itemActivated( const QModelIndex &index );
    void itemFetchDone( KJob *job );
    void modelChanged();
    void save();

  private:
    Akonadi::CollectionModel *mCollectionModel;
    Akonadi::CollectionView *mCollectionView;
    Akonadi::ItemModel *mItemModel;
    Ui::ItemViewWidget itemUi;
    Ui::ContentViewWidget contentUi;
    int mCurrentCollection;
    Akonadi::Item mCurrentItem;
};

#endif
