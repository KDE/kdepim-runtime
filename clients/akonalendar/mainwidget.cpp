/*
    This file is part of Akonadi.

    Copyright (c) 2008 Bruno Virlet <bvirlet@kdemail.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "mainwidget.h"
#include "mainwindow.h"

#include <QtGui/QSortFilterProxyModel>

MainWidget::MainWidget( MainWindow* parent )
    : QWidget( parent ), mMainWindow( parent )
{
    // Layout
    QHBoxLayout *layout = new QHBoxLayout( this );
    QSplitter *topSplitter = new QSplitter( Qt::Vertical, this );
    layout->addWidget( topSplitter );
    QSplitter *splitter = new QSplitter( Qt::Horizontal,  this );
    topSplitter->addWidget( splitter );

    /*
     * Views
     */
    // Calendar listview
    mCollectionList = new Akonadi::CollectionView();
    splitter->addWidget( mCollectionList );

    // Event listview
    mIncidenceList = new QTreeView( this );
    mIncidenceList->setDragEnabled( this );
    mIncidenceList->setRootIsDecorated( false );
    splitter->addWidget( mIncidenceList );

    // Event view
    mIncidenceViewer = new KCalItemBrowser( this );
    topSplitter->addWidget( mIncidenceViewer );


    /*
     * Models
     */
    // Calendar model
    mCollectionModel = new Akonadi::CollectionModel( this );
    mCollectionProxyModel = new Akonadi::CollectionFilterProxyModel( this );
    mCollectionProxyModel->setSourceModel( mCollectionModel );
    mCollectionProxyModel->addMimeType( QString::fromLatin1( "text/calendar" ) );

    // display collections sorted
    QSortFilterProxyModel *sortModel = new QSortFilterProxyModel( this );
    sortModel->setDynamicSortFilter( true );
    sortModel->setSortCaseSensitivity( Qt::CaseInsensitive );
    sortModel->setSourceModel( mCollectionProxyModel );

    // Calendar view (list of incidences)
    mIncidenceModel = new KCalModel( this );

    /*
     * Connexion between views and models
     */
    mIncidenceList->setModel( mIncidenceModel );
    mCollectionList->setModel( sortModel );

    /*
     * React to user orders
     */
    connect( mCollectionList, SIGNAL(clicked(const Akonadi::Collection&)),
             SLOT(collectionClicked(const Akonadi::Collection&)) );
    connect( mIncidenceList, SIGNAL(clicked(QModelIndex)), SLOT(itemActivated(QModelIndex)) );
}

void MainWidget::collectionClicked( const Akonadi::Collection& collection )
{
    mIncidenceModel->setCollection( collection );
}

void MainWidget::itemActivated( const QModelIndex& index )
{
    const Akonadi::Item item = mIncidenceModel->itemForIndex( index );
    mIncidenceViewer->setItem( item );
}

