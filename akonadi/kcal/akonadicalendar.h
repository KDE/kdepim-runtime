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

    /**
     * @deprecated: FORAKONADI
     */
private:
    bool beginChange( KCal::Incidence *incidence );
public:
    bool beginChangeFORAKONADI( const Akonadi::Item &incidence );

    /**
     * @deprecated: FORAKONADI
     */
private:
    bool endChange( KCal::Incidence *incidence );
public:
    bool endChangeFORAKONADI( const Akonadi::Item &incidence );

    bool reload(); //TODO remove, atm abstract in Calendar
    bool save(); //TODO remove, atm abstract in Calendar
    void close(); //TODO remove, atm abstract in Calendar

    bool addAgent( const KUrl &mUrl );

    /**
     * @deprecated: FORAKONADI
     */
  private:
    bool addIncidence( KCal::Incidence *incidence );
  public:
    bool addIncidenceFORAKONADI( const KCal::Incidence::Ptr &incidence );

    /**
     * @deprecated: FORAKONADI
     */
  private:
    bool deleteIncidence( KCal::Incidence *incidence );
  public:
    bool deleteIncidenceFORAKONADI( const Akonadi::Item &incidence );

    void incidenceUpdated( KCal::IncidenceBase *incidenceBase );

    /**
     * @deprecated: FORAKONADI
     */
  private:
    bool addEvent( KCal::Event *event );
  public:
    bool addEventFORAKONADI( const KCal::Event::Ptr &event );

    /**
     * @deprecated: FORAKONADI
     */
  private:
    bool deleteEvent( KCal::Event *event );
  public:
    bool deleteEventFORAKONADI( const Akonadi::Item &event );

    /**
     * @deprecated: FORAKONADI
     */
    void deleteAllEventsFORAKONADI() { Q_ASSERT(false); } //TODO remove, atm abstract in Calendar

    /**
     * @deprecated: FORAKONADI
     */
  private:
    KCal::Event::List rawEvents( EventSortField sortField = EventSortUnsorted, SortDirection sortDirection = SortDirectionAscending );
  public:
    Akonadi::Item ::List rawEventsFORAKONADI( EventSortField sortField = EventSortUnsorted, SortDirection sortDirection = SortDirectionAscending );

    /**
     * @deprecated: FORAKONADI
     */
  private:
    KCal::Event::List rawEvents( const QDate &start, const QDate &end, const KDateTime::Spec &timeSpec = KDateTime::Spec(), bool inclusive = false );
  public:
    Akonadi::Item ::List rawEventsFORAKONADI( const QDate &start, const QDate &end, const KDateTime::Spec &timeSpec = KDateTime::Spec(), bool inclusive = false );

    /**
     * @deprecated: FORAKONADI
     */
  private:
    KCal::Event::List rawEventsForDate( const QDate &date, const KDateTime::Spec &timeSpec = KDateTime::Spec(), EventSortField sortField = EventSortUnsorted, SortDirection sortDirection = SortDirectionAscending );
  public:
    Akonadi::Item ::List rawEventsForDateFORAKONADI( const QDate &date, const KDateTime::Spec &timeSpec = KDateTime::Spec(), EventSortField sortField = EventSortUnsorted, SortDirection sortDirection = SortDirectionAscending );

    /**
     * @deprecated: FORAKONADI
     */
  private:
    KCal::Event::List rawEventsForDate( const KDateTime &dt );
  public:
    Akonadi::Item::List rawEventsForDateFORAKONADI( const KDateTime &dt );

    /**
     * @deprecated: FORAKONADI
     */
  private:
    KCal::Event *event( const QString &uid );
  public:
    Akonadi::Item eventFORAKONADI( const Akonadi::Item::Id &id );

    /**
     * @deprecated: FORAKONADI
     */
  private:
    bool addTodo( KCal::Todo *todo );
  public:
    bool addTodoFORAKONADI( const KCal::Todo::Ptr &todo );

    /**
     * @deprecated: FORAKONADI
     */
  private:
    bool deleteTodo( KCal::Todo *todo );
  public:
    bool deleteTodoFORAKONADI( const Akonadi::Item &todo );

    /**
     * @deprecated: FORAKONADI
     */
  private:
    void deleteAllTodos() { Q_ASSERT(false); } //TODO remove, atm abstract in Calendar
  public:
    void deleteAllTodosFORAKONADI() { Q_ASSERT(false); } //TODO remove, atm abstract in Calendar

    /**
     * @deprecated: FORAKONADI
     */
  private:
    KCal::Todo::List rawTodos( TodoSortField sortField = TodoSortUnsorted, SortDirection sortDirection = SortDirectionAscending );
  public:
    Akonadi::Item::List rawTodosFORAKONADI( TodoSortField sortField = TodoSortUnsorted, SortDirection sortDirection = SortDirectionAscending );

    /**
     * @deprecated: FORAKONADI
     */
  private:
    KCal::Todo::List rawTodosForDate( const QDate &date );
  public:
    Akonadi::Item::List rawTodosForDateFORAKONADI( const QDate &date );

    /**
     * @deprecated: FORAKONADI
     */
  private:
    KCal::Todo *todo( const QString &uid );
  public:
    Akonadi::Item todoFORAKONADI( const Akonadi::Item::Id &uid );

    /**
     * @deprecated: FORAKONADI
     */
  private:
    bool addJournal( KCal::Journal *journal );
  public:
    bool addJournalFORAKONADI( const KCal::Journal::Ptr &journal );

    /**
     * @deprecated: FORAKONADI
     */
  private:
    bool deleteJournal( KCal::Journal *journal );
  public:
    bool deleteJournalFORAKONADI( const Akonadi::Item &journal );

    /**
     * @deprecated: FORAKONADI
     */
  private:
    void deleteAllJournals() { Q_ASSERT(false); } //TODO remove, atm abstract in Calendar
  public:
    void deleteAllJournalsFORAKONADI() { Q_ASSERT(false); } //TODO remove, atm abstract in Calendar

    /**
     * @deprecated: FORAKONADI
     */
  private:
    KCal::Journal::List rawJournals( JournalSortField sortField = JournalSortUnsorted, SortDirection sortDirection = SortDirectionAscending );
  public:
    Akonadi::Item::List rawJournalsFORAKONADI( JournalSortField sortField = JournalSortUnsorted, SortDirection sortDirection = SortDirectionAscending );

    /**
     * @deprecated: FORAKONADI
     */
  private:
    KCal::Journal::List rawJournalsForDate( const QDate &date );
  public:
    Akonadi::Item ::List rawJournalsForDateFORAKONADI( const QDate &date );

    /**
     * @deprecated: FORAKONADI
     */
  private:
    KCal::Journal *journal( const QString &uid );
  public:
    Akonadi::Item journalFORAKONADI( const Akonadi::Item::Id &id );

    KCal::Alarm::List alarms( const KDateTime &from, const KDateTime &to );
    KCal::Alarm::List alarmsTo( const KDateTime &to );

    using QObject::event;   // prevent warning about hidden virtual method

  public Q_SLOTS:
    /**
     * @deprecated: FORAKONADI
     */
    void deleteIncidenceProxyMethod( KCal::Incidence *incidence ) { deleteIncidence(incidence); }
    void deleteIncidenceProxyMethodFORAKONADI( const Akonadi::Item &incidence ) { deleteIncidenceFORAKONADI(incidence); }

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
