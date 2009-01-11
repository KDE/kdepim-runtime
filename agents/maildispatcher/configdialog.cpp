/*
    Copyright 2008 Ingo Klöcker <kloecker@kde.org>

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

#include "configdialog.h"
#include "settings.h"

#include <kconfigdialogmanager.h>

//#include <akonadi/collectionmodel.h>
//#include <akonadi/collectionview.h>
//#include <akonadi/collectionfilterproxymodel.h>
#include <akonadi/collectionrequester.h>

ConfigDialog::ConfigDialog(QWidget * parent) :
    KDialog( parent )
{
    ui.setupUi( mainWidget() );
    mManager = new KConfigDialogManager( this, Settings::self() );
    mManager->updateWidgets();
    QWidget *w = ui.collectionListContainer;
    QVBoxLayout *l = new QVBoxLayout( w );

    mOutboxRequester = new Akonadi::CollectionRequester();
    mOutboxRequester->setMimeTypeFilter( QStringList() << QLatin1String( "message/rfc822" ) );
    l->addWidget( mOutboxRequester );

//   mCollectionView = new Akonadi::CollectionView( w );
//   mCollectionView->setSelectionMode( QAbstractItemView::SingleSelection );
//   connect( mCollectionView, SIGNAL(clicked(QModelIndex)),
//            this, SLOT(collectionActivated(QModelIndex)) );
//   l->addWidget( mCollectionView );
// 
//   mCollectionModel = new Akonadi::CollectionModel( this );
//   Akonadi::CollectionFilterProxyModel *sortModel = new Akonadi::CollectionFilterProxyModel( this );
//   sortModel->setDynamicSortFilter( true );
//   sortModel->setSortCaseSensitivity( Qt::CaseInsensitive );
//   sortModel->addMimeTypeFilter( "message/rfc822" );
//   sortModel->setSourceModel( mCollectionModel );
//   mCollectionView->setModel( sortModel );

    connect( this, SIGNAL(okClicked()),
             this, SLOT(save()) );
}

void ConfigDialog::save()
{
    mManager->updateSettings();

//   QModelIndex index = mCollectionView->currentIndex();
//   if ( index.isValid() )
//   {
//     const Akonadi::Collection col = index.model()->data( index, Akonadi::CollectionModel::CollectionRole ).value<Akonadi::Collection>();
//     if ( col.isValid() )
//     {
//       kDebug() << "Collection" << col.id() << "selected";
//       Settings::self()->setOutbox( col.id() );
//     }
//   }
    const Akonadi::Collection outbox = mOutboxRequester->collection();
    if ( outbox.isValid() )
    {
        kDebug() << "Collection" << outbox.id() << "selected";
        Settings::self()->setOutbox( outbox.id() );
    }

    Settings::self()->writeConfig();
}

#include "configdialog.moc"
