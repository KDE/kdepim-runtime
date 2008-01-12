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
#include "collectionattributespage.h"
#include "collectioninternalspage.h"

#include <libakonadi/job.h>
#include <libakonadi/collectionview.h>
#include <libakonadi/item.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/messagecollectionmodel.h>
#include <libakonadi/collectionfilterproxymodel.h>
#include <libakonadi/collectionpropertiesdialog.h>

#include <kabc/kabcmodel.h>
#include <kabc/kabcitembrowser.h>
#include <kmime/messagemodel.h>

#include <libkdepim/addresseeview.h>

#include <kdebug.h>
#include <kconfig.h>

#include <QSplitter>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QStackedWidget>

using namespace Akonadi;

BrowserWidget::BrowserWidget(QWidget * parent) :
    QWidget( parent ),
    mItemModel( 0 ),
    mCurrentCollection( 0 )
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

  QWidget *itemViewParent = new QWidget( this );
  itemUi.setupUi( itemViewParent );

  itemUi.modelBox->addItem( "Generic" );
  itemUi.modelBox->addItem( "Mail" );
  itemUi.modelBox->addItem( "Contacts" );
  connect( itemUi.modelBox, SIGNAL(activated(int)), SLOT(modelChanged()) );
  modelChanged();

  itemUi.itemView->setModel( mItemModel );
  itemUi.itemView->setSelectionMode( QAbstractItemView::ExtendedSelection );
  connect( itemUi.itemView, SIGNAL(clicked(QModelIndex)), SLOT(itemActivated(QModelIndex)) );
  splitter2->addWidget( itemViewParent );
  itemViewParent->layout()->setMargin( 0 );

  QWidget *contentViewParent = new QWidget( this );
  contentUi.setupUi( contentViewParent );
  connect( contentUi.saveButton, SIGNAL(clicked()), SLOT(save()) );
  splitter2->addWidget( contentViewParent );

  CollectionPropertiesDialog::registerPage<CollectionAttributePage>();
  CollectionPropertiesDialog::registerPage<CollectionInternalsPage>();
}

void BrowserWidget::collectionActivated(const QModelIndex & index)
{
  mCurrentCollection = mCollectionView->model()->data( index, CollectionModel::CollectionIdRole ).toInt();
  if ( mCurrentCollection <= 0 )
    return;
  mItemModel->setCollection( Collection( mCurrentCollection ) );
}

void BrowserWidget::itemActivated(const QModelIndex & index)
{
  DataReference ref = mItemModel->referenceForIndex( index );
  if ( ref.isNull() )
    return;
  ItemFetchJob *job = new ItemFetchJob( ref, this );
  job->addFetchPart( Item::PartBody );
  connect( job, SIGNAL(result(KJob*)), SLOT(itemFetchDone(KJob*)) );
  job->start();
}

void BrowserWidget::itemFetchDone(KJob * job)
{
  ItemFetchJob *fetch = static_cast<ItemFetchJob*>( job );
  if ( job->error() ) {
    kWarning( 5265 ) << "Item fetch failed: " << job->errorString();
  } else if ( fetch->items().isEmpty() ) {
    kWarning( 5265 ) << "No item found!";
  } else {
    const Item item = fetch->items().first();
    mCurrentItem = item;
    if ( item.hasPayload<KABC::Addressee>() ) {
      const KABC::Addressee addr = item.payload<KABC::Addressee>();

      contentUi.addresseeView->setAddressee( addr );
      contentUi.stack->setCurrentWidget( contentUi.addresseeViewPage );
    } else {
      QByteArray data = item.part( Item::PartBody );
      contentUi.dataView->setPlainText( data );
      contentUi.stack->setCurrentWidget( contentUi.dataViewPage );
    }
  }
}

void BrowserWidget::modelChanged()
{
  delete mItemModel;
  switch ( itemUi.modelBox->currentIndex() ) {
    case 1:
      mItemModel = new MessageModel( this );
      break;
    case 2:
      mItemModel = new KABCModel( this );
      break;
    default:
      mItemModel = new ItemModel( this );
  }
  itemUi.itemView->setModel( mItemModel );
  if ( mCurrentCollection > 0 )
    mItemModel->setCollection( Collection( mCurrentCollection ) );
}

void BrowserWidget::save()
{
  const QByteArray data = contentUi.dataView->toPlainText().toUtf8();
  Item item = mCurrentItem;
  item.addPart( Item::PartBody, data );
  ItemStoreJob *store = new ItemStoreJob( item, this );
  store->storePayload();
}

#include "browserwidget.moc"
