/*
    Copyright (c) 2013 Mark Gaiser <markg85@gmail.com>

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

#ifndef CALENDARDATA_H
#define CALENDARDATA_H

#include <QObject>
#include <QFlags>
#include <QDate>

namespace Akonadi {
class EntityMimeTypeFilterModel;
class ETMCalendar;
}

class DateTimeRangeFilterModel;
class QAbstractItemModel;

class CalendarData : public QObject
{
    Q_OBJECT

    /**
     * This property is used to determine which data from which month to show. One should
     * set it's Year, Month and Day although the Day is of less importance. Set it using
     * YYYY-MM-DD format.
     * 
     * This component (CalendarData) works by providing a startDate and endDate. It will
     * then fetch the data between the given range.
     */
    Q_PROPERTY(QDate startDate READ startDate WRITE setStartDate NOTIFY startDateChanged)
    
    /**
     * This property is used to determine which data from which month to show. One should
     * set it's Year, Month and Day although the Day is of less importance. Set it using
     * YYYY-MM-DD format.
     * 
     * This component (CalendarData) works by providing a startDate and endDate. It will
     * then fetch the data between the given range.
     */
    Q_PROPERTY(QDate endDate READ endDate WRITE setEndDate NOTIFY endDateChanged)
    
    /**
     * This determines which kind of data types should be contained in
     * selectedDayModel and upcomingEventsModel. By default all types are included.
     * NOTE: Only the Event type is fully implemented.
     * TODO: Fully implement the other data types throughout this code.
     */
    Q_PROPERTY(int types READ types WRITE setTypes NOTIFY typesChanged)
    
    /**
     * The number of days a week contains.
     * TODO: perhaps this one can just be removed. A week is 7 days by definition.
     * However, i don't know if that's also the case in other more exotic calendars.
     */
    Q_PROPERTY(int sorting READ sorting WRITE setSorting NOTIFY sortingChanged)
    
    /**
     * If an error occured, it will be set in this string as human readable text.
     */
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    
    /**
     * Indicated if the model is loading.
     */
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    
    /**
     * This model contains the data from startDate till endDate.
     * Each entry contains metadata about the current day. The exact metadata can 
     * be found in "daysmodel.cpp" where the exact names usable in QML are being set.
     */
    Q_PROPERTY(QAbstractItemModel* model READ model CONSTANT)

    /**
     * The Type and Sort enums are registered in Qt so that they can be used from within QML.
     */
    Q_ENUMS(Type)
    Q_ENUMS(Sort)

public:
    enum Type {
        Holiday = 1,
        Event = 2,
        Todo = 4,
        Journal = 8
    };
    Q_DECLARE_FLAGS(Types, Type)

    enum Sort {
        Ascending = 1,
        Descending = 2,
        None = 4
    };
    Q_DECLARE_FLAGS(Sorting, Sort)

    explicit CalendarData(QObject *parent = 0);
    QDate startDate() const;
    void setStartDate(const QDate &dateTime);
    QDate endDate() const;
    void setEndDate(const QDate &dateTime);
    QAbstractItemModel* model() const;

    // Sorting
    int sorting() const;
    void setSorting(int sorting);

    // Overlapping events
    bool showOverlapping();
    void setShowOverlapping(bool overlapping);

signals:
    void startDateChanged();
    void endDateChanged();
    void typesChanged();
    void errorMessageChanged();
    void loadingChanged();
    void sortingChanged();

private:
    int types() const;
    void setTypes(int types);
    QString errorMessage() const;
    bool loading() const;

    void updateTypes();

    QDate m_startDate;
    QDate m_endDate;
    Types m_types;
    Sorting m_sorting;

    Akonadi::ETMCalendar *m_etmCalendar;
    Akonadi::EntityMimeTypeFilterModel *m_itemList;
    DateTimeRangeFilterModel *m_filteredList;

    bool m_overlapping;
};

#endif // CALENDARDATA_H
