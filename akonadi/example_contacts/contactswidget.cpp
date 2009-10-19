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

#include <akonadi/changerecorder.h>
#include <akonadi/control.h>
#include <kdescendantsproxymodel.h>
#include <akonadi/entitydisplayattribute.h>
// #include "entitytreemodel.h"
#include <akonadi/entitymimetypefiltermodel.h>
#include <akonadi/entitytreeview.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>
#include <kselectionproxymodel.h>

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

  ChangeRecorder *monitor = new ChangeRecorder( this );
  monitor->fetchCollection( true );
  monitor->setItemFetchScope( scope );
  monitor->setCollectionMonitored( Collection::root() );
  monitor->setMimeTypeMonitored( QLatin1String( "text/directory" ) );

  Session *session = new Session( QByteArray( "ContactsApplication-" ) + QByteArray::number( qrand() ), this );

  etm = new ContactsModel( session, monitor, this);

//   new ModelTest(etm, this);

//   etm->setItemPopulationStrategy(EntityTreeModel::LazyPopulation);

  etm->setIncludeRootCollection(true);
  etm->setRootCollectionDisplayName(i18nc("Name of top level for all collections in the application", "[All]"));

  collectionTree = new EntityMimeTypeFilterModel(this);
  collectionTree->setSourceModel(etm);

  // Include only collections in this proxy model.
  collectionTree->addMimeTypeInclusionFilter( Collection::mimeType() );

  collectionTree->setHeaderGroup(EntityTreeModel::CollectionTreeHeaders);

  treeview->setModel(collectionTree);
  treeview->setColumnHidden(1, true);
  treeview->setColumnHidden(2, true);
  treeview->setColumnHidden(3, true);

  treeview->setSelectionMode(QAbstractItemView::ExtendedSelection);

  KSelectionProxyModel *selProxy = new KSelectionProxyModel(treeview->selectionModel(), this);
  selProxy->setSourceModel(etm);
//   new ModelTest(selProxy, this);

  descList = new KDescendantsProxyModel(this);
  descList->setSourceModel(selProxy);
//  descList->setHeaderSet(EntityTreeModel::ItemListHeaders);
//   new ModelTest(descList, this);

  itemList = new EntityMimeTypeFilterModel(this);
  itemList->setSourceModel(descList);

  // Exclude collections from the list view.
  itemList->addMimeTypeExclusionFilter( Collection::mimeType() );

  listView = new EntityTreeView(splitter);
  listView->setSortingEnabled(false);
  listView->setModel(itemList);
  listView->setColumnHidden(3, true);
  splitter->addWidget(listView);

  layout->addWidget( splitter );

  browser = new QTextBrowser( splitter );
  splitter->addWidget(browser);

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
      browser->setText( QString::fromLatin1( ba ) );
    }
//   }
}

void ContactsWidget::modelDataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight)
{
}

#include "contactswidget.moc"
