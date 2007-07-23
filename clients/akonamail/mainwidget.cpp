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

#include "mainwidget.h"

#include <libakonadi/collection.h>
#include <libakonadi/collectionview.h>
#include <libakonadi/collectionfilterproxymodel.h>
#include <libakonadi/collectionmodel.h>
#include <libakonadi/itemfetchjob.h>
#include <kmime/messagemodel.h>
#include <kmime/messagethreaderproxymodel.h>

#include <QVBoxLayout>
#include <QSplitter>
#include <QTextEdit>

using namespace Akonadi;

MainWidget::MainWidget(QWidget * parent) :
    QWidget( parent )
{
  QHBoxLayout *layout = new QHBoxLayout( this );

  QSplitter *splitter = new QSplitter( Qt::Horizontal, this );
  layout->addWidget( splitter );

  // Left part, collection view
  mCollectionView = new Akonadi::CollectionView();
  connect( mCollectionView, SIGNAL(clicked(QModelIndex)), SLOT(collectionActivated(QModelIndex)) );
  splitter->addWidget( mCollectionView );
  // Filter the collection to only show the emails
  mCollectionModel = new Akonadi::CollectionModel( this );
  mCollectionProxyModel = new Akonadi::CollectionFilterProxyModel(  this );
  mCollectionProxyModel->setSourceModel( mCollectionModel );
  mCollectionProxyModel->addMimeType( QString::fromLatin1( "message/rfc822" ) );

  // Right part, message list + message viewer
  QSplitter *rightSplitter = new QSplitter( Qt::Vertical, this );
  splitter->addWidget( rightSplitter );
  mMessageList = new QTreeView( this );
  mMessageList->setDragEnabled( true );
  mMessageList->setSelectionMode( QAbstractItemView::ExtendedSelection );
  connect( mMessageList, SIGNAL(clicked(QModelIndex)), SLOT(itemActivated(QModelIndex)) );
  rightSplitter->addWidget( mMessageList );

  mCollectionView->setModel( mCollectionProxyModel );

  mMessageModel = new Akonadi::MessageModel( this );
  mMessageProxyModel = new Akonadi::MessageThreaderProxyModel( this );
  mMessageProxyModel->setSourceModel( mMessageModel );
  mMessageList->setModel( mMessageProxyModel );

  mMessageModel->setCollection( Collection( 1 ) ); // test

  mMessageView = new QTextEdit( this );
  rightSplitter->addWidget( mMessageView );


  splitter->setSizes( QList<int>() << 200 << 400 );
  rightSplitter->setSizes( QList<int>() << 200 << 100 );
}

void MainWidget::collectionActivated(const QModelIndex & index)
{
  mCurrentCollectionId = mCollectionView->model()->data( index, CollectionModel::CollectionIdRole ).toInt();
  if ( mCurrentCollectionId <= 0 )
    return;
  mMessageModel->setCollection( Collection( mCurrentCollectionId ) );
}

void MainWidget::itemActivated(const QModelIndex & index)
{
  DataReference ref = mMessageModel->referenceForIndex( index );

  if ( ref.isNull() )
    return;

  ItemFetchJob *job = new ItemFetchJob( ref, this );
  connect( job, SIGNAL( result(KJob*) ), SLOT( itemFetchDone(KJob*) ) );
  job->start();
}

void MainWidget::itemFetchDone(KJob * job)
{
  ItemFetchJob *fetch = static_cast<ItemFetchJob*>( job );
  if ( job->error() ) {
    qWarning() << "Mail fetch failed: " << job->errorString();
  } else if ( fetch->items().isEmpty() ) {
    qWarning() << "No mail found!";
  } else {
    const Item item = fetch->items().first();
    mMessageView->setPlainText( item.part( Item::PartBody ) );
  }
}

