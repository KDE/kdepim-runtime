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

#ifndef CALENDAR_H
#define CALENDAR_H

#include <QObject>
#include <QDate>
#include <QAbstractListModel>
#include <QAbstractItemModel>
#include <kcalendarsystem.h>

#include "calendardata.h"
#include "daydata.h"
#include "daysmodel.h"
#include "calendardayhelper.h"
#include "incidencemodifier.h"

class Calendar : public QObject
{
    Q_OBJECT
    
    /**
     * This property is used to determine which data from which month to show. One should
     * set it's Year, Month and Day although the Day is of less importance. Set it using
     * YYYY-MM-DD format.
     */
    Q_PROPERTY(QDate startDate READ startDate WRITE setStartDate NOTIFY startDateChanged)
    
    /**
     * This determines which kind of data types should be contained in
     * selectedDayModel and upcomingEventsModel. By default all types are included.
     * NOTE: Only the Event type is fully implemented.
     * TODO: Fully implement the other data types throughout this code.
     */
    Q_PROPERTY(int types READ types WRITE setTypes NOTIFY typesChanged)
    
    /**
     * This determines how entries in selectedDayModel and upcomingEventsModel are sorted.
     * By default we don't sort and just rely on how Akonadi returns our data.
     */
    Q_PROPERTY(int sorting READ sorting WRITE setSorting NOTIFY sortingChanged)
    
    /**
     * The number of days a week contains.
     * TODO: perhaps this one can just be removed. A week is 7 days by definition.
     * However, i don't know if that's also the case in other more exotic calendars.
     */
    Q_PROPERTY(int days READ days WRITE setDays NOTIFY daysChanged)
    
    /**
     * The number of weeks that the model property should contain.
     */
    Q_PROPERTY(int weeks READ weeks WRITE setWeeks NOTIFY weeksChanged)
    
    /**
     * The start day of a week. By default this is Monday (Qt::Monday). It can be
     * changed. One then needs to use the numbers in the Qt DayOfWeek enum:
     * 
     *    Monday = 1
     *    Tuesday = 2
     *    Wednesday = 3
     *    Thursday = 4
     *    Friday = 5
     *    Saturday = 6
     *    Sunday = 7
     * 
     * This value doesn't do anything to other data structures, but helps you 
     * visualizing the data.
     */
    Q_PROPERTY(int startDay READ startDay WRITE setStartDay NOTIFY startDayChanged)
    
    /**
     * The full year in a numeric value. For example 2013, not 13.
     */
    Q_PROPERTY(int year READ year CONSTANT)
    
    /**
     * If an error occured, it will be set in this string as human readable text.
     */
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    
    /**
     * This is the human readable long month name. So not "Feb" but "February".
     * TODO: this should either be done in QML using javascript or by making a
     * 	     function available because this is limiting. There are places
     *       where you would want the short month name.
     */
    Q_PROPERTY(QString monthName READ monthName CONSTANT)
    
    /**
     * This model contains the actual grid data of days. For example, if you had set:
     * - days = 7 (7 days in one week)
     * - weeks = 6 (6 weeks in one month view)
     * then this model will contain 42 entries (days * weeks). Each entry contains
     * metadata about the current day. The exact metadata can be found in "daysmodel.cpp"
     * where the exact names usable in QML are being set.
     */
    Q_PROPERTY(QAbstractListModel* model READ model CONSTANT)
    
    /**
     * This model contains the week numbers for the given date grid.
     */
    Q_PROPERTY(QList<int> weeksModel READ weeksModel NOTIFY startDateChanged CONSTANT)
    
    /**
     * This model contains the events that occur on this day. This also includes events that
     * might not be occuring today, but overlap it.
     */
    Q_PROPERTY(QAbstractItemModel* selectedDayModel READ selectedDayModel CONSTANT)
    
    /**
     * This model contains the events that are upcomming from the date you selected.
     * By default the selected date is your current date. This model will then contain
     * the dates from the selected date + 1 month. Not changable for the moment.
     */
    Q_PROPERTY(QAbstractItemModel* upcomingEventsModel READ upcomingEventsModel CONSTANT)

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


    explicit Calendar(QObject *parent = 0);

    // Start date
    QDate startDate() const;
    void setStartDate(const QDate &dateTime);

    // Types
    int types() const;
    void setTypes(int types);

    // Sorting
    int sorting() const;
    void setSorting(int sorting);

    // Days
    int days();
    void setDays(int days);

    // Weeks
    int weeks();
    void setWeeks(int weeks);

    // Start day
    int startDay();
    void setStartDay(int day);

    // Error message
    QString errorMessage() const;

    // Month name
    QString monthName() const;
    int year() const;
 
    // Model containing all events for the given date range
    QAbstractListModel* model() const;

    // Model filter that only gived the events for a selected day
    QAbstractItemModel* selectedDayModel() const;

    // Model filter that only gived the events for a selected day
    QAbstractItemModel* upcomingEventsModel() const;

    QList<int> weeksModel() const;


    // QML invokables
    /**
     * Convenience function for QML to change the current data to the next month.
     */
    Q_INVOKABLE void nextMonth();
    
    /**
     * Convenience function for QML to change the current data to the next year.
     */
    Q_INVOKABLE void nextYear();
    
    /**
     * Convenience function for QML to change the current data to the previous month.
     */
    Q_INVOKABLE void previousMonth();
    
    /**
     * Convenience function for QML to change the current data to the previous year.
     */
    Q_INVOKABLE void previousYear();
    
    /**
     * Get the day name by the day possible days (1 - 7). See Qt DayOfWeek enum.
     */
    Q_INVOKABLE QString dayName(int weekday) const ;
    
    /**
     * This sets the currently selected date. This will also the data of the
     * selectedDayModel property since it will contain data of the selected day.
     */
    Q_INVOKABLE void setSelectedDay(int year, int month, int day) const;
    
    /**
     * Changing this will update the upcomingEventsModel based on the date you provided.
     */
    Q_INVOKABLE void upcommingEventsFromDay(int year, int month, int day) const;
    
signals:
    void startDateChanged();
    void typesChanged();
    void daysChanged();
    void weeksChanged();
    void startDayChanged();
    void errorMessageChanged();
    void sortingChanged();

public slots:
    /**
     * This function updates the actual data in the model (m_model) member.
     */
    void updateData();
    
private:
    QDate m_startDate;
    Types m_types;
    Sorting m_sorting;
    QList<DayData> m_dayList;
    QList<int> m_weekList;
    DaysModel* m_model;
    CalendarDayHelper* m_dayHelper;
    int m_days;
    int m_weeks;
    int m_startDay;
    QString m_errorMessage;
    CalendarData* m_calDataPerDay;
    CalendarData* m_upcommingEvents;
};

#endif // CALENDAR_H
