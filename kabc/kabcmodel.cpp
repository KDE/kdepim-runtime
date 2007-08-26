/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#include "kabcmodel.h"

#include <kabc/addressee.h>
#include <libakonadi/item.h>
#include <libakonadi/itemfetchjob.h>

KABCModel::KABCModel( QObject *parent )
  : Akonadi::ItemModel( parent )
{
  addFetchPart( Akonadi::Item::PartBody );
}

int KABCModel::columnCount( const QModelIndex& ) const
{
  return 3;
}

QVariant KABCModel::data( const QModelIndex &index, int role ) const
{
  if ( role != Qt::DisplayRole )
    return QVariant();

  if ( !index.isValid() )
    return QVariant();

  if ( index.row() >= rowCount() )
    return QVariant();

  const Akonadi::Item item = itemForIndex( index );

  if ( !item.hasPayload<KABC::Addressee>() )
    return QVariant();

  const KABC::Addressee addr = item.payload<KABC::Addressee>();
  switch ( index.column() ) {
    case 0:
      return addr.givenName();
      break;
    case 1:
      return addr.familyName();
      break;
    case 2:
      return addr.preferredEmail();
      break;
    default:
      break;
  }

  return QVariant();
}

QVariant KABCModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( role != Qt::DisplayRole )
    return QVariant();

  if ( orientation != Qt::Horizontal )
    return QVariant();

  switch ( section ) {
    case 0:
      return KABC::Addressee::givenNameLabel();
      break;
    case 1:
      return KABC::Addressee::familyNameLabel();
      break;
    case 2:
      return KABC::Addressee::emailLabel();
      break;
    default:
      break;
  }

  return QVariant();
}
