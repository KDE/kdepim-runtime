/*
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>


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

#include "favoritecollectionsmodel.h"

#include <QtGui/QItemSelectionModel>

#include "entitytreemodel.h"

using namespace Akonadi;

/**
 * @internal
 */
class FavoriteCollectionsModel::Private
{
  public:
    Private( FavoriteCollectionsModel *parent )
      : q( parent )
    {
    }

    void clearAndUpdateSelection()
    {
      q->selectionModel()->clear();
      updateSelection();
    }

    void updateSelection()
    {
      EntityTreeModel *model = qobject_cast<EntityTreeModel*>( q->sourceModel() );
      Q_ASSERT( model!=0 );

      foreach (const Collection &c, collections) {
        QModelIndex index = model->indexForCollection( model->collectionForId( c.id() ) );
        q->selectionModel()->select( index,
                                     QItemSelectionModel::Select );
      }
    }

    FavoriteCollectionsModel * const q;

    Collection::List collections;
};

FavoriteCollectionsModel::FavoriteCollectionsModel( EntityTreeModel *source, QObject *parent )
  : SelectionProxyModel( new QItemSelectionModel( source, parent ), parent ),
    d( new Private( this ) )
{
  setSourceModel( source );
  setOmitChildren( true );
  setIncludeAllSelected( true );

  connect( source, SIGNAL( modelReset() ), this, SLOT( clearAndUpdateSelection() ) );
  connect( source, SIGNAL( layoutChanged() ), this, SLOT( clearAndUpdateSelection() ) );
  connect( source, SIGNAL( rowsInserted(QModelIndex, int, int) ), this, SLOT( updateSelection() ) );
}

FavoriteCollectionsModel::~FavoriteCollectionsModel()
{
  delete d;
}

void Akonadi::FavoriteCollectionsModel::setCollections( const Collection::List &collections )
{
  d->collections = collections;
  d->clearAndUpdateSelection();
}

Collection::List Akonadi::FavoriteCollectionsModel::collections() const
{
  return d->collections;
}

QVariant Akonadi::FavoriteCollectionsModel::headerData( int section, Qt::Orientation orientation, int role) const
{
  if ( section == 0
    && orientation == Qt::Horizontal
    && role == Qt::DisplayRole ) {
    return "Favorite Folders";
  } else {
    return SelectionProxyModel::headerData( section, orientation, role );
  }
}

#include "favoritecollectionsmodel.moc"

