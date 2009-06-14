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

FilterCollectionModel::FilterCollectionModel( QObject * parent, Filter * filter )
  : Akonadi::CollectionModel( parent )
{
}

FilterCollectionModel::~FilterCollectionModel()
{
}

QVariant FilterCollectionModel::data( const QModelIndex &index, int role ) const
{
  if( role == Qt::CheckStateRole )
    return QVariant( Qt::Checked );
  return Akonadi::CollectionModel::data( index, role );
}

Qt::ItemFlags FilterCollectionModel::flags( const QModelIndex &index ) const
{
  Qt::ItemFlags flags = Akonadi::CollectionModel::flags( index );
  return flags | Qt::ItemIsUserCheckable;
}


