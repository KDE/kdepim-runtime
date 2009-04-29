/*
    This file is part of KJots.

    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

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

#include "kjotswidget.h"

#include <QHBoxLayout>
#include <QSplitter>
#include <QStackedWidget>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextBrowser>

#include <QDirModel>
#include <QColumnView>

#include <QSortFilterProxyModel>

#include <akonadi/agentinstancecreatejob.h>
#include <akonadi/agentmanager.h>
#include <akonadi/agenttype.h>
#include <akonadi/control.h>
#include "collectionchildorderattribute.h"
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionmodel.h>
#include "descendantentitiesproxymodel.h"
#include <akonadi/entitydisplayattribute.h>
#include "entityfilterproxymodel.h"
#include "entitytreemodel.h"
#include "entitytreeview.h"
#include "entityupdateadapter.h"
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>
#include "selectionproxymodel.h"

#include <KTextEdit>

#include "kjotspage.h"

#include <kdebug.h>
#include "modeltest.h"

using namespace Akonadi;

KJotsWidget::KJotsWidget( QWidget * parent, Qt::WindowFlags f )
    : QWidget( parent, f )
{

  QSplitter *splitter = new QSplitter( this );
  QHBoxLayout *layout = new QHBoxLayout( this );

  treeview = new EntityTreeView( splitter );
//   treeview = new QColumnView(splitter);
//   treeview->setSortingEnabled(false);

  if ( !Akonadi::Control::start() ) {
    kFatal() << "Unable to start Akonadi server, exit application";
    return;
  }

  // TODO: Tell akonadi to load the "akonadi_kjots_resource" if it's not already loaded.
  // This can't be done synchronously, but the monitor will notify the model that it has new collections
  // when the job is done.

  // Check  Akonadi::AgentManager::self()->instances() if it contains one, if it doesn't create one and report errors
  // if necessary.

//   Akonadi::AgentManager *manager = Akonadi::AgentManager::self();
//
//   Akonadi::AgentType::List types = manager->types();
//   foreach( const Akonadi::AgentType &type, types ) {
//     qDebug() << "Type:" << type.name() << type.description() << type.identifier();
//     Akonadi::AgentInstance instance = manager->instance(type.identifier());
// // //     kDebug() << "instance:" << instance.identifier() << instance.name();
// // //     instance.setIsOnline(true);
//     instance.synchronize();
// //     Akonadi::AgentInstanceCreateJob *job = new Akonadi::AgentInstanceCreateJob( type );
// //     job->start();
//   }
// //   kFatal();

//   kDebug() << "ins";
//   foreach( Akonadi::AgentInstance instance, manager->instances() ) {
//     qDebug() << "Type:" << instance.name() << instance.identifier();
//     instance.setIsOnline(true);
//     instance.synchronize();
// //     Akonadi::AgentInstance instance = m
//   }
//   kDebug() << "ins done";

  // Use Collection::root as the top level 'bookshelf'
  Collection rootCollection = Collection::root();

  ItemFetchScope scope;
  scope.fetchFullPayload( true ); // Need to have full item when adding it to the internal data structure
//   scope.fetchAttribute< CollectionChildOrderAttribute >();
  scope.fetchAttribute< EntityDisplayAttribute >();

  Monitor *monitor = new Monitor( this );
  monitor->fetchCollection( true );
  monitor->setItemFetchScope( scope );
  monitor->setCollectionMonitored( Collection::root() );
  monitor->setMimeTypeMonitored( KJotsPage::mimeType() );
//   monitor->setCollectionMonitored( rootCollection );
//   monitor->fetchCollectionStatistics( false );

  Session *session = new Session( QByteArray( "EntityTreeModel-" ) + QByteArray::number( qrand() ), this );

  etm = new Akonadi::EntityTreeModel(session, monitor, this); // now takes a session rather than an EUA
//   etm->setItemPopulationStrategy(EntityTreeModel::NoItemPopulation);
//   etm->setItemPopulationStrategy(EntityTreeModel::LazyPopulation);
//   etm->setIncludeRootCollection(true);
//   etm->setRootCollectionDisplayName("[All]");

//   DescendantEntitiesProxyModel *demp = new DescendantEntitiesProxyModel( //QModelIndex(),
//   this );
//   demp->setSourceModel(etm);
//   demp->setRootIndex(QModelIndex());

//   demp->setDisplayAncestorData(true, QString(" / "));

//   EntityFilterProxyModel *proxy = new EntityFilterProxyModel();
//   proxy->setSourceModel(etm);
//   proxy->addMimeTypeInclusionFilter( Collection::mimeType() );

//   new ModelTest(proxy, this);
  new ModelTest( etm, this );
//   new ModelTest( dem, this );
//   new ModelTest( demp, this );

//   treeview->setModel(proxy);
//   treeview->setModel( dem );
//   treeview->setModel( demp );
//   treeview->setModel( fcp );

//   QDirModel *dm = new QDirModel(this);
//   treeview->setModel( dm );
//   treeview->setModel( proxy );
  treeview->setModel( etm );
  treeview->setSelectionMode(QAbstractItemView::ExtendedSelection);

  selProxy = new SelectionProxyModel(treeview->selectionModel(), this);
  selProxy->setSourceModel(etm);

  // TODO: Write a QAbstractItemView subclass to render kjots selection.
  connect(selProxy, SIGNAL(rowsInserted(const QModelIndex &, int, int)), SLOT(renderSelection()));
  connect(selProxy, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), SLOT(renderSelection()));

  stackedWidget = new QStackedWidget( splitter );

  editor = new KTextEdit( stackedWidget );
  stackedWidget->addWidget( editor );

  layout->addWidget( splitter );

  browser = new QTextBrowser( stackedWidget );
  stackedWidget->addWidget( browser );
  stackedWidget->setCurrentWidget( browser );

  connect( etm, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
      SLOT( dataChanged(const QModelIndex &, const QModelIndex &)));

}

KJotsWidget::~KJotsWidget()
{

}

void KJotsWidget::renderSelection()
{
  int rows = selProxy->rowCount();

  // If the selection is a single page, present it for editing...
  if (rows == 1)
  {
    QModelIndex idx = selProxy->index( 0, 0, QModelIndex());
    Item item = idx.data(EntityTreeModel::ItemRole).value<Item>();
    if (item.isValid())
    {
      KJotsPage page = item.payload<KJotsPage>();
      editor->setText( page.content() );
      stackedWidget->setCurrentWidget( editor );
      return;
    }
  }

  // ... Otherwise, render the selection read-only.

  QTextDocument doc;
  QTextCursor cursor(&doc);

  const int column = 0;
  for (int row = 0; row < rows; ++row)
  {
    QModelIndex idx = selProxy->index(row, column, QModelIndex());
    Collection col = idx.data(EntityTreeModel::CollectionRole).value<Collection>();
    if (col.isValid())
    {
      addBook( &cursor, idx);
    }
    Item item = idx.data(EntityTreeModel::ItemRole).value<Item>();
    if (item.isValid())
    {
      addPage( &cursor, idx);
    }
  }
  browser->setText( doc.toPlainText() );
  stackedWidget->setCurrentWidget( browser );
}

void KJotsWidget::addPage(QTextCursor *cursor, const QModelIndex &idx)
{
  Item item = etm->data( idx, EntityTreeModel::ItemRole ).value< Item >();
  KJotsPage page = item.payload<KJotsPage>();
  cursor->insertText("Title: " + page.title() + "\n");
  cursor->insertText("Content: " + page.content() + "\n");
}

void KJotsWidget::addBook(QTextCursor *cursor, const QModelIndex &idx)
{
  Collection c = etm->data( idx, EntityTreeModel::CollectionRole ).value< Collection >();
  cursor->insertText( "Begin book: " + c.name() + "\n" );
  int rowCount = etm->rowCount(idx);
  for( int row = 0; row < rowCount; row++ )
  {
    QModelIndex bookContentIndex = etm->index(row, idx.column(), idx);
    Item i = etm->data( bookContentIndex, EntityTreeModel::ItemRole ).value< Item >();

    if (i.isValid())
    {
      addPage(cursor, bookContentIndex);
    } else {
      Collection c = etm->data( bookContentIndex, EntityTreeModel::CollectionRole ).value<Collection>();
      if (c.isValid())
      {
          addBook(cursor, bookContentIndex);
      }
    }
  }
  cursor->insertText( "End book: " + c.name() + "\n" );
}

#include "kjotswidget.moc"
