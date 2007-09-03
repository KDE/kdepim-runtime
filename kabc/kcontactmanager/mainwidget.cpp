/*
    This file is part of KContactManager.

    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include <QtGui/QHBoxLayout>
#include <QtGui/QListView>
#include <QtGui/QSplitter>

#include <libakonadi/collectionmodel.h>
#include <libakonadi/collectionview.h>
#include <libakonadi/itemview.h>

#include <kabc/kabcmodel.h>
#include <kabc/kabcitembrowser.h>

#include "mainwidget.h"

MainWidget::MainWidget( QWidget *parent )
  : QWidget( parent )
{
  setupGui();

  mCollectionModel = new Akonadi::CollectionModel( this );

  QStringList mimeTypes;
  mimeTypes << QLatin1String( "text/x-vcard" )
            << QLatin1String( "text/directory" )
            << QLatin1String( "text/vcard" );
  mCollectionModel->filterByMimeTypes( mimeTypes );

  mCollectionView->setModel( mCollectionModel );

  mContactModel = new KABCModel( this );
  mContactView->setModel( mContactModel );

  connect( mCollectionView, SIGNAL( currentChanged( const Akonadi::Collection& ) ),
           mContactModel, SLOT( setCollection( const Akonadi::Collection& ) ) );
  connect( mContactView, SIGNAL( currentChanged( const Akonadi::DataReference& ) ),
           mContactDetails, SLOT( setUid( const Akonadi::DataReference& ) ) );
}

MainWidget::~MainWidget()
{
}

void MainWidget::setupGui()
{
  QHBoxLayout *layout = new QHBoxLayout( this );

  QSplitter *splitter = new QSplitter;
  layout->addWidget( splitter );

  mCollectionView = new Akonadi::CollectionView();
  splitter->addWidget( mCollectionView );

  mContactView = new Akonadi::ItemView;
  splitter->addWidget( mContactView );

  mContactDetails = new KABCItemBrowser;
  splitter->addWidget( mContactDetails );
}
