/*
    This file is part of libkabc.
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "resourceakonadiconfig.h"
#include "resourceakonadi.h"

#include <libakonadi/collection.h>
#include <libakonadi/collectionfilterproxymodel.h>
#include <libakonadi/collectionmodel.h>
#include <libakonadi/collectionview.h>

#include <kdebug.h>
#include <kdialog.h>
#include <kconfig.h>

#include <QLayout>

using namespace Akonadi;
using namespace KABC;

ResourceAkonadiConfig::ResourceAkonadiConfig( QWidget *parent )
  : KRES::ConfigWidget( parent ), mView( 0 )
{
  QVBoxLayout *mainLayout = new QVBoxLayout( this );
  mainLayout->setMargin( 0 );
  mainLayout->setSpacing( KDialog::spacingHint() );

  mView = new CollectionView( 0, this );
  mView->setSelectionMode( QAbstractItemView::SingleSelection );

  mainLayout->addWidget( mView );

  connect( mView, SIGNAL( currentChanged( const Akonadi::Collection& ) ),
           this, SLOT( collectionChanged( const Akonadi::Collection& ) ) );

  CollectionModel *sourceModel = new CollectionModel( this );

  CollectionFilterProxyModel *filterModel = new CollectionFilterProxyModel( this );
  filterModel->addMimeType( "text/directory" );
  filterModel->setSourceModel( sourceModel );

  mView->setModel( filterModel );

  connect( mView->model(), SIGNAL( rowsInserted( const QModelIndex&, int, int ) ),
           this, SLOT( rowsInserted( const QModelIndex&, int, int ) ) );
}

void ResourceAkonadiConfig::loadSettings( KRES::Resource *res )
{
  ResourceAkonadi *resource = dynamic_cast<ResourceAkonadi*>( res );

  if ( !resource ) {
    kDebug(5700) << "cast failed";
    return;
  }

  Collection collection = resource->collection();

  mCollectionId = collection.id();

  QAbstractItemModel *model = mView->model();

  const int rowCount = model->rowCount();

  for ( int row = 0; row < rowCount; ++row ) {
    QModelIndex index = model->index( row, 0, mView->rootIndex() );
    if ( !index.isValid() )
      continue;

    QVariant data = model->data( index, CollectionModel::CollectionIdRole );
    if ( !data.isValid() )
      continue;

    if ( data.toInt() == mCollectionId ) {
      mView->setCurrentIndex( index );
      return;
    }
  }
}

void ResourceAkonadiConfig::saveSettings( KRES::Resource *res )
{
  ResourceAkonadi *resource = dynamic_cast<ResourceAkonadi*>( res );

  if ( !resource ) {
    kDebug(5700) << "cast failed";
    return;
  }

  resource->setCollection( Collection( mCollectionId ) );
}

void ResourceAkonadiConfig::collectionChanged( const Akonadi::Collection& collection )
{
  mCollectionId = collection.id();
}

void ResourceAkonadiConfig::rowsInserted( const QModelIndex &parent, int start, int end )
{
  QAbstractItemModel *model = mView->model();

  for ( int row = start; row <= end; ++row ) {
    QModelIndex index = model->index( row, 0, parent );
    if ( !index.isValid() )
      continue;

    QVariant data = model->data( index, CollectionModel::CollectionIdRole );
    if ( !data.isValid() )
      continue;

    if ( data.toInt() == mCollectionId ) {
      mView->setCurrentIndex( index );
      return;
    }
  }
}

#include "resourceakonadiconfig.moc"
