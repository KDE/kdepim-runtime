/*
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>
    Copyright (c) 2009 Omat Holding B.V. <info@omat.nl>

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
#include "blogmodel.h"
#include "microblogdelegate.h"

#include <akonadi/collection.h>
#include <akonadi/collectionview.h>
#include <akonadi/collectionfilterproxymodel.h>
#include <akonadi/collectionmodel.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/collectionmodifyjob.h>

#include <QtCore/QDebug>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTextEdit>
#include <QtGui/QSortFilterProxyModel>
#include <QTreeView>

using namespace Akonadi;

MainWidget::MainWidget( MainWindow * parent ) :
        QWidget( parent ), mMainWindow( parent )
{
    QHBoxLayout *layout = new QHBoxLayout( this );

    QSplitter *splitter = new QSplitter( Qt::Horizontal, this );
    layout->addWidget( splitter );

    // Left part, collection view
    mCollectionList = new Akonadi::CollectionView();
    connect( mCollectionList, SIGNAL( clicked( const Akonadi::Collection & ) ),
             SLOT( collectionClicked( const Akonadi::Collection & ) ) );
    splitter->addWidget( mCollectionList );

    // Filter the collection to only show the blogs
    mCollectionModel = new Akonadi::CollectionModel( this );
    /*
        Does not seem to work untill next check. Recheck with current trunk and enable if needed.

        mCollectionProxyModel = new Akonadi::CollectionFilterProxyModel( this );
        mCollectionProxyModel->setSourceModel( mCollectionModel );
        mCollectionProxyModel->addMimeTypeFilter( "application/x-vnd.kde.microblog" );
        mCollectionList->setModel( mCollectionProxyModel );
    */
    mCollectionList->setModel( mCollectionModel );


    // Right part, blog list
    mMessageList = new QTreeView( this );
    mMessageList->setRootIsDecorated( false );
    mMessageList->setDragEnabled( false );
    mMessageList->setSelectionMode( QAbstractItemView::ExtendedSelection );
    mMessageList->setSortingEnabled( true );

    MicroblogDelegate *delegate = new MicroblogDelegate( mMessageList, this );
    mMessageList->setItemDelegate( delegate );

    mMessageModel = new BlogModel( this );

    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSortRole( Qt::EditRole );
    proxyModel->setDynamicSortFilter( true );
    proxyModel->setSourceModel( mMessageModel );

    mMessageList->setModel( proxyModel );
    splitter->addWidget( mMessageList );

    splitter->setSizes( QList<int>() << 100 << 170 );
}

void MainWidget::collectionClicked( const Akonadi::Collection & collection )
{
    mCurrentCollection = collection;
    mMessageModel->setCollection( Collection( mCurrentCollection ) );
}

