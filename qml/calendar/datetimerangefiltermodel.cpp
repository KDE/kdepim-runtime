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

#include "datetimerangefiltermodel.h"
#include "calendarroleproxymodel.h"

#include <QDateTime>
#include <QDebug>

#include <akonadi/entitytreemodel.h>
#include <akonadi/item.h>
#include <kcalcore/event.h>
#include <kcalcore/journal.h>
#include <kcalcore/todo.h>
#include <kcalcore/incidence.h>
#include <KDateTime>

DateTimeRangeFilterModel::DateTimeRangeFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_overlapping(false)
{
}

void DateTimeRangeFilterModel::setStartDate(const QDate &date)
{
    if (m_startDate == date)
        return;

    m_startDate = date;

    invalidateFilter();
}

void DateTimeRangeFilterModel::setEndDate(const QDate &date)
{
    if (m_endDate == date)
        return;

    m_endDate = date;

    invalidateFilter();
}

void DateTimeRangeFilterModel::includeOverlappingEvents(bool overlapping)
{
    if(m_overlapping == overlapping) {
        return;
    }
    m_overlapping = overlapping;
    invalidateFilter();
}

bool DateTimeRangeFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (!m_startDate.isValid() && !m_endDate.isValid())
        return true;

    const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    const Akonadi::Item item = sourceModel()->data(index, Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();

    KDateTime incidenceStart;
    KDateTime incidenceEnd;
    if (item.mimeType() == KCalCore::Event::eventMimeType()) {
        incidenceStart = item.payload<KCalCore::Event::Ptr>()->dtStart();
        incidenceEnd = item.payload<KCalCore::Event::Ptr>()->dtEnd();
    } else if (item.mimeType() == KCalCore::Todo::todoMimeType()) {
        incidenceStart = item.payload<KCalCore::Todo::Ptr>()->dtStart();
        incidenceEnd = item.payload<KCalCore::Todo::Ptr>()->dtStart();
    } else if (item.mimeType() == KCalCore::Journal::journalMimeType()) {
        incidenceStart = item.payload<KCalCore::Journal::Ptr>()->dtStart();
        incidenceEnd = item.payload<KCalCore::Journal::Ptr>()->dtStart();
    }

    if(!m_overlapping) {
        if(m_startDate.isValid() && m_startDate == incidenceStart.date()) {
            return true;
        }
    } else {
        if (m_startDate.isValid() && !m_endDate.isValid()) {
            if (incidenceEnd.date() >= m_startDate)
                return true;
        } else if (!m_startDate.isValid() && m_endDate.isValid()) {
            if (incidenceStart.date() <= m_endDate)
                return true;
        } else if (m_startDate.isValid() && m_endDate.isValid()) {
            if (incidenceStart.date() < m_startDate && incidenceEnd.date() >= m_startDate)
                return true;
            if (incidenceStart.date() < m_endDate && incidenceEnd.date() >= m_endDate)
                return true;
            if (incidenceStart.date() <= m_startDate && incidenceEnd.date() >= m_endDate)
                return true;
            if (incidenceStart.date() >= m_startDate && incidenceEnd.date() <= m_endDate)
                return true;
        }
    }


    return false;
}

bool DateTimeRangeFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const Akonadi::Item itemLeft = left.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
    const Akonadi::Item itemRight = right.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
    return itemLeft.payload<KCalCore::Incidence::Ptr>()->dtStart().dateTime() < itemRight.payload<KCalCore::Incidence::Ptr>()->dtStart().dateTime();
}
