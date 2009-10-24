/*
    This file is part of Akonadi.

    Copyright (c) 2009 KDAB
    Author: Sebastian Sauer <sebsauer@kdab.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#ifndef AKONADI_KCAL_AKONADICALENDAR_H
#define AKONADI_KCAL_AKONADICALENDAR_H

#include "akonadi-kcal_export.h"
#include "calendarbase.h"

class QAbstractItemModel;

namespace Akonadi {
  class Collection;
  class Item;
}

namespace KCal {
    class CalFormat;
}

namespace KOrg {

/**
 * Implements a KCal::Calendar that uses Akonadi as backend.
 */
class AKONADI_KCAL_EXPORT AkonadiCalendar : public CalendarBase
{
    Q_OBJECT
  public:
    explicit AkonadiCalendar( QAbstractItemModel *model, const KDateTime::Spec &timeSpec );
    ~AkonadiCalendar();

    bool addAgent( const KUrl &mUrl );

    void incidenceUpdated( KCal::IncidenceBase *incidenceBase );

    Akonadi::Item ::List rawEvents( EventSortField sortField = EventSortUnsorted, SortDirection sortDirection = SortDirectionAscending );
    Akonadi::Item ::List rawEvents( const QDate &start, const QDate &end, const KDateTime::Spec &timeSpec = KDateTime::Spec(), bool inclusive = false );
    Akonadi::Item ::List rawEventsForDate( const QDate &date, const KDateTime::Spec &timeSpec = KDateTime::Spec(), EventSortField sortField = EventSortUnsorted, SortDirection sortDirection = SortDirectionAscending );
    Akonadi::Item::List rawEventsForDate( const KDateTime &dt );

    Akonadi::Item event( const Akonadi::Item::Id &id );

    Akonadi::Item::List rawTodos( TodoSortField sortField = TodoSortUnsorted, SortDirection sortDirection = SortDirectionAscending );
    Akonadi::Item::List rawTodosForDate( const QDate &date );

    Akonadi::Item todo( const Akonadi::Item::Id &uid );

    Akonadi::Item::List rawJournals( JournalSortField sortField = JournalSortUnsorted, SortDirection sortDirection = SortDirectionAscending );
    Akonadi::Item::List rawJournalsForDate( const QDate &date );

    Akonadi::Item journal( const Akonadi::Item::Id &id );

    Akonadi::Item::List alarms( const KDateTime &from, const KDateTime &to );
    Akonadi::Item::List alarmsTo( const KDateTime &to );

    /* reimp */ Akonadi::Item findParent( const Akonadi::Item& item ) const;
    /* reimp */ Akonadi::Item::List findChildren( const Akonadi::Item &item ) const;
    /* reimp */ bool isChild( const Akonadi::Item& parent, const Akonadi::Item &child ) const;

    Akonadi::Item::Id itemIdForIncidenceUid(const QString &uid) const;
 
    using QObject::event;   // prevent warning about hidden virtual method

  Q_SIGNALS:
    void signalErrorMessage( const QString& );

  private:
    Q_DISABLE_COPY( AkonadiCalendar )
    class Private;
    Private *const d;
};

}

#endif
