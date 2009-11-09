/*
    Copyright (c) 2008 Bruno Virlet <bvirlet@kdemail.net>
                  2009 KDAB; Author: Frank Osterfeld <osterfeld@kde.org>

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

#include "calendarmodel.h"
#include "utils.h"

#include <Akonadi/Item>
#include <Akonadi/Collection>

#include <KCal/Incidence>
#include <KCal/Event>

#include <KIconLoader>
#include <KLocale>

#include <QtGui/QPixmap>

using namespace Akonadi;
using namespace KCal;

class CalendarModel::Private
{
  public:
    explicit Private( CalendarModel *qq )
    :q( qq )
    {
    }

  private:
    CalendarModel * const q;
};

CalendarModel::CalendarModel( Session *session, ChangeRecorder* monitor, QObject *parent )
  : EntityTreeModel( session, monitor, parent ),
    d( new Private( this ) )
{
}

CalendarModel::~CalendarModel()
{
  delete d;
}

QVariant CalendarModel::entityData( const Item& item, int column, int role ) const {
  const Incidence::Ptr incidence = Akonadi::incidence( item );
  if ( !incidence )
    return QVariant();

  switch( role ) {
  case Qt::DecorationRole:
    if ( column != Summary )
      return QVariant();
    if ( incidence->type() == "Todo" )
      return SmallIcon( QLatin1String( "view-pim-tasks" ) );
    if ( incidence->type() == "Journal" )
      return SmallIcon( QLatin1String( "view-pim-journal" ) );
    if ( incidence->type() == "Event" )
      return SmallIcon( QLatin1String( "view-calendar" ) );
    return SmallIcon( QLatin1String( "network-wired" ) );
  case Qt::DisplayRole:
    switch( column ) {
    case Summary:
      return incidence->summary();
    case DateTimeStart:
      return incidence->dtStart().toString();
    case DateTimeEnd:
      return incidence->dtEnd().toString();
    case DateTimeDue:
      if ( Todo::ConstPtr todo = Akonadi::todo( item ) )
        return todo->dtDue().toString();
      else
        return QVariant();
    case Priority:
      if ( Todo::ConstPtr todo = Akonadi::todo( item ) )
        return todo->priority();
      else
        return QVariant();
    case PercentComplete:
      if ( Todo::ConstPtr todo = Akonadi::todo( item ) )
        return todo->percentComplete();
      else
        return QVariant();
    case Type:
      return incidence->type();
    default:
      break;
    }
    case SortRole:
    switch( column ) {
    case Summary:
      return incidence->summary();
    case DateTimeStart:
      return incidence->dtStart().toTime_t();
    case DateTimeEnd:
      return incidence->dtEnd().toTime_t();
    case DateTimeDue:
      if ( Todo::ConstPtr todo = Akonadi::todo( item ) )
        return todo->dtDue().toTime_t();
      else
        return QVariant();
    case Priority:
      if ( Todo::ConstPtr todo = Akonadi::todo( item ) )
        return todo->priority();
      else
        return QVariant();
    case PercentComplete:
      if ( Todo::ConstPtr todo = Akonadi::todo( item ) )
        return todo->percentComplete();
      else
        return QVariant();
    case Type:
      return incidence->type();
    default:
      break;
    }
    return QVariant();
  default:
    return QVariant();
  }

  return QVariant();
}

QVariant CalendarModel::entityData( const Akonadi::Collection &collection, int column, int role ) const {
  return EntityTreeModel::entityData( collection, column, role );
}

int CalendarModel::entityColumnCount( EntityTreeModel::HeaderGroup headerSet ) const {
  if ( headerSet == EntityTreeModel::ItemListHeaders )
    return ItemColumnCount;
  else
    return CollectionColumnCount;
}

QVariant CalendarModel::entityHeaderData( int section, Qt::Orientation orientation, int role, EntityTreeModel::HeaderGroup headerSet ) const {
  if ( role != Qt::DisplayRole || orientation != Qt::Horizontal )
    return QVariant();
  if ( headerSet == EntityTreeModel::ItemListHeaders ) {
    switch( section ) {
    case Summary:
      return i18nc( "@title:column, calendar event summary", "Summary" );
    case DateTimeStart:
      return i18nc( "@title:column, calendar event start date and time", "Start Date and Time" );
    case DateTimeEnd:
      return i18nc( "@title:column, calendar event end date and time", "End Date and Time" );
    case Type:
      return i18nc( "@title:column, calendar event type", "Type" );
    case DateTimeDue:
      return i18nc( "@title:column, todo item due date and time", "Due Date and Time" );
    case Priority:
      return i18nc( "@title:column, todo item priority", "Priority" );
    case PercentComplete:
      return i18nc( "@title:column, todo item completion in percent", "Complete" );
    default:
      return QVariant();
    }
  }
  if ( headerSet == EntityTreeModel::CollectionTreeHeaders ) {
    switch ( section ) {
      case CollectionTitle:
        return i18nc( "@title:column, calendar title", "Calendar" );
      default:
        return QVariant();
    }
  }
  return QVariant();
}

