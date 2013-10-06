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

#include "calendardayhelper.h"

#include <QDebug>

CalendarDayHelper::CalendarDayHelper(QObject *parent)
    : QObject(parent)
    , m_year(0)
    , m_month(0)
{
    m_cal = new Akonadi::ETMCalendar();
    m_cal->setCollectionFilteringEnabled(false);

    connect(m_cal, SIGNAL(calendarChanged()), this, SLOT(fillLists()));
    connect(m_cal, SIGNAL(calendarChanged()), this, SIGNAL(calendarChanged()));

//    QTreeView* tv = new QTreeView();
//    tv->setModel(m_cal->model());
//    tv->show();
}

void CalendarDayHelper::setDate(int year, int month)
{
    if(m_year != year) {
        m_year = year;
    }

    if(m_month != month) {
        m_month = month;
    }

    // Also fill the list if the date changes
    fillLists();
}

bool CalendarDayHelper::containsHolidayItems(int day)
{
    // Not implemented yet
    return false;
}

bool CalendarDayHelper::containsEventItems(int day)
{
    QDate compareDate(m_year, m_month, day);
    //qDebug() << "Going to check events against a list with:" << m_eventList.count() << "items on day: " << day << ", Year: " << m_year << " and month: " << m_month;
    foreach(KCalCore::Event::Ptr event, m_eventList) {
        //qDebug() << "Checking entry against date:" << compareDate << "with dtStart:" << event->dtStart().date() << "and dtEnd:" << event->dtEnd().date();

        // Keep this line as comment for now. This line works for overlapping calendar events as well.
        //if(event->dtStart().date() <= compareDate && event->dtEnd().date() >= compareDate) {

        // This condition only enters when the event started todat
        if(event->dtStart().date() == compareDate) {
            return true;
        }
    }
    return false;
}

bool CalendarDayHelper::containsTodoItems(int day)
{
  // Not implemented yet
  return false;
}

bool CalendarDayHelper::containsJournalItems(int day)
{
  // Not implemented yet
  return false;
}

void CalendarDayHelper::fillLists()
{
    // Note: this function is "just" filling the events from the current month.
    // It should be extended to fill the events from the exact date ranges as provided in calendar.cpp/h
    QDate date(m_year, m_month, 1);
    QDate endDate = date.addMonths(1);
    m_eventList = m_cal->rawEvents(date, endDate);
    m_todoList = m_cal->rawTodos(date, endDate);

}
