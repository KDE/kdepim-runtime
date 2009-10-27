/*
  This file is part of Akonadi.

    Copyright (c) 2009 KDAB
    Author: Frank Osterfeld <frank@kdab.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#include "collectionselection.h"
#include "utils.h"

#include <QItemSelectionModel>

using namespace Akonadi;

class Akonadi::CollectionSelection::Private
{
public:
  explicit Private( QItemSelectionModel* model_ ) : model( model_ ) {

  }

  QItemSelectionModel* model;
};

CollectionSelection::CollectionSelection( QItemSelectionModel *selectionModel ) : QObject( selectionModel ), d( new Private ( selectionModel ) )
{
  connect( selectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(slotSelectionChanged(QItemSelection,QItemSelection)) );
}

CollectionSelection::~CollectionSelection()
{
  delete d;
}

QItemSelectionModel* CollectionSelection::model() const
{
  return d->model;
}

bool CollectionSelection::hasSelection() const
{
  return d->model->hasSelection();
}

Collection::List CollectionSelection::selectedCollections() const
{
  Collection::List selected;
  Q_FOREACH ( const QModelIndex &idx, d->model->selectedIndexes() )
    selected.append( collectionFromIndex( idx ) );
  return selected;
}

QList<Collection::Id> CollectionSelection::selectedCollectionIds() const {
  QList<Collection::Id> selected;
  Q_FOREACH ( const QModelIndex &idx, d->model->selectedIndexes() )
    selected.append( collectionIdFromIndex( idx ) );
  return selected;
}

void CollectionSelection::slotSelectionChanged( const QItemSelection &selectedIndexes, const QItemSelection &deselectedIndexes )
{
  const Collection::List selected = collectionsFromIndexes( selectedIndexes.indexes() );
  const Collection::List deselected = collectionsFromIndexes( deselectedIndexes.indexes() );

  emit selectionChanged( selected, deselected );
  Q_FOREACH ( const Collection &c, deselected )
    emit collectionDeselected( c );
  Q_FOREACH ( const Collection &c, selected )
    emit collectionSelected( c );
}
