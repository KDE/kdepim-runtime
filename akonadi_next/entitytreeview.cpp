/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>
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

#include "entitytreeview.h"

#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtGui/QApplication>
#include <QtGui/QDragMoveEvent>
#include <QtGui/QHeaderView>
#include <QtGui/QMenu>

#include <KAction>
#include <KLocale>
#include <KMessageBox>
#include <KUrl>
#include <KXMLGUIFactory>
#include <KXmlGuiWindow>

#include <akonadi/collection.h>
#include <akonadi/control.h>
#include <akonadi/item.h>
#include "entitytreemodel.h"

#include <kdebug.h>

using namespace Akonadi;

/**
 * @internal
 */
class EntityTreeView::Private
{
public:
  Private( EntityTreeView *parent )
      : mParent( parent ),
      xmlGuiWindow( 0 ), showChildCollectionTree(false) {
  }

  void init();
  void dragExpand();
  void itemClicked( const QModelIndex& );
  void itemCurrentChanged( const QModelIndex& );
  bool hasParent( const QModelIndex& idx, Collection::Id parentId );

  EntityTreeView *mParent;
  QModelIndex dragOverIndex;
  QTimer dragExpandTimer;

  KXmlGuiWindow *xmlGuiWindow;
  bool showChildCollectionTree;
};

void EntityTreeView::showChildCollectionTree( bool include )
{
  d->showChildCollectionTree = include;
}

bool EntityTreeView::childCollectionTreeShown()
{
  return d->showChildCollectionTree;
}

void EntityTreeView::Private::init()
{
  mParent->header()->setClickable( true );
  mParent->header()->setStretchLastSection( false );
//   mParent->setRootIsDecorated( false );

//   mParent->setAutoExpandDelay ( QApplication::startDragTime() );

  mParent->setSortingEnabled( true );
  mParent->sortByColumn( 0, Qt::AscendingOrder );
  mParent->setEditTriggers( QAbstractItemView::EditKeyPressed );
  mParent->setAcceptDrops( true );
  mParent->setDropIndicatorShown( true );
  mParent->setDragDropMode( DragDrop );
  mParent->setDragEnabled( true );

  dragExpandTimer.setSingleShot( true );
  mParent->connect( &dragExpandTimer, SIGNAL( timeout() ), SLOT( dragExpand() ) );

  mParent->connect( mParent, SIGNAL( clicked( const QModelIndex& ) ),
                    mParent, SLOT( itemClicked( const QModelIndex& ) ) );

  Control::widgetNeedsAkonadi( mParent );
}

bool EntityTreeView::Private::hasParent( const QModelIndex& idx, Collection::Id parentId )
{
  QModelIndex idx2 = idx;
  while ( idx2.isValid() ) {
    if ( mParent->model()->data( idx2, EntityTreeModel::CollectionIdRole ).toLongLong() == parentId )
      return true;

    idx2 = idx2.parent();
  }
  return false;
}

void EntityTreeView::Private::dragExpand()
{
  mParent->setExpanded( dragOverIndex, true );
  dragOverIndex = QModelIndex();
}

void EntityTreeView::Private::itemClicked( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  const Collection col = index.model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
  if ( !col.isValid() )
    return;

  emit mParent->clicked( col );
}

void EntityTreeView::Private::itemCurrentChanged( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  const Collection col = index.model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
  if ( !col.isValid() )
    return;

  emit mParent->currentChanged( col );
}

EntityTreeView::EntityTreeView( QWidget * parent ) :
    QTreeView( parent ),
    d( new Private( this ) )
{

  setSelectionMode( QAbstractItemView::SingleSelection );
  d->init();
}

EntityTreeView::EntityTreeView( KXmlGuiWindow *xmlGuiWindow, QWidget * parent ) :
    QTreeView( parent ),
    d( new Private( this ) )
{
  d->xmlGuiWindow = xmlGuiWindow;
  d->init();
}

EntityTreeView::~EntityTreeView()
{
  delete d;
}

void EntityTreeView::setRootIndex(const QModelIndex &idx)
{
  QAbstractItemView::setRootIndex(idx);
  if (!d->showChildCollectionTree)
  {
    QModelIndex rowIndex = idx.child( 0, 0);
    while (rowIndex.isValid())
    {
      if ( model()->data( rowIndex, EntityTreeModel::MimeTypeRole ) == Collection::mimeType() )
      {
        setRowHidden( rowIndex.row(), idx, true );
      }
      rowIndex = rowIndex.sibling( rowIndex.row() + 1, rowIndex.column() );
    }
  }
}

void EntityTreeView::setModel( QAbstractItemModel * model )
{
  QTreeView::setModel( model );
  header()->setStretchLastSection( true );

  connect( selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ),
           this, SLOT( itemCurrentChanged( const QModelIndex& ) ) );
}

void EntityTreeView::dragMoveEvent( QDragMoveEvent * event )
{
  QModelIndex index = indexAt( event->pos() );
  if ( d->dragOverIndex != index ) {
    d->dragExpandTimer.stop();
    if ( index.isValid() && !isExpanded( index ) && itemsExpandable() ) {
      d->dragExpandTimer.start( QApplication::startDragTime() );
      d->dragOverIndex = index;
    }
  }

  // Check if the collection under the cursor accepts this data type
  Collection col = model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
  if (!col.isValid())
  {
    Item item = model()->data( index, EntityTreeModel::ItemRole ).value<Item>();
    if (item.isValid())
    {
      col = model()->data( index.parent(), EntityTreeModel::CollectionRole ).value<Collection>();
    }
  }
  if ( col.isValid() )
  {
    QStringList supportedContentTypes = col.contentMimeTypes();
    const QMimeData *data = event->mimeData();
    KUrl::List urls = KUrl::List::fromMimeData( data );
    foreach( const KUrl &url, urls ) {
      const Collection collection = Collection::fromUrl( url );
      if ( collection.isValid() ) {
        if ( !supportedContentTypes.contains( Collection::mimeType() ) )
          break;

        // Check if we don't try to drop on one of the children
        if ( d->hasParent( index, collection.id() ) )
          break;
      } else { // This is an item.
        QString type = url.queryItems()[ QString::fromLatin1( "type" )];
        if ( !supportedContentTypes.contains( type ) )
          break;
      }
      // All urls are supported. process the event.
      QTreeView::dragMoveEvent( event );
      return;
    }
  }

  event->setDropAction( Qt::IgnoreAction );
  return;
}

void EntityTreeView::dragLeaveEvent( QDragLeaveEvent * event )
{
  d->dragExpandTimer.stop();
  d->dragOverIndex = QModelIndex();
  QTreeView::dragLeaveEvent( event );
}


void EntityTreeView::dropEvent( QDropEvent * event )
{
  d->dragExpandTimer.stop();
  d->dragOverIndex = QModelIndex();

  QModelIndexList idxs = selectedIndexes();


  QMenu popup( this );
  QAction* moveDropAction;
  // TODO If possible, hide unavailable actions ...
  // Use the model to determine if a move is ok.
//   if (...)
//   {
  moveDropAction = popup.addAction( KIcon( QString::fromLatin1( "edit-rename" ) ), i18n( "&Move here" ) );
//   }

  //TODO: If dropping on one of the selectedIndexes, just return.
  // open a context menu offering different drop actions (move, copy and cancel)
  QAction* copyDropAction = popup.addAction( KIcon( QString::fromLatin1( "edit-copy" ) ), i18n( "&Copy here" ) );
  popup.addSeparator();
  popup.addAction( KIcon( QString::fromLatin1( "process-stop" ) ), i18n( "Cancel" ) );

  QAction *activatedAction = popup.exec( QCursor::pos() );

  if ( activatedAction == moveDropAction ) {
    event->setDropAction( Qt::MoveAction );
  } else if ( activatedAction == copyDropAction ) {
    event->setDropAction( Qt::CopyAction );
  }
  // TODO: Handle link action.
  else return;

  QTreeView::dropEvent( event );
}

void EntityTreeView::contextMenuEvent( QContextMenuEvent * event )
{
  if ( !d->xmlGuiWindow )
    return;
  QMenu *popup = static_cast<QMenu*>( d->xmlGuiWindow->guiFactory()->container(
                                        QLatin1String( "akonadi_collectionview_contextmenu" ), d->xmlGuiWindow ) );
  if ( popup )
    popup->exec( event->globalPos() );
}

void EntityTreeView::setXmlGuiWindow( KXmlGuiWindow * xmlGuiWindow )
{
  d->xmlGuiWindow = xmlGuiWindow;
}

#include "entitytreeview.moc"
