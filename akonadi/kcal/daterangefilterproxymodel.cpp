/*
    Copyright (c) 2009 KDAB
    Author: Frank Osterfeld <osterfeld@kde.org>

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

#include "calendarmodel.h"
#include "daterangefilterproxymodel.h"

#include <KDateTime>

#include <QVariant>

using namespace Akonadi;

class DateRangeFilterProxyModel::Private {
  public:
  explicit Private() : startColumn( CalendarModel::DateTimeStart ), endColumn( CalendarModel::DateTimeEnd ), startT( 0 ), endT( 0 ) {}
    int startColumn;
    int endColumn;
    KDateTime start;
    KDateTime end;
    uint startT;
    uint endT;
};

DateRangeFilterProxyModel::DateRangeFilterProxyModel( QObject* parent ) : QSortFilterProxyModel( parent ), d( new Private ) {
  setFilterRole( CalendarModel::SortRole );
}

DateRangeFilterProxyModel::~DateRangeFilterProxyModel() {
  delete d;
}

KDateTime DateRangeFilterProxyModel::startDate() const {
  return d->start;
}

void DateRangeFilterProxyModel::setStartDate( const KDateTime& date ) {
  if ( date == d->start )
    return;
  d->start = date;
  d->startT = date.toTime_t();
  invalidateFilter();
}

KDateTime DateRangeFilterProxyModel::endDate() const {
  return d->end;
}

void DateRangeFilterProxyModel::setEndDate( const KDateTime& date ) {
  if ( date == d->end )
    return;
  d->end = date;
  d->endT = date.toTime_t();
  invalidateFilter();
}
  
int DateRangeFilterProxyModel::startDateColumn() const {
  return d->startColumn;
}

void DateRangeFilterProxyModel::setStartDateColumn( int column ) {
  if ( column == d->startColumn )
    return;
  d->startColumn = column;
  invalidateFilter();
}

int DateRangeFilterProxyModel::endDateColumn() const {
  return d->endColumn;
}

void DateRangeFilterProxyModel::setEndDateColumn( int column ) {
  if ( column == d->endColumn )
    return;
  d->endColumn = column;
  invalidateFilter();
}
bool DateRangeFilterProxyModel::filterAcceptsRow( int source_row, const QModelIndex& source_parent ) const {

  if ( d->end.isValid() ) {
    const QModelIndex idx = sourceModel()->index( source_row, d->startColumn, source_parent );
    const QVariant v = idx.data( filterRole() );
    bool ok;
    const uint start = v.toUInt( &ok );
    if ( ok && start > d->endT )
      return false;
  }

  const bool recurs = sourceModel()->index( source_row, 0, source_parent ).data( CalendarModel::RecursRole ).toBool();

  if ( recurs ) // that's fuzzy and might return events not actually recurring in the range
    return true;

  if ( d->start.isValid() ) {
    const QModelIndex idx = sourceModel()->index( source_row, d->endColumn, source_parent );
    const QVariant v = idx.data( filterRole() );
    bool ok;
    const uint end = v.toUInt( &ok );
    if ( ok && end < d->startT )
      return false;
  }
  return true;
}
