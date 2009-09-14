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

#include <kxmlguiclient.h>

#include <akonadi/collection.h>
#include <akonadi/control.h>
#include <akonadi/item.h>
#include <akonadi/entitytreemodel.h>

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
      xmlGuiClient( 0 )
  {
  }

  void init();
  void dragExpand();
  void itemClicked( const QModelIndex& );
  void itemDoubleClicked( const QModelIndex& );
  void itemCurrentChanged( const QModelIndex& );

  void slotSelectionChanged( const QItemSelection & selected, const QItemSelection & deselected );

  bool hasParent( const QModelIndex& idx, Collection::Id parentId );
  Collection currentDropTarget( QDropEvent* event ) const;

  EntityTreeView *mParent;
  QModelIndex dragOverIndex;
  QTimer dragExpandTimer;

  KXMLGUIClient *xmlGuiClient;
};
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
  mParent->connect( mParent, SIGNAL( doubleClicked( const QModelIndex& ) ),
                    mParent, SLOT( itemDoubleClicked( const QModelIndex& ) ) );

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

void EntityTreeView::Private::slotSelectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
  const int column = 0;
  foreach (const QItemSelectionRange &range, selected)
  {
    QModelIndex idx = range.topLeft();
    if (idx.column() > 0)
      continue;
    for (int row = idx.row(); row <= range.bottomRight().row(); ++row)
    {
      // Don't use canFetchMore here. We need to bypass the check in 
      // the EntityFilterModel when it shows only collections.
      mParent->model()->fetchMore(idx.sibling(row, column));
    }
  }
}

void EntityTreeView::Private::itemClicked( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  const Collection collection = index.model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
  if ( collection.isValid() ) {
    emit mParent->clicked( collection );
  } else {
    const Item item = index.model()->data( index, EntityTreeModel::ItemRole ).value<Item>();
    if ( item.isValid() )
      emit mParent->clicked( item );
  }
}

void EntityTreeView::Private::itemDoubleClicked( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  const Collection collection = index.model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
  if ( collection.isValid() ) {
    emit mParent->doubleClicked( collection );
  } else {
    const Item item = index.model()->data( index, EntityTreeModel::ItemRole ).value<Item>();
    if ( item.isValid() )
      emit mParent->doubleClicked( item );
  }
}

void EntityTreeView::Private::itemCurrentChanged( const QModelIndex &index )
{
  if ( !index.isValid() )
    return;

  const Collection collection = index.model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
  if ( collection.isValid() ) {
    emit mParent->currentChanged( collection );
  } else {
    const Item item = index.model()->data( index, EntityTreeModel::ItemRole ).value<Item>();
    if ( item.isValid() )
      emit mParent->currentChanged( item );
  }
}


Collection EntityTreeView::Private::currentDropTarget(QDropEvent* event) const
{
  const QModelIndex index = mParent->indexAt( event->pos() );
  Collection col = mParent->model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
  if ( !col.isValid() ) {
    const Item item = mParent->model()->data( index, EntityTreeModel::ItemRole ).value<Item>();
    if ( item.isValid() )
      col = mParent->model()->data( index.parent(), EntityTreeModel::CollectionRole ).value<Collection>();
  }
  return col;
}


EntityTreeView::EntityTreeView( QWidget * parent ) :
    QTreeView( parent ),
    d( new Private( this ) )
{

  setSelectionMode( QAbstractItemView::SingleSelection );
  d->init();
}

EntityTreeView::EntityTreeView( KXMLGUIClient *xmlGuiClient, QWidget * parent ) :
    QTreeView( parent ),
    d( new Private( this ) )
{
  d->xmlGuiClient = xmlGuiClient;
  d->init();
}

EntityTreeView::~EntityTreeView()
{
  delete d;
}

void EntityTreeView::setModel( QAbstractItemModel * model )
{
  QTreeView::setModel( model );
  header()->setStretchLastSection( true );

  connect( selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ),
           this, SLOT( itemCurrentChanged( const QModelIndex& ) ) );

  connect( selectionModel(), SIGNAL( selectionChanged( const QItemSelection &, const QItemSelection & ) ),
           SLOT( slotSelectionChanged( const QItemSelection &, const QItemSelection & ) ) );
}

void EntityTreeView::dragMoveEvent( QDragMoveEvent * event )
{
  const QModelIndex index = indexAt( event->pos() );
  if ( d->dragOverIndex != index ) {
    d->dragExpandTimer.stop();
    if ( index.isValid() && !isExpanded( index ) && itemsExpandable() ) {
      d->dragExpandTimer.start( QApplication::startDragTime() );
      d->dragOverIndex = index;
    }
  }

  // Check if the collection under the cursor accepts this data type
  const Collection col = d->currentDropTarget( event );
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

  const Collection target = d->currentDropTarget( event );
  if ( !target.isValid() )
    return;

  QMenu popup( this );
  int actionCount = 0;
  Qt::DropAction defaultAction;
  QAction* moveDropAction = 0;
  // TODO check if the source supports moving

  if ( (target.rights() & (Collection::CanCreateCollection | Collection::CanCreateItem))
    && (event->possibleActions() & Qt::MoveAction) ) {
    moveDropAction = popup.addAction( KIcon( QString::fromLatin1( "edit-rename" ) ), i18n( "&Move here" ) );
    ++actionCount;
    defaultAction = Qt::MoveAction;
  }
  QAction* copyDropAction = 0;
  if ( (target.rights() & (Collection::CanCreateCollection | Collection::CanCreateItem))
    && (event->possibleActions() & Qt::CopyAction) ) {
    copyDropAction = popup.addAction( KIcon( QString::fromLatin1( "edit-copy" ) ), i18n( "&Copy here" ) );
    ++actionCount;
    defaultAction = Qt::CopyAction;
  }
  QAction* linkAction = 0;
  if ( (target.rights() & Collection::CanLinkItem) && (event->possibleActions() & Qt::LinkAction) ) {
    linkAction = popup.addAction( KIcon( QLatin1String( "edit-link" ) ), i18n( "&Link here" ) );
    ++actionCount;
    defaultAction = Qt::LinkAction;
  }

  if ( actionCount == 0 ) {
    kDebug() << "Cannot drop here:" << event->possibleActions() << model()->supportedDragActions() << model()->supportedDropActions();
    return;
  }

  if ( actionCount == 1 ) {
    kDebug() << "Selecting drop action" << defaultAction << ", there are no other possibilities";
    event->setDropAction( defaultAction );
    QTreeView::dropEvent( event );
    return;
  }

  popup.addSeparator();
  popup.addAction( KIcon( QString::fromLatin1( "process-stop" ) ), i18n( "Cancel" ) );

  QAction *activatedAction = popup.exec( QCursor::pos() );

  if ( !activatedAction ) {
    return;
  } else if ( activatedAction == moveDropAction ) {
    event->setDropAction( Qt::MoveAction );
  } else if ( activatedAction == copyDropAction ) {
    event->setDropAction( Qt::CopyAction );
  } else if ( activatedAction == linkAction ) {
    event->setDropAction( Qt::LinkAction );
  } else {
    return;
  }
  QTreeView::dropEvent( event );
}

void EntityTreeView::contextMenuEvent( QContextMenuEvent * event )
{
  if ( !d->xmlGuiClient )
    return;

  const QModelIndex index = indexAt( event->pos() );

  QMenu *popup = 0;

  // check if the index under the cursor is a collection or item
  const Item item = model()->data( index, EntityTreeModel::ItemRole ).value<Item>();
  if ( item.isValid() )
    popup = static_cast<QMenu*>( d->xmlGuiClient->factory()->container(
                                 QLatin1String( "akonadi_itemview_contextmenu" ), d->xmlGuiClient ) );
  else
    popup = static_cast<QMenu*>( d->xmlGuiClient->factory()->container(
                                 QLatin1String( "akonadi_collectionview_contextmenu" ), d->xmlGuiClient ) );
  if ( popup )
    popup->exec( event->globalPos() );
}

void EntityTreeView::setXmlGuiClient( KXMLGUIClient * xmlGuiClient )
{
  d->xmlGuiClient = xmlGuiClient;
}

void EntityTreeView::startDrag(Qt::DropActions _supportedActions)
{
  QModelIndexList indexes;
  bool sourceReadable = true;
  foreach ( const QModelIndex &index, selectionModel()->selectedRows() ) {
    if ( !model()->flags( index ) & Qt::ItemIsDragEnabled )
      continue;

    if ( sourceReadable ) {
      Collection source = index.data( EntityTreeModel::CollectionRole ).value<Collection>();
      if ( !source.isValid() ) {
        // index points to an item
        source = index.data( EntityTreeModel::ParentCollectionRole ).value<Collection>();
        sourceReadable = source.rights() & Collection::CanDeleteItem;
      } else {
        // index points to a collection
        sourceReadable = source.rights() & Collection::CanDeleteCollection;
      }
    }

    indexes.append( index );
  }

  if ( indexes.isEmpty() )
    return;

  QMimeData *mimeData = model()->mimeData( indexes );
  if ( !mimeData )
    return;

  QDrag *drag = new QDrag( this );
  drag->setMimeData( mimeData );
  kDebug() << indexes.size();
  if ( indexes.size() > 1 ) {
    drag->setPixmap( KIcon( "document-multiple" ).pixmap( QSize( 22, 22 ) ) );
  } else {
    QPixmap pixmap = indexes.first().data( Qt::DecorationRole ).value<QIcon>().pixmap( QSize( 22, 22 ) );
    if ( pixmap.isNull() )
      pixmap = KIcon( "text-plain" ).pixmap( QSize( 22, 22 ) );
    drag->setPixmap( pixmap );
  }

  Qt::DropActions supportedActions( _supportedActions );
  if ( !sourceReadable )
    supportedActions &= ~Qt::MoveAction;
  drag->exec( supportedActions, Qt::CopyAction );
}

#include "entitytreeview.moc"
