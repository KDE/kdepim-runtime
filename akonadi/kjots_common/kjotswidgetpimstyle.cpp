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

#include "kjotswidgetpimstyle.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QStackedWidget>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextBrowser>

#include <QSortFilterProxyModel>

#include <akonadi/control.h>
#include "collectionchildorderattribute.h"
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionmodel.h>
#include "descendantentitiesproxymodel.h"
#include <akonadi/entitydisplayattribute.h>
#include "entitytreemodel.h"
#include "entityfilterproxymodel.h"
#include "entitytreeview.h"
#include "entityupdateadapter.h"
#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>

#include <KTextEdit>

// #include "kjotsbookshelf.h"
#include "kjotspage.h"

#include <kdebug.h>
#include "modeltest.h"

using namespace Akonadi;

KJotsWidgetPimStyle::KJotsWidgetPimStyle( QWidget * parent, Qt::WindowFlags f )
    : QWidget( parent, f )
{

  QSplitter *splitter = new QSplitter( this );
  QHBoxLayout *layout = new QHBoxLayout( this );

//   bookshelf = new KJotsBookshelf( splitter );

  treeview = new EntityTreeView( splitter );

  if ( !Akonadi::Control::start() ) {
    kFatal() << "Unable to start Akonadi server, exit application";
    return;
  }

  // TODO: Tell akonadi to load the "akonadi_kjots_resource" if it's not already loaded.
  // This can't be done synchronously, but the monitor will notify the model that it has new collections
  // when the job is done.

  // Check  Akonadi::AgentManager::self()->instances() if it contains one, if it doesn't create one and report errors
  // if necessary.


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
//   monitor->setCollectionMonitored( rootCollection );
//   monitor->fetchCollectionStatistics( false );

  Session *session = new Session( QByteArray( "EntityTreeModel-" ) + QByteArray::number( qrand() ), this );
  EntityUpdateAdapter *eua = new EntityUpdateAdapter( session, this );

//   ClientSideEntityStorage *cses = new ClientSideEntityStorage( monitor,
//                                                                QStringList() << KJotsPage::mimeType(),
//                                                                rootCollection,
//                                                                this);

//   etm = new Akonadi::EntityTreeModel( eua, cses, this);
  etm = new Akonadi::EntityTreeModel( eua, monitor, this);
  etm->fetchMimeTypes(QStringList() << KJotsPage::mimeType());

  treeproxy = new EntityFilterProxyModel(this);

  treeproxy->setSourceModel(etm);
  treeproxy->addMimeTypeInclusionFilter( Collection::mimeType() );

//   QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);

//   proxy = new Akonadi::EntityOrderProxyModel(this);
//   proxy = new Akonadi::EntitySortFilterProxyModel( this );
//   proxy->addMimeTypeFilters( QStringList()
//                              << KJotsPage::mimeType() );
//   proxy->setDynamicSortFilter ( true );
//   proxy->setSourceModel( etm );
//   proxy->sort( 0 );  // Sort ascending.

//   new ModelTest(proxy, this);
//   new ModelTest( etm, this );

//   bookshelf->setModel( etm );
//   bookshelf->setModel( proxy );

  treeview->setModel(treeproxy);
//   treeview->setModel( etm );

  stackedWidget = new QStackedWidget( splitter );

  QSplitter *hSplitter = new QSplitter(Qt::Vertical, splitter);

//   QVBoxLayout *vl = new QVBoxLayout( stackedWidget );

  descList = new DescendantEntitiesProxyModel(this);
  descList->setSourceModel(etm);

  listproxy = new EntityFilterProxyModel(this);
//   listproxy->setSourceModel(etm);
  listproxy->setSourceModel(descList);

  listproxy->addMimeTypeInclusionFilter( KJotsPage::mimeType() );
//   listproxy->includeChildCollectionTree(false);

//   listproxy->addMimeTypeExclusionFilter( Collection::mimeType() );

  itemlist = new EntityTreeView(stackedWidget);
  itemlist->setModel(etm);
  itemlist->showChildCollectionTree(false);
//   itemlist->setModel(listproxy);
  hSplitter->addWidget(itemlist);
  hSplitter->addWidget(stackedWidget);

  editor = new KTextEdit( stackedWidget );
  stackedWidget->addWidget( editor );

//   stackedWidget->setLayout(vl);
  layout->addWidget( splitter );
//   layout->addLayout(vl);

  browser = new QTextBrowser( stackedWidget );
  stackedWidget->addWidget( browser );
  stackedWidget->setCurrentWidget( browser );
//   vl->addWidget( stackedWidget );

  connect( treeview->selectionModel(),
      SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
      SLOT( treeSelectionChanged ( const QItemSelection &, const QItemSelection & ) ) );

  connect( itemlist->selectionModel(),
      SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
      SLOT( listSelectionChanged ( const QItemSelection &, const QItemSelection & ) ) );

//   connect( treeview, SIGNAL( clicked( const QModelIndex & ) ), SLOT( showPages( const QModelIndex & ) ) );

  connect( itemlist, SIGNAL( clicked( const QModelIndex & ) ), SLOT( editPage( const QModelIndex & ) ) );

  connect( etm, SIGNAL( dataChanged( const QModelIndex &, const QModelIndex & ) ),
      SLOT( modelDataChanged( const QModelIndex &, const QModelIndex & ) ) );
}

void KJotsWidgetPimStyle::editPage(const QModelIndex &idx)
{
  Item i = etm->data( idx, EntityTreeModel::ItemRole ).value< Item >();
  if ( i.isValid() )
  {
    KJotsPage page = i.payload< KJotsPage >();
    editor->setText( page.content() );
    stackedWidget->setCurrentWidget( editor );
  }
}

KJotsWidgetPimStyle::~KJotsWidgetPimStyle()
{

}

void KJotsWidgetPimStyle::savePage( const QModelIndex & idx )
{
  kDebug() << idx;
    Item i = etm->data(idx, EntityTreeModel::ItemRole).value<Item>();
    if ( i.isValid() && editor->document()->isModified() )
    {
kDebug() << "foo";
        KJotsPage page = i.payload<KJotsPage>();
        page.setContent(editor->toPlainText());
        i.setPayload(page);
        etm->setData(idx, QVariant::fromValue(i), EntityTreeModel::ItemRole);
    }
}

void KJotsWidgetPimStyle::listSelectionChanged( const QItemSelection & selected, const QItemSelection &deselected )
{
  // Save any changes on the page we've just been editing.
  if ( deselected.indexes().size() == 1 )
  {
    savePage( deselected.indexes().at( 0 ) );
  }

}

void KJotsWidgetPimStyle::treeSelectionChanged ( const QItemSelection & selected, const QItemSelection & deselected )
{
  if ( selected.indexes().size() == 1 )
  {
    // Only one item selected. If it's a book, render it. If it's a page, display it for editing.
    QModelIndex idx = selected.indexes().at(0);

//     itemlist->setRootIndex( listproxy->mapFromSource( treeproxy->mapToSource( idx ) ) );
    QModelIndex etmIndex = treeproxy->mapToSource( idx );
    descList->setRootIndex(etmIndex);
    QModelIndex descListIndex = descList->mapFromSource(etmIndex);
    listproxy->setRootIndex(descListIndex);
    QModelIndex filteredListIndex = listproxy->mapFromSource(descListIndex);

    itemlist->setRootIndex(filteredListIndex);

//     itemlist->setRootIndex( treeproxy->mapToSource( idx ) );
  }
}

void KJotsWidgetPimStyle::modelDataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight)
{
  QItemSelectionRange changedRange(topLeft, bottomRight);

  QModelIndexList selectedIndexes = treeview->selectionModel()->selectedIndexes();

  if (topLeft == bottomRight)
  {
    Item i = etm->data(topLeft, EntityTreeModel::ItemRole).value<Item>();
    if (i.isValid())
    {
      // Check if the item updated is the item being edited and offer a conflict dialog.
      if ( selectedIndexes.size() == 1 && selectedIndexes[0] == topLeft )
      {
        // Oops. The item being updated by akonadi is the item currently being edited in kjots.
        // The update from akonadi is probably rouge. We ignore it and overwrite it in a moment.

      } else {
        // A random
      }
    } else {
      Collection c = etm->data(topLeft, EntityTreeModel::CollectionRole).value<Collection>();
      if (c.isValid())
      {
        // A collection was updated.
      }
    }

  }

  foreach( QItemSelectionRange selectedRange, treeview->selectionModel()->selection() )
  {
   if ( selectedRange.intersects( changedRange ) )
    {
      if ( selectedRange.indexes().size() == 1)
      {
        // TODO: This might be a really bad idea. Don't update a page that I am currently editing.
        // How should I handle these conflicts?
        // Notify the user with a dialog?

  // No. It must be a rogue edit. Ignore it and overwrite it now or later.
        if (!editor->document()->isModified())
        {
          QModelIndex idx = selectedRange.indexes().at(0);
          Item i = etm->data(idx, EntityTreeModel::ItemRole).value<Item>();
          if (i.isValid())
          {
            KJotsPage page = i.payload<KJotsPage>();
            editor->setText(page.content());
            return;
          }
        }
      }
      return;
    } else {
      foreach( QModelIndex p, changedRange.indexes() )
      {
        while ((p = p.parent()).isValid())
        {
          if (selectedRange.indexes().contains(p))
          {
            return;
          }
        }
      }
    }
  }
}

#include "kjotswidgetpimstyle.moc"