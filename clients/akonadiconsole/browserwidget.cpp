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

#include "browserwidget.h"

#include <libakonadi/job.h>
#include <libakonadi/collectionview.h>
#include <libakonadi/item.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/messagemodel.h>
#include <libakonadi/messagecollectionmodel.h>

#include <kdebug.h>

#include <QSplitter>
#include <QTextEdit>
#include <QVBoxLayout>

using namespace Akonadi;

BrowserWidget::BrowserWidget(QWidget * parent) :
    QWidget( parent )
{
  QVBoxLayout *layout = new QVBoxLayout( this );

  QSplitter *splitter = new QSplitter( Qt::Horizontal, this );
  layout->addWidget( splitter );

  mCollectionView = new Akonadi::CollectionView();
  connect( mCollectionView, SIGNAL(clicked(QModelIndex)), SLOT(collectionActivated(QModelIndex)) );
  splitter->addWidget( mCollectionView );

  mCollectionModel = new Akonadi::CollectionModel( this );
  mCollectionView->setModel( mCollectionModel );

  QSplitter *splitter2 = new QSplitter( Qt::Vertical, this );
  splitter->addWidget( splitter2 );

  mItemModel = new Akonadi::ItemModel( this );
//   mItemModel = new Akonadi::MessageModel( this );

  mItemView = new QTreeView( this );
  mItemView->setRootIsDecorated( false );
  mItemView->setModel( mItemModel );
  connect( mItemView, SIGNAL(clicked(QModelIndex)), SLOT(itemActivated(QModelIndex)) );
  splitter2->addWidget( mItemView );

  mDataView = new QTextEdit( this );
  mDataView->setReadOnly( true );
  splitter2->addWidget( mDataView );
}

void BrowserWidget::collectionActivated(const QModelIndex & index)
{
  QString path = mCollectionModel->pathForIndex( mCollectionView->sourceIndex( index ) );
  if ( path.isNull() )
    return;
  mItemModel->setPath( path );
}

void BrowserWidget::itemActivated(const QModelIndex & index)
{
  DataReference ref = mItemModel->referenceForIndex( index );
  if ( ref.isNull() )
    return;
  ItemFetchJob *job = new ItemFetchJob( ref, this );
  connect( job, SIGNAL(done(Akonadi::Job*)), SLOT(itemFetchDone(Akonadi::Job*)) );
  job->start();
}

void BrowserWidget::itemFetchDone(Akonadi::Job * job)
{
  ItemFetchJob *fetch = static_cast<ItemFetchJob*>( job );
  if ( job->error() ) {
    qWarning() << "Item fetch failed: " << job->errorText();
  } else if ( fetch->items().isEmpty() ) {
    qWarning() << "No item found!";
  } else {
    Item *item = fetch->items().first();
    mDataView->setPlainText( item->data() );
  }
  job->deleteLater();
}

#include "browserwidget.moc"
