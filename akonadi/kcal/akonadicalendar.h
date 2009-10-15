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
    explicit AkonadiCalendar( const KDateTime::Spec &timeSpec );
    ~AkonadiCalendar();

    bool hasCollection( const Akonadi::Collection &collection ) const;
    void addCollection( const Akonadi::Collection &collection );
    void removeCollection( const Akonadi::Collection &collection );

    bool beginChange( const Akonadi::Item &incidence );

    bool endChange( const Akonadi::Item &incidence );

    bool reload(); //TODO remove, atm abstract in Calendar
    bool save(); //TODO remove, atm abstract in Calendar
    void close(); //TODO remove, atm abstract in Calendar

    bool addAgent( const KUrl &mUrl );

    bool addIncidence( const KCal::Incidence::Ptr &incidence );

    bool deleteIncidence( const Akonadi::Item &incidence );

    void incidenceUpdated( KCal::IncidenceBase *incidenceBase );

    bool addEvent( const KCal::Event::Ptr &event );

    bool deleteEvent( const Akonadi::Item &event );

    void deleteAllEvents() { Q_ASSERT(false); } //TODO remove, atm abstract in Calendar

    Akonadi::Item ::List rawEvents( EventSortField sortField = EventSortUnsorted, SortDirection sortDirection = SortDirectionAscending );

    Akonadi::Item ::List rawEvents( const QDate &start, const QDate &end, const KDateTime::Spec &timeSpec = KDateTime::Spec(), bool inclusive = false );

    Akonadi::Item ::List rawEventsForDate( const QDate &date, const KDateTime::Spec &timeSpec = KDateTime::Spec(), EventSortField sortField = EventSortUnsorted, SortDirection sortDirection = SortDirectionAscending );

    Akonadi::Item::List rawEventsForDate( const KDateTime &dt );

    Akonadi::Item event( const Akonadi::Item::Id &id );

    bool addTodo( const KCal::Todo::Ptr &todo );

    bool deleteTodo( const Akonadi::Item &todo );

    void deleteAllTodos() { Q_ASSERT(false); } //TODO remove, atm abstract in Calendar

    Akonadi::Item::List rawTodos( TodoSortField sortField = TodoSortUnsorted, SortDirection sortDirection = SortDirectionAscending );

    Akonadi::Item::List rawTodosForDate( const QDate &date );

    Akonadi::Item todo( const Akonadi::Item::Id &uid );

    bool addJournal( const KCal::Journal::Ptr &journal );

    bool deleteJournal( const Akonadi::Item &journal );

    void deleteAllJournals() { Q_ASSERT(false); } //TODO remove, atm abstract in Calendar

    Akonadi::Item::List rawJournals( JournalSortField sortField = JournalSortUnsorted, SortDirection sortDirection = SortDirectionAscending );

    Akonadi::Item ::List rawJournalsForDate( const QDate &date );

    Akonadi::Item journal( const Akonadi::Item::Id &id );

    KCal::Alarm::List alarms( const KDateTime &from, const KDateTime &to );
    KCal::Alarm::List alarmsTo( const KDateTime &to );

    /* reimp */ Akonadi::Item findParent( const Akonadi::Item& item ) const;
    /* reimp */ Akonadi::Item::List findChildren( const Akonadi::Item &item ) const;

    using QObject::event;   // prevent warning about hidden virtual method

  public Q_SLOTS:
    /**
     * @deprecated: 
     */
    void deleteIncidenceProxyMethod( const Akonadi::Item &incidence ) { deleteIncidence(incidence); }

  Q_SIGNALS:
    void signalErrorMessage( const QString& );
    // Same signals Akonadi::Monitor provides to allow later to refactor code to
    // use Collection+Monitor+etc direct rather then the AkonadiCalendar class.
    /*
    void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers );
    void itemMoved( const Akonadi::Item &item, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination );
    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void itemRemoved( const Akonadi::Item &item );
    void itemLinked( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void itemUnlinked( const Akonadi::Item &item, const Akonadi::Collection &collection );
    void collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent );
    void collectionChanged( const Akonadi::Collection &collection );
    void collectionRemoved( const Akonadi::Collection &collection );
    void collectionStatisticsChanged( Akonadi::Collection::Id id, const Akonadi::CollectionStatistics &statistics );
    void collectionMonitored( const Akonadi::Collection &collection, bool monitored );
    void itemMonitored( const Akonadi::Item &item, bool monitored );
    void resourceMonitored( const QByteArray &identifier, bool monitored );
    void mimeTypeMonitored( const QString &mimeType, bool monitored );
    void allMonitored( bool monitored );
    */

  private:
    Q_DISABLE_COPY( AkonadiCalendar )
    class Private;
    Private *const d;
};

}

#endif
