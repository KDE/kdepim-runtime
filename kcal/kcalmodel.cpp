/*
    Copyright (c) 2008 Bruno Virlet <bvirlet@kdemail.net>

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

#include "kcalmodel.h"

#include <libakonadi/item.h>
#include <libakonadi/itemfetchjob.h>

#include <kcal/incidence.h>
#include <kcal/event.h>

#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

class KCalModel::Private
{
  public:
};

KCalModel::KCalModel( QObject *parent )
  : Akonadi::ItemModel( parent ),
    d( new Private() )
{
  addFetchPart( Akonadi::Item::PartBody );
}

KCalModel::~KCalModel()
{
  delete d;
}

int KCalModel::columnCount( const QModelIndex& ) const
{
  return 4;
}

QVariant KCalModel::data( const QModelIndex &index,  int role ) const
{
  if ( role == Akonadi::ItemModel::IdRole )
    return Akonadi::ItemModel::data( index, role );

  if ( role != Qt::DisplayRole || !index.isValid() || index.row() >= rowCount() )
    return QVariant();

  const Akonadi::Item item = itemForIndex( index );

  if ( !item.hasPayload<IncidencePtr>() )
    return QVariant();

  const IncidencePtr incidence = item.payload<IncidencePtr>();

  switch( index.column() ) {
    case 0:
      return incidence->summary();
      break;
    case 1:
      return incidence->dtStartStr();
      break;
    case 2:
      if ( incidence->type() == "Event" ) {
        KCal::Event* event = static_cast<KCal::Event*>( incidence.get() );
        return event->dtEndStr();
      } else {
        return QVariant();
      }
      break;
    case 3:
      return incidence->type();
      break;
    default:
      break;
  }

  return QVariant();
}

QVariant KCalModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( role != Qt::DisplayRole || orientation != Qt::Horizontal )
    return QVariant();

  return QVariant();
}
