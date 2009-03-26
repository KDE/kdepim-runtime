/*
    This file is part of the Akonadi Contacts example.

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

#include "contactswidget.h"

#include <QHBoxLayout>
#include <QSplitter>
#include <QTextBrowser>

#include <akonadi/control.h>
#include "descendantentitiesproxymodel.h"
#include <akonadi/entitydisplayattribute.h>
// #include "entitytreemodel.h"
#include "entityfilterproxymodel.h"
#include "entitytreeview.h"
#include "entityupdateadapter.h"
#include <akonadi/item.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>

#include <KLocale>

#include <kdebug.h>
#include "modeltest.h"

#include "contactsmodel.h"

using namespace Akonadi;

ContactsWidget::ContactsWidget( QWidget * parent, Qt::WindowFlags f )
    : QWidget( parent, f )
{

  QSplitter *splitter = new QSplitter( this );
  QHBoxLayout *layout = new QHBoxLayout( this );

  treeview = new EntityTreeView( splitter );

  if ( !Akonadi::Control::start() ) {
    kFatal() << "Unable to start Akonadi server, exit application";
    return;
  }

  Collection rootCollection = Collection::root();

  ItemFetchScope scope;
  scope.fetchFullPayload( true ); // Need to have full item when adding it to the internal data structure
//   scope.fetchAttribute< CollectionChildOrderAttribute >();
  scope.fetchAttribute< EntityDisplayAttribute >();

  Monitor *monitor = new Monitor( this );
  monitor->fetchCollection( true );
  monitor->setItemFetchScope( scope );
  monitor->setCollectionMonitored( Collection::root() );

  Session *session = new Session( QByteArray( "ContactsApplication-" ) + QByteArray::number( qrand() ), this );
//   EntityUpdateAdapter *eua = new EntityUpdateAdapter( session, this );

//   etm = new Akonadi::EntityTreeModel( eua, monitor, this);
  etm = new ContactsModel( session, monitor, this);

//   new ModelTest(etm, this);

  etm->fetchMimeTypes( QStringList() << "text/directory" );

  etm->setIncludeRootCollection(true);
  etm->setRootCollectionDisplayName(i18nc("Name of top level for all collections in the application", "[All]"));

  collectionTree = new EntityFilterProxyModel(this);
  collectionTree->setSourceModel(etm);

  // Include only collections in this proxy model.
  collectionTree->addMimeTypeInclusionFilter( Collection::mimeType() );

  collectionTree->setHeaderSet(AbstractItemModel::CollectionTreeHeaders);

// //   new ModelTest(collectionTree, this);

  treeview->setModel(collectionTree);
  treeview->setColumnHidden(1, true);
  treeview->setColumnHidden(2, true);
  treeview->setColumnHidden(3, true);

//   QSplitter *hSplitter = new QSplitter(Qt::Vertical, splitter);

  descList = new DescendantEntitiesProxyModel(this);
  descList->setSourceModel(etm);
  descList->setHeaderSet(AbstractItemModel::ItemListHeaders);

  itemList = new EntityFilterProxyModel(this);
  itemList->setSourceModel(descList);

  // Exclude collections from the list view.
  itemList->addMimeTypeExclusionFilter( Collection::mimeType() );

  listView = new EntityTreeView(splitter);
  listView->setModel(itemList);
  listView->setColumnHidden(3, true);
  splitter->addWidget(listView);

  layout->addWidget( splitter );

  browser = new QTextBrowser( splitter );
  splitter->addWidget(browser);

  connect( treeview->selectionModel(),
      SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
      SLOT( treeSelectionChanged ( const QItemSelection &, const QItemSelection & ) ) );

  connect( listView->selectionModel(),
      SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
      SLOT( listSelectionChanged ( const QItemSelection &, const QItemSelection & ) ) );

  connect( etm, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ),
      SLOT( modelDataChanged( const QModelIndex &, const QModelIndex & ) ) );
}

ContactsWidget::~ContactsWidget()
{

}

void ContactsWidget::listSelectionChanged( const QItemSelection & selected, const QItemSelection &deselected )
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

void ContactsWidget::treeSelectionChanged ( const QItemSelection & selected, const QItemSelection & deselected )
{
//   if ( selected.indexes().size() == 1 )
//   {

    QModelIndex idx = selected.indexes().at(0);

    QModelIndex etmIndex = collectionTree->mapToSource( idx );

    descList->setRootIndex(etmIndex);
    QModelIndex descendedListIndex = descList->mapFromSource(etmIndex);

    itemList->setRootIndex(descendedListIndex);
    QModelIndex filteredListIndex = itemList->mapFromSource(descendedListIndex);

    listView->setRootIndex(filteredListIndex);

//   }
}

void ContactsWidget::modelDataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight)
{
}

#include "contactswidget.moc"
