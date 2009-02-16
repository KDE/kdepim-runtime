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
#include <QListView>

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
    mCollectionList->setModel( mCollectionModel );
    //mCollectionProxyModel = new Akonadi::CollectionFilterProxyModel(  this );
    //mCollectionProxyModel->setSourceModel( mCollectionModel );
    //mCollectionProxyModel->addMimeTypeFilter( QString::fromLatin1( "message/x-microblog" ) );
    //mCollectionProxyModel->addMimeTypeFilter( QString::fromLatin1( "application/x-vnd.kde.microblog" ) );

    // display collections sorted
    //QSortFilterProxyModel *sortModel = new QSortFilterProxyModel( this );
    //sortModel->setDynamicSortFilter( true );
    //sortModel->setSortCaseSensitivity( Qt::CaseInsensitive );
    //sortModel->setSourceModel( mCollectionProxyModel );

    // Right part, blog list
    mMessageList = new QListView( this );
    mMessageList->setDragEnabled( false );
    mMessageList->setSelectionMode( QAbstractItemView::ExtendedSelection );
    MicroblogDelegate *delegate = new MicroblogDelegate( this, mMessageList );
    mMessageList->setItemDelegate( delegate );

    mMessageModel = new BlogModel( this );
    mMessageList->setModel( mMessageModel );
    splitter->addWidget( mMessageList );

    splitter->setSizes( QList<int>() << 200 << 200 );
}

void MainWidget::collectionClicked( const Akonadi::Collection & collection )
{
    mCurrentCollection = collection;
    mMessageModel->setCollection( Collection( mCurrentCollection ) );
}

