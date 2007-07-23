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

#include<QWidget>

class QModelIndex;
class QTreeView;
class QTextEdit;

class KJob;

namespace Akonadi {
class CollectionView;
class CollectionModel;
class CollectionFilterProxyModel;
class MessageThreaderProxyModel;
class MessageModel;
}

class MainWidget: public QWidget
{
  Q_OBJECT

  public:
    MainWidget( QWidget *parent = 0 );

  private slots:
    void collectionActivated(const QModelIndex & index);
    void itemActivated(const QModelIndex & index);
    void itemFetchDone(KJob * job);

  private:
    Akonadi::CollectionModel *mCollectionModel;
    Akonadi::CollectionFilterProxyModel *mCollectionProxyModel;
    Akonadi::CollectionView *mCollectionView;
    Akonadi::MessageModel *mMessageModel;
    Akonadi::MessageThreaderProxyModel *mMessageProxyModel;
    QTreeView *mMessageList;
    QTextEdit *mMessageView;
    int mCurrentCollectionId;
};

#endif
