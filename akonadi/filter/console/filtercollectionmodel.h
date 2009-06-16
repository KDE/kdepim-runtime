/******************************************************************************
 *
 *  File : filtercollectionmodel.h
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

#ifndef _FILTERCOLLECTIONMODEL_H_
#define _FILTERCOLLECTIONMODEL_H_

#include <akonadi/collection.h>
#include <akonadi/collectionmodel.h>


class Filter;

class FilterCollectionModel : public Akonadi::CollectionModel
{
public:
  FilterCollectionModel( QObject * parent, Filter * filter );
  virtual ~FilterCollectionModel();
protected:
  Filter * mFilter;
protected:
  virtual Qt::ItemFlags flags( const QModelIndex &index ) const;
  virtual QVariant data( const QModelIndex &index, int role ) const;
  virtual bool setData( const QModelIndex &index, const QVariant & value, int role );
};

#endif //!_FILTERCOLLECTIONMODEL_H_
