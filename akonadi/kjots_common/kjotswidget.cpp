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
//  monitor->setMimeTypeMonitored( KJotsPage::mimeType() );
//   monitor->setCollectionMonitored( rootCollection );
//   monitor->fetchCollectionStatistics( false );

  Session *session = new Session( QByteArray( "EntityTreeModel-" ) + QByteArray::number( qrand() ), this );

//  EntityUpdateAdapter *eua = new EntityUpdateAdapter( session, this );

  etm = new Akonadi::EntityTreeModel(session, monitor, this); // now takes a session rather than an EUA
   etm->fetchMimeTypes(QStringList() << KJotsPage::mimeType());
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
//   proxy->setSourceModel(demp);
//   proxy->addMimeTypeExclusionFilter( Collection::mimeType() );

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
  treeview->setModel( etm );

  stackedWidget = new QStackedWidget( splitter );

  editor = new KTextEdit( stackedWidget );
  stackedWidget->addWidget( editor );

  layout->addWidget( splitter );

  browser = new QTextBrowser( stackedWidget );
  stackedWidget->addWidget( browser );
  stackedWidget->setCurrentWidget( browser );

  connect( treeview->selectionModel(),
      SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
      SLOT( selectionChanged( const QItemSelection &, const QItemSelection &) ) );

  connect( etm, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
      SLOT( dataChanged(const QModelIndex &, const QModelIndex &)));

}

KJotsWidget::~KJotsWidget()
{

}

void KJotsWidget::renderSelected()
{
  QTextDocument doc;
  QTextCursor cursor(&doc);

//   renderToBrowser(&cursor, selected.indexes());
  addBook( &cursor, treeview->currentIndex() );

// This should probably work.
//   browser->setDocument(&doc);
  browser->setText( doc.toPlainText() );
  stackedWidget->setCurrentWidget( browser );
}

// This stuff is for multiple selections.
// QModelIndexList KJotsWidget::squash(QModelIndexList list)
// {
//   QModelIndexList parents;
//   QModelIndexList retList;
//
//   // The number of 'collection' indexes is small.
//   // If list is ordered, then parents will come before children.
//   // Then I can build a list of parents as I go, and simply iterate
//   // over the list.
//
//   QModelIndex last;
//   bool ignore = false;
//   foreach(QModelIndex idx, list)
//   {
//     QModelIndex p = idx.parent();
//     while ((p = p.parent()).isValid())
//     {
//       if (parents.contains(p))
//       {
//         continue;
//       }
//     }
//
//     retList << idx;
//     if (etm->hasChildren(idx))
//     {
//       parents << idx;
//     }
//
//     last = idx;
//   }
//   return retList;
// }

// void KJotsWidget::renderToBrowser(QTextCursor *cursor, const QModelIndexList list )
// {
//   // remove children of selected indexes.
//   // list = squash(list);
//
//
//   foreach(QModelIndex idx, list)
//   {
//     if (hasSelectedParent(idx, list))
//       continue;
//
//     Item i = etm->data(idx, EntityTreeModel::ItemRole).value<Item>();
//     if ( i.isValid() )
//     {
//       KJotsPage page = i.payload<KJotsPage>();
//       addPage(cursor, page);
//     } else {
//       Collection c = etm->data(idx, EntityTreeModel::CollectionRole).value<Collection>();
//       if ( c.isValid() )
//       {
//         addBook(cursor, c);
//       }
//     }
//   }
//
// //   foreach(QItemSelectionRange range, selected)
// //   {
// //     foreach(QModelIndex idx, range )
// //     {
// //
// //     }
// //   }
// }

void KJotsWidget::addPage(QTextCursor *cursor, KJotsPage page)
{
  cursor->insertText("Title: " + page.title() + "\n");
  cursor->insertText("Content: " + page.content() + "\n");
}

void KJotsWidget::addBook(QTextCursor *cursor, QModelIndex idx)
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
      KJotsPage page = i.payload<KJotsPage>();
      addPage(cursor, page);
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

void KJotsWidget::selectionChanged ( const QItemSelection & selected, const QItemSelection & deselected )
{
  // Save any changes on the page we've just been editing.
  if (deselected.indexes().size() == 1)
  {
    QModelIndex idx = deselected.indexes().at(0);
    Item i = etm->data(idx, EntityTreeModel::ItemRole).value<Item>();
// // //     // Hmmm. document.isModified is false on subsequent
    kDebug() << idx << editor->document()->isModified();
    if ( i.isValid() && editor->document()->isModified() )
    {
        KJotsPage page = i.payload<KJotsPage>();
        page.setContent(editor->toPlainText());
        i.setPayload(page);
        etm->setData(idx, QVariant::fromValue(i), EntityTreeModel::ItemRole);
    } else {
      Collection col = etm->data(idx, EntityTreeModel::CollectionRole).value<Collection>();
      if (col.isValid())
      {
//         m_clientSideEntityStorage->purgeCollection(col.id());
//         treeview->collapse(idx);
      }
    }

  } // else nothing was changed.

  kDebug() << selected.indexes();

  if ( selected.indexes().size() == 1 )
  {
    // Only one item selected. If it's a book, render it. If it's a page, display it for editing.
    QModelIndex idx = selected.indexes().at(0);
    Item i = etm->data( idx, EntityTreeModel::ItemRole ).value< Item >();
    if ( i.isValid() )
    {
      kDebug() << i.payloadData();
      KJotsPage page = i.payload< KJotsPage >();
      editor->setText( page.content() );
      stackedWidget->setCurrentWidget( editor );
      return;
    }
    Collection col = etm->data(idx, EntityTreeModel::CollectionRole).value<Collection>();
//     if (col.isValid() && m_clientSideEntityStorage->canPopulate(col.id()))
//     {
//       m_clientSideEntityStorage->populateCollection(col.id());
      //         treeview->collapse(idx);
//     }
  }
  // selected is not a single page. May be a book or a range of books/pages.
  // Render the selection to the browser.
  renderSelected();
}

void KJotsWidget::dataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight)
{
  QItemSelectionRange changedRange(topLeft, bottomRight);

  QModelIndexList selectedIndexes = treeview->selectionModel()->selectedIndexes();

  if (topLeft == bottomRight)
  {
  kDebug() << topLeft;
    Item i = etm->data(topLeft, EntityTreeModel::ItemRole).value<Item>();
    if (i.isValid())
    {
      kDebug() << i.payloadData();
      // Check if the item updated is the item being edited and offer a conflict dialog.
//       if ()


    } else {
      Collection c = etm->data(topLeft, EntityTreeModel::CollectionRole).value<Collection>();
      if (c.isValid())
      {
        // A collection was updated.
      }
    }

  }

//   if ( treeview->selectionModel()->selectedIndexes().size() == 1)
//   {
//     QModelIndex idx = selectedRange.indexes().at(0);
//     Item i = etm->data(idx, EntityTreeModel::ItemRole).value<Item>();
//     if (i.isValid())
//     {
//     }
//   }


  foreach( QItemSelectionRange selectedRange, treeview->selectionModel()->selection() )
  {
   if ( selectedRange.intersects( changedRange ) )
    {
      if ( selectedRange.indexes().size() == 1)
      {
        // TODO: This might be a really bad idea. Don't update a page that I am currently editing.
        // How should I handle these conflicts?
        // Notify the user with a dialog?
        QModelIndex idx = selectedRange.indexes().at(0);
        Item i = etm->data(idx, EntityTreeModel::ItemRole).value<Item>();
        if (i.isValid())
        {
          KJotsPage page = i.payload<KJotsPage>();
          editor->setText(page.content());
          return;
        }
      }

      renderSelected();
      return;
    } else {
      foreach( QModelIndex p, changedRange.indexes() )
      {
        while ((p = p.parent()).isValid())
        {
          if (selectedRange.indexes().contains(p))
          {
            renderSelected();
            return;
          }
        }
      }
    }
  }
}

#include "kjotswidget.moc"
