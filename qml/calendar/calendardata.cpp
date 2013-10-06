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

#include "calendardata.h"

#include "calendarroleproxymodel.h"
#include "datetimerangefiltermodel.h"

#include <akonadi/calendar/etmcalendar.h>
#include <akonadi/entitytreemodel.h>
#include <akonadi/entitymimetypefiltermodel.h>
#include <kdescendantsproxymodel.h>

using namespace Akonadi;

CalendarData::CalendarData(QObject *parent)
    : QObject(parent)
    , m_types(Holiday | Event | Todo | Journal)
    , m_sorting(None)
    , m_overlapping(false)
{
    m_etmCalendar = new ETMCalendar();
    m_etmCalendar->setParent(this); //TODO: hit sergio

    EntityTreeModel *model = m_etmCalendar->entityTreeModel();
    model->setCollectionFetchStrategy(EntityTreeModel::InvisibleCollectionFetch);

    m_itemList = new EntityMimeTypeFilterModel(this);
    m_itemList->setSourceModel(model);

    CalendarRoleProxyModel *roleModel = new CalendarRoleProxyModel(this);
    roleModel->setSourceModel(m_itemList);

    m_filteredList = new DateTimeRangeFilterModel(this);
    m_filteredList->setSourceModel(roleModel);

    updateTypes();
}

void CalendarData::updateTypes()
{
    m_itemList->clearFilters();

    QStringList mimeTypes;
    if (m_types & Event)
        mimeTypes << KCalCore::Event::eventMimeType();

    if (m_types & Todo)
        mimeTypes << KCalCore::Todo::todoMimeType();

    if (m_types & Journal)
        mimeTypes << KCalCore::Journal::journalMimeType();

    qDebug() << mimeTypes;
    m_itemList->addMimeTypeInclusionFilters(mimeTypes);
}


QDate CalendarData::startDate() const
{
    return m_startDate;
}

void CalendarData::setStartDate(const QDate &dateTime)
{
    if (m_startDate == dateTime)
        return;

    m_startDate = dateTime;
    m_filteredList->setStartDate(m_startDate);
    emit startDateChanged();
}

QDate CalendarData::endDate() const
{
    return m_endDate;
}

void CalendarData::setEndDate(const QDate &dateTime)
{
    if (m_endDate == dateTime)
        return;

    m_endDate = dateTime;
    m_filteredList->setEndDate(m_endDate);
    emit endDateChanged();
}

int CalendarData::types() const
{
    return m_types;
}

void CalendarData::setTypes(int types)
{
    if (m_types == types)
        return;

    m_types = static_cast<Types>(types);
    updateTypes();

    emit typesChanged();
}

QString CalendarData::errorMessage() const
{
    return QString();
}

bool CalendarData::loading() const
{
    return false;
}

QAbstractItemModel *CalendarData::model() const
{
    return m_filteredList;
}

int CalendarData::sorting() const
{
    return m_sorting;
}

void CalendarData::setSorting(int sorting)
{
    if (m_sorting == sorting)
        return;

    m_sorting = static_cast<Sorting>(sorting);

    if(m_sorting & Ascending) {
        m_filteredList->sort(0, Qt::AscendingOrder);
        m_filteredList->setDynamicSortFilter(true);
    } else if (m_sorting & Descending) {
        m_filteredList->sort(0, Qt::DescendingOrder);
        m_filteredList->setDynamicSortFilter(true);
    } else if (m_sorting & None) {
        m_filteredList->setDynamicSortFilter(false);
    }

    emit sortingChanged();
}

bool CalendarData::showOverlapping()
{
    return m_overlapping;
}

void CalendarData::setShowOverlapping(bool overlapping)
{
    if(m_overlapping == overlapping) {
        return;
    }
    m_overlapping = overlapping;
    m_filteredList->includeOverlappingEvents(m_overlapping);
}
