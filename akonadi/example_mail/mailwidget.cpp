/*
    This file is part of the Akonadi Mail example.

    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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

#include "mailwidget.h"

#include <QHBoxLayout>
#include <QSplitter>
#include <QTextBrowser>

#include <akonadi/control.h>
#include "descendantentitiesproxymodel.h"
#include <akonadi/entitydisplayattribute.h>
#include "entitytreemodel.h"
#include "entityfilterproxymodel.h"
#include "entitytreeview.h"
#include "entityupdateadapter.h"
#include <akonadi/item.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>

#include "mailmodel.h"

#include "contactsmodel.h"

#include <kdebug.h>
#include "modeltest.h"
#include <QTimer>

using namespace Akonadi;

MailWidget::MailWidget( QWidget * parent, Qt::WindowFlags f )
    : QWidget( parent, f )
{

  QSplitter *splitter = new QSplitter( this );
  QHBoxLayout *layout = new QHBoxLayout( this );

  treeview = new EntityTreeView( splitter );

  if ( !Akonadi::Control::start() ) {
    kFatal() << "Unable to start Akonadi server, exit application";
    return;
  }

  // TODO: Tell akonadi to load a local mail resource if it's not already loaded.
  // This can't be done synchronously, but the monitor will notify the model that it has new collections
  // when the job is done.

  // Check  Akonadi::AgentManager::self()->instances() if it contains one, if it doesn't create one and report errors
  // if necessary.

  Collection rootCollection = Collection::root();

  ItemFetchScope scope;
  scope.fetchFullPayload( true ); // Need to have full item when adding it to the internal data structure
  scope.fetchAttribute< EntityDisplayAttribute >();

  Monitor *monitor = new Monitor( this );
  monitor->fetchCollection( true );
  monitor->setItemFetchScope( scope );
  // Only monitoring the root collection works.
  monitor->setCollectionMonitored( Collection::root() );
  monitor->setMimeTypeMonitored( "message/rfc822" );
//   monitor->setCollectionMonitored( rootCollection );
//   monitor->fetchCollectionStatistics( false );

  Session *session = new Session( QByteArray( "MailApplication-" ) + QByteArray::number( qrand() ), this );

  etm = new MailModel( session, monitor, this);

  // TODO: This stuff should probably be in the mailmodel constructor.
  etm->setItemPopulationStrategy(EntityTreeModel::LazyPopulation);

  collectionTree = new EntityFilterProxyModel(this);

  collectionTree->setSourceModel(etm);

  // Include only collections in this proxy model.
  collectionTree->addMimeTypeInclusionFilter( Collection::mimeType() );
  collectionTree->setHeaderSet(EntityTreeModel::CollectionTreeHeaders);

  treeview->setModel(collectionTree);
  treeview->setColumnHidden(2, true);
  QSplitter *hSplitter = new QSplitter(Qt::Vertical, splitter);

  itemList = new EntityFilterProxyModel(this);
  itemList->setSourceModel(etm);

  // Exclude collections from the list view.
  itemList->addMimeTypeExclusionFilter( Collection::mimeType() );
  itemList->setHeaderSet(EntityTreeModel::ItemListHeaders);

  listView = new EntityTreeView(splitter);
  listView->setModel(itemList);
  hSplitter->addWidget(listView);

  layout->addWidget( splitter );

  browser = new QTextBrowser( splitter );
  hSplitter->addWidget(browser);

  connect( treeview->selectionModel(),
      SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
      SLOT( treeSelectionChanged ( const QItemSelection &, const QItemSelection & ) ) );

  connect( listView->selectionModel(),
      SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
      SLOT( listSelectionChanged ( const QItemSelection &, const QItemSelection & ) ) );

  connect( etm, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ),
      SLOT( modelDataChanged( const QModelIndex &, const QModelIndex & ) ) );

}

MailWidget::~MailWidget()
{

}

void MailWidget::listSelectionChanged( const QItemSelection & selected, const QItemSelection &deselected )
{
//   if ( selected.indexes().size() == 1 )
//   {
    QModelIndex idx = selected.indexes().at( 0 );

    Item i = itemList->data( idx, EntityTreeModel::ItemRole ).value< Item >();
    if ( i.isValid() )
    {
      QByteArray ba = i.payloadData();
      browser->setText( ba );
    }
//   }
}

void MailWidget::treeSelectionChanged ( const QItemSelection & selected, const QItemSelection & deselected )
{
//   if ( selected.indexes().size() == 1 )
//   {
    QModelIndex idx = selected.indexes().at(0);

    QModelIndex etmIndex = collectionTree->mapToSource( idx );

    itemList->setRootIndex(etmIndex);
    QModelIndex filteredListIndex = itemList->mapFromSource(etmIndex);

    listView->setRootIndex(filteredListIndex);
//   }
}

void MailWidget::modelDataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight)
{
}

#include "mailwidget.moc"
