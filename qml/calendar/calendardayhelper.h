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

#ifndef CALENDARDAYHELPER_H
#define CALENDARDAYHELPER_H

#include <QObject>
#include <akonadi/calendar/etmcalendar.h>
#include <kcalcore/event.h>

class QAbstractItemModel;

class CalendarDayHelper : public QObject
{
    Q_OBJECT
public:
    explicit CalendarDayHelper(QObject *parent = 0);
    
    void setDate(int year, int month);

    bool containsHolidayItems(int day);
    bool containsEventItems(int day);
    bool containsTodoItems(int day);
    bool containsJournalItems(int day);

    void updateCurrentDatModel();
    QAbstractItemModel* selectedDayModel() const;

signals:
    void calendarChanged();
    
public slots:
    void fillLists();

private:
    Akonadi::ETMCalendar * m_cal;
    int m_year;
    int m_month;
    KCalCore::Event::List m_eventList;
    KCalCore::Todo::List m_todoList;
    KCalCore::Journal::List m_journalList;
};

#endif // CALENDARDAYHELPER_H
