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
   // if (!m_startDate.isValid() && !m_endDate.isValid())
    //    return true;

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
        //QDate date(QDate::currentDate());
        if (incidenceStart.date().year() == -(m_startDate.year()+2700))
            {
                qDebug()<<m_startDate.year()+2700;
                return true;
            }
       /* else if (m_startDate.isValid() && !m_endDate.isValid()) {
            if (incidenceEnd.date() >= m_startDate)
                return true;
        } 
        else if (!m_startDate.isValid() && m_endDate.isValid()) {
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
        }*/
    }


    return false;
}

bool DateTimeRangeFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const Akonadi::Item itemLeft = left.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
    const Akonadi::Item itemRight = right.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
    return itemLeft.payload<KCalCore::Incidence::Ptr>()->dtStart().dateTime() < itemRight.payload<KCalCore::Incidence::Ptr>()->dtStart().dateTime();
}
