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

#include <KConfigDialogManager>

//#include <akonadi/collectionmodel.h>
//#include <akonadi/collectionview.h>
//#include <akonadi/collectionfilterproxymodel.h>
#include <Akonadi/Collection>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionRequester>

using namespace Akonadi;

ConfigDialog::ConfigDialog(QWidget * parent) :
    KDialog( parent )
{
    ui.setupUi( mainWidget() );
    mManager = new KConfigDialogManager( this, Settings::self() );
    mManager->updateWidgets();

    ui.outboxSelector->setMimeTypeFilter( QStringList() << QLatin1String( "message/rfc822" ) );
    // collection has no name if I skip the fetch job and just do Collection c(id)
    CollectionFetchJob *job = new CollectionFetchJob( Collection( Settings::self()->outbox() ), CollectionFetchJob::Base );
    if( job->exec() )
    {
        Collection::List cs = job->collections();
        if( !cs.empty() )
            ui.outboxSelector->setCollection( cs.first() );
    }

    ui.sentMailSelector->setMimeTypeFilter( QStringList() << QLatin1String( "message/rfc822" ) );
    job = new CollectionFetchJob( Collection( Settings::self()->sentMail() ), CollectionFetchJob::Base );
    if( job->exec() )
    {
        Collection::List cs = job->collections();
        if( !cs.empty() )
            ui.sentMailSelector->setCollection( cs.first() );
    }

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
    const Collection outbox = ui.outboxSelector->collection();
    if ( outbox.isValid() )
    {
        kDebug() << "Collection" << outbox.id() << "selected for outbox.";
        Settings::self()->setOutbox( outbox.id() );
    }

    const Collection sentMail = ui.sentMailSelector->collection();
    if ( sentMail.isValid() )
    {
        kDebug() << "Collection" << sentMail.id() << "selected for sentMail.";
        Settings::self()->setSentMail( sentMail.id() );
    }

    Settings::self()->writeConfig();
}

#include "configdialog.moc"
