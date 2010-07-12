/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Stephen Kelly <stephen@kdab.com>

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

#include "etmstatesaver.h"

#include <QtCore/QModelIndex>
#include <QtGui/QItemSelection>
#include <QtGui/QTreeView>

#include <akonadi/entitytreemodel.h>

using namespace Akonadi;

ETMStateSaver::ETMStateSaver(QObject* parent)
  : Future::KViewStateSaver(parent)
{
}

QModelIndex ETMStateSaver::indexFromConfigString(const QAbstractItemModel *model, const QString& key) const
{
  if ( key.startsWith( QLatin1Char( 'x' ) ) )
    return QModelIndex();

  Entity::Id id = key.mid( 1 ).toLongLong();
  if ( id < 0 )
    return QModelIndex();

  if ( key.startsWith( QLatin1Char( 'c' ) ) )
  {
    QModelIndex idx = EntityTreeModel::modelIndexForCollection( model, Collection( id ) );
    if ( !idx.isValid() )
    {
      kDebug() << "Collection with id" << id << "is not in model yet";
      return QModelIndex();
    }
    return idx;
  }
  else if ( key.startsWith( QLatin1Char( 'i' ) ) )
  {
    QModelIndexList list = EntityTreeModel::modelIndexesForItem( model, Item( id ) );
    if ( list.isEmpty() )
    {
      kDebug() << "Item with id" << id << "is not in model yet";
      return QModelIndex();
    }
    return list.first();
   }
  return QModelIndex();
}

QString ETMStateSaver::indexToConfigString(const QModelIndex& index) const
{
  if ( !index.isValid() )
    return QLatin1String( "x-1" );
  const Collection c = index.data( EntityTreeModel::CollectionRole ).value<Collection>();
  if ( c.isValid() )
    return QString::fromLatin1( "c%1" ).arg( c.id() );
  Entity::Id id = index.data( EntityTreeModel::ItemIdRole ).value<Entity::Id>();
  if ( id >= 0 )
    return QString::fromLatin1( "i%1" ).arg( id );
  return QString();
}

void ETMStateSaver::selectCollections(const Akonadi::Collection::List& list)
{
  QStringList colStrings;
  foreach(const Collection &col, list)
    colStrings << QString::fromLatin1( "c%1" ).arg( col.id() );
  restoreSelection(colStrings);
}

void ETMStateSaver::selectCollections(const QList< Collection::Id >& list)
{
  QStringList colStrings;
  foreach(const Collection::Id &colId, list)
    colStrings << QString::fromLatin1( "c%1" ).arg( colId );
  restoreSelection(colStrings);
}

void ETMStateSaver::selectItems(const Akonadi::Item::List& list)
{
  QStringList itemStrings;
  foreach(const Item &item, list)
    itemStrings << QString::fromLatin1( "i%1" ).arg( item.id() );
  restoreSelection(itemStrings);
}

void ETMStateSaver::selectItems(const QList< Item::Id >& list)
{
  QStringList itemStrings;
  foreach(const Item::Id &itemId, list)
    itemStrings << QString::fromLatin1( "i%1" ).arg( itemId );
  restoreSelection(itemStrings);
}
