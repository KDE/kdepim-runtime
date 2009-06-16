/******************************************************************************
 *
 *  File : filtercollectionmodel.cpp
 *  Created on Sun 14 Jun 2009 23:21:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Filter Console Application
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "filtercollectionmodel.h"
#include "filter.h"

#include <QStringList>

FilterCollectionModel::FilterCollectionModel( QObject * parent, Filter * filter )
  : Akonadi::CollectionModel( parent ), mFilter( filter )
{
}

FilterCollectionModel::~FilterCollectionModel()
{
}

QVariant FilterCollectionModel::data( const QModelIndex &index, int role ) const
{
  if( role == Qt::CheckStateRole )
  {
    QVariant d = Akonadi::CollectionModel::data( index, Akonadi::CollectionModel::CollectionIdRole );
    if( !d.isValid() )
      return QVariant( Qt::Unchecked );
    bool ok;
    qint64 id = d.toLongLong( &ok );
    if( !ok )
      return QVariant( Qt::Unchecked );
    return mFilter->hasCollection( (Akonadi::Collection::Id)id ) ? Qt::Checked : Qt::Unchecked;
  }

  return Akonadi::CollectionModel::data( index, role );
}

// this is cloned from CollectionUtils, which is private in akonadi (TODO: make it public ?)
inline bool isFolder( const Akonadi::Collection &collection )
{
  return (collection.parent() != Akonadi::Collection::root().id() &&
          collection.resource() != QLatin1String( "akonadi_search_resource" ) &&
          !collection.contentMimeTypes().isEmpty());
}

Qt::ItemFlags FilterCollectionModel::flags( const QModelIndex &index ) const
{
  Qt::ItemFlags flags = Akonadi::CollectionModel::flags( index );

  QVariant d = Akonadi::CollectionModel::data( index, Akonadi::CollectionModel::CollectionRole );
  if( !d.isValid() )
    return flags;

  if( !d.canConvert< Akonadi::Collection >() )
    return flags;

  Akonadi::Collection coll = d.value< Akonadi::Collection >();
  if( !coll.isValid() )
    return flags;

  if( isFolder( coll ) )
    return flags | Qt::ItemIsUserCheckable;
  return flags & ( ~Qt::ItemIsEnabled );
}

bool FilterCollectionModel::setData( const QModelIndex &index, const QVariant & value, int role )
{
  if( role == Qt::CheckStateRole )
  {
    if( !value.isValid() )
      return Akonadi::CollectionModel::setData( index, value, role );
    int val;
    bool ok;
    val = value.toInt( &ok );
    if( !ok )
      return Akonadi::CollectionModel::setData( index, value, role );

    QVariant d = Akonadi::CollectionModel::data( index, Akonadi::CollectionModel::CollectionIdRole );
    if( !d.isValid() )
      return Akonadi::CollectionModel::setData( index, value, role );
    qint64 id = d.toLongLong( &ok );
    if( !ok )
      return Akonadi::CollectionModel::setData( index, value, role );

    if( val == Qt::Checked )
    {
      if( !mFilter->hasCollection( (Akonadi::Collection::Id)id ) )
        mFilter->addCollection( new Akonadi::Collection( id ) );
    } else {
      if( mFilter->hasCollection( (Akonadi::Collection::Id)id ) )
        mFilter->removeCollection( (Akonadi::Collection::Id)id );
    }

    emit dataChanged( index, index );
  }

  return Akonadi::CollectionModel::setData( index, value, role );
}



