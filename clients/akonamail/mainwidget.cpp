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
#include "mainwindow.h"

#include <akonadi/collection.h>
#include <akonadi/collectionview.h>
#include <akonadi/collectionfilterproxymodel.h>
#include <akonadi/collectionstatisticsmodel.h>
#include <akonadi/collectionstatisticsdelegate.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/kmime/messagemodel.h>
#include <akonadi/kmime/messagethreaderproxymodel.h>
#include <agents/mailthreader/mailthreaderagent.h>

#include <QtCore/QDebug>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTextEdit>
#include <QtGui/QSortFilterProxyModel>

using namespace Akonadi;

MainWidget::MainWidget( MainWindow * parent) :
  QWidget( parent ), mMainWindow( parent )
{
  connect( mMainWindow, SIGNAL( threadCollection() ),
           this, SLOT( threadCollection() ) );

  QHBoxLayout *layout = new QHBoxLayout( this );

  QSplitter *splitter = new QSplitter( Qt::Horizontal, this );
  layout->addWidget( splitter );

  // Left part, collection view
  mCollectionList = new Akonadi::CollectionView();
  connect( mCollectionList, SIGNAL(clicked(const Akonadi::Collection &)),
           SLOT(collectionClicked(const Akonadi::Collection &)) );
  CollectionStatisticsDelegate *collectionDelegate =
                           new CollectionStatisticsDelegate( mCollectionList );
  collectionDelegate->setUnreadCountShown( true ); //For testing, should be toggleable columns eventually
  mCollectionList->setItemDelegate( collectionDelegate );
  splitter->addWidget( mCollectionList );
  // Filter the collection to only show the emails
  mCollectionModel = new Akonadi::CollectionStatisticsModel( this );
  mCollectionProxyModel = new Akonadi::CollectionFilterProxyModel(  this );
  mCollectionProxyModel->setSourceModel( mCollectionModel );
  mCollectionProxyModel->addMimeTypeFilter( QString::fromLatin1( "message/rfc822" ) );

  // display collections sorted
  QSortFilterProxyModel *sortModel = new QSortFilterProxyModel( this );
  sortModel->setDynamicSortFilter( true );
  sortModel->setSortCaseSensitivity( Qt::CaseInsensitive );
  sortModel->setSourceModel( mCollectionProxyModel );

  // Right part, message list + message viewer
  QSplitter *rightSplitter = new QSplitter( Qt::Vertical, this );
  splitter->addWidget( rightSplitter );
  mMessageList = new QTreeView( this );
  mMessageList->setDragEnabled( true );
  mMessageList->setSelectionMode( QAbstractItemView::ExtendedSelection );
  connect( mMessageList, SIGNAL(clicked(QModelIndex)), SLOT(itemActivated(QModelIndex)) );
  rightSplitter->addWidget( mMessageList );

  mCollectionList->setModel( sortModel );
  mMessageModel = new Akonadi::MessageModel( this );
  mMessageProxyModel = new Akonadi::MessageThreaderProxyModel( this );
  mMessageProxyModel->setSourceModel( mMessageModel );
  mMessageList->setModel( mMessageProxyModel );

  mMessageView = new QTextEdit( this );
  rightSplitter->addWidget( mMessageView );


  splitter->setSizes( QList<int>() << 200 << 500 );
  rightSplitter->setSizes( QList<int>() << 300 << 200 );
}

void MainWidget::collectionClicked(const Akonadi::Collection & collection)
{
  mCurrentCollection = collection;
  mMessageModel->setCollection( Collection( mCurrentCollection ) );
}

void MainWidget::itemActivated(const QModelIndex & index)
{
  const Item item = mMessageModel->itemForIndex(  mMessageProxyModel->mapToSource( index ) );

  if ( !item.isValid() )
    return;

  ItemFetchJob *job = new ItemFetchJob( item, this );
  job->fetchScope().addFetchPart( Item::FullPayload );
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
    mMessageView->setPlainText( item.payloadData() );
  }
}

void MainWidget::threadCollection()
{
  MailThreaderAttribute *a = mCurrentCollection.attribute<MailThreaderAttribute>( Collection::AddIfMissing );
  a->deserialize( QByteArray( "sort" ) );
  CollectionModifyJob *job = new CollectionModifyJob( mCurrentCollection );
  if ( !job->exec() )
    qDebug() << "Unable to modify collection";
}


