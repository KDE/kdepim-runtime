/*
    This file is part of kdepim.
    Copyright (c) 2009 Kevin Krammer <kevin.krammer@gmx.at>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "storecollectiondialog.h"

#include "storecollectionfilterproxymodel.h"

#include <akonadi/collectionmodel.h>
#include <akonadi/collectionview.h>

#include <KLocale>

#include <QLabel>
#include <QVBoxLayout>

using namespace Akonadi;

static QModelIndex findCollection( const Collection &collection,
        const QModelIndex &parent, QAbstractItemModel *model)
{
  const int rowCount = model->rowCount( parent );

  for ( int row = 0; row < rowCount; ++row ) {
    QModelIndex index = model->index( row, 0, parent );
    if ( !index.isValid() )
      continue;

    QVariant data = model->data( index, CollectionModel::CollectionIdRole );
    if ( !data.isValid() )
      continue;

    if ( data.toInt() == collection.id() ) {
      return index;
    }

    index = findCollection( collection, index, model );
    if ( index.isValid() )
      return index;
  }

  return QModelIndex();
}

StoreCollectionDialog::StoreCollectionDialog( QWidget* parent )
  : KDialog( parent ),
    mLabel( 0 ),
    mFilterModel( 0 ),
    mView( 0 )
{
  setCaption( i18nc( "@title:window", "Target Folder Selection") );

  setButtons( KDialog::Ok | KDialog::Cancel );

  CollectionModel *model = new CollectionModel( this );

  QWidget *widget = new QWidget( this );

  QVBoxLayout *mainLayout = new QVBoxLayout( widget );
  mainLayout->setMargin( KDialog::marginHint() );
  mainLayout->setSpacing( KDialog::spacingHint() );

  // initially hide label until a text is set
  mLabel = new QLabel( widget );
  mLabel->hide();

  mainLayout->addWidget( mLabel );

  mFilterModel = new StoreCollectionFilterProxyModel( this );
  mFilterModel->setSourceModel( model );

  mView = new CollectionView( widget );
  mView->setSelectionMode( QAbstractItemView::SingleSelection );
  mView->setModel( mFilterModel );

  connect( mView, SIGNAL( currentChanged( const Akonadi::Collection& ) ),
           this, SLOT( collectionChanged( const Akonadi::Collection& ) ) );
  connect( mView->model(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ),
           this, SLOT( collectionsInserted( const QModelIndex&, int, int ) ) );

  mainLayout->addWidget( mView );

  setMainWidget( widget );
}

StoreCollectionDialog::~StoreCollectionDialog()
{
}

void StoreCollectionDialog::setLabelText( const QString &labelText )
{
  mLabel->setText( labelText );
  mLabel->show();
}

void StoreCollectionDialog::setMimeType( const QString &mimeType )
{
  mFilterModel->clearFilters();
  mFilterModel->addMimeTypeFilter( mimeType );
}

void StoreCollectionDialog::setSelectedCollection( const Collection &collection )
{
  mSelectedCollection = collection;

  QModelIndex index = findCollection( mSelectedCollection, mView->rootIndex(), mView->model() );
  if ( index.isValid() ) {
    mView->setCurrentIndex( index );
  }
}

Collection StoreCollectionDialog::selectedCollection() const
{
  return mSelectedCollection;
}

void StoreCollectionDialog::setSubResourceModel( const AbstractSubResourceModel *subResourceModel )
{
  mFilterModel->setSubResourceModel( subResourceModel );
}

void StoreCollectionDialog::collectionChanged( const Collection &collection )
{
  mSelectedCollection = collection;
}

void StoreCollectionDialog::collectionsInserted( const QModelIndex &parent, int start, int end )
{
  QAbstractItemModel *model = mView->model();

  for ( int row = start; row <= end; ++row ) {
    QModelIndex index = model->index( row, 0, parent );
    if ( !index.isValid() )
      continue;

    QVariant data = model->data( index, CollectionModel::CollectionIdRole );
    if ( !data.isValid() )
      continue;

    if ( data.toLongLong() == mSelectedCollection.id() ) {
      mView->setCurrentIndex( index );
      return;
    }

    index = findCollection( mSelectedCollection, index, model );
    if ( index.isValid() ) {
      mView->setCurrentIndex( index );
      return;
    }
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
