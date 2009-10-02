/*
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

#include "dragdropmanager_p.h"

#include <QtGui/QDropEvent>

#include <KDE/KUrl>

#include "akonadi/collection.h"
#include "akonadi/entitytreemodel.h"

using namespace Akonadi;

DragDropManager::DragDropManager( QAbstractItemView *view )
    : m_view( view )
{

}

Collection DragDropManager::currentDropTarget(QDropEvent* event) const
{
  const QModelIndex index = m_view->indexAt( event->pos() );
  Collection col = m_view->model()->data( index, EntityTreeModel::CollectionRole ).value<Collection>();
  if ( !col.isValid() ) {
    const Item item = m_view->model()->data( index, EntityTreeModel::ItemRole ).value<Item>();
    if ( item.isValid() )
      col = m_view->model()->data( index.parent(), EntityTreeModel::CollectionRole ).value<Collection>();
  }
  return col;
}

bool DragDropManager::dropAllowed( QDragMoveEvent * event )
{
  // Check if the collection under the cursor accepts this data type
  const Collection col = currentDropTarget( event );
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
        if ( hasAncestor( m_view->indexAt( event->pos() ), collection.id() ) )
          break;
      } else { // This is an item.
        QString type = url.queryItems()[ QString::fromLatin1( "type" )];
        if ( !supportedContentTypes.contains( type ) )
          break;
      }
      return true;
    }
  }
  return false;
}

bool DragDropManager::hasAncestor( const QModelIndex& idx, Collection::Id parentId )
{
  QModelIndex idx2 = idx;
  while ( idx2.isValid() ) {
    if ( m_view->model()->data( idx2, EntityTreeModel::CollectionIdRole ).toLongLong() == parentId )
      return true;

    idx2 = idx2.parent();
  }
  return false;
}

