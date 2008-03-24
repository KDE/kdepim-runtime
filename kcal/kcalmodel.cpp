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

#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>

#include <kcal/incidence.h>
#include <kcal/event.h>

#include <klocale.h>
#include <boost/shared_ptr.hpp>

#include <kiconloader.h>
#include <QtGui/QPixmap>

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

using namespace Akonadi;

class KCalModel::Private
{
  public:
};

KCalModel::KCalModel( QObject *parent )
  : ItemModel( parent ),
    d( new Private() )
{
  addFetchPart( Item::PartBody );
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
  if ( role == ItemModel::IdRole )
    return ItemModel::data( index, role );

  if ( !index.isValid() || index.row() >= rowCount() )
    return QVariant();

  const Item item = itemForIndex( index );

  if ( !item.hasPayload<IncidencePtr>() )
    return QVariant();

  const IncidencePtr incidence = item.payload<IncidencePtr>();

  // Icon for the model entry
  switch( role ) {
  case Qt::DecorationRole:
    if ( index.column() == 0 ) {
      if ( incidence->type() == "Todo" ) {
        return SmallIcon( QLatin1String( "view-pim-tasks" ) );
      } else if ( incidence->type() == "Journal" ) {
        return SmallIcon( QLatin1String( "view-pim-journal" ) );
      } else if ( incidence->type() == "Event" ) {
        return SmallIcon( QLatin1String( "view-pim-calendar" ) );
      } else {
        return SmallIcon( QLatin1String( "network-wired" ) );
      }
    }
    break;
  case Qt::DisplayRole:
    switch( index.column() ) {
    case Summary:
      return incidence->summary();
      break;
    case DateTimeStart:
      return incidence->dtStart().toString();
      break;
    case DateTimeEnd:
      return incidence->dtEnd().toString();
      break;
    case Type:
      return incidence->type();
      break;
    default:
      break;
    }
    break;
  default:
    return QVariant();
    break;
  }

  return QVariant();
}

QVariant KCalModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( role == Qt::DisplayRole && orientation == Qt::Horizontal ) {
    switch( section ) {
    case Summary:
      return i18nc( "@title:column, calendar event summary", "Summary" );
    case DateTimeStart:
      return i18nc( "@title:column, calendar event start date and time", "Start date and time" );
    case DateTimeEnd:
      return i18nc( "@title:column, calendar event end date and time", "End date and time" );
    case Type:
      return i18nc( "@title:column, calendar event type", "Type" );
    default:
      return QString();
    }
  }

  return ItemModel::headerData( section, orientation, role );
}
