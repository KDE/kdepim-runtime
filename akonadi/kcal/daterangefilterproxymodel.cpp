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

#include "daterangefilterproxymodel.h"

#include <KDateTime>

#include <QVariant>

using namespace Akonadi;

class DateRangeFilterProxyModel::Private {
  public:
    explicit Private() : column( 0 ), startT( 0 ), endT( 0 ) {}
    int column;
    KDateTime start;
    KDateTime end;
    uint startT;
    uint endT;
};

DateRangeFilterProxyModel::DateRangeFilterProxyModel( QObject* parent ) : QSortFilterProxyModel( parent ), d( new Private ) {
}

DateRangeFilterProxyModel::~DateRangeFilterProxyModel() {
  delete d;
}

KDateTime DateRangeFilterProxyModel::startDate() const {
  return d->start;
}

void DateRangeFilterProxyModel::setStartDate( const KDateTime& date ) {
  d->start = date;
  d->startT = date.toTime_t();
}

KDateTime DateRangeFilterProxyModel::endDate() const {
  return d->end;
}

void DateRangeFilterProxyModel::setEndDate( const KDateTime& date ) {
  d->end = date;
  d->endT = date.toTime_t();
}
  
int DateRangeFilterProxyModel::dateColumn() const {
  return d->column;
}

void DateRangeFilterProxyModel::setDateColumn( int column ) {
  d->column = column;
}

bool DateRangeFilterProxyModel::filterAcceptsRow( int source_row, const QModelIndex& source_parent ) const {
  const QModelIndex idx = source_parent.child( source_row, d->column );
  const QVariant v = idx.data( filterRole() );
  if ( !v.isValid() )
    return false;
  bool ok;
  const uint tt = v.toUInt( &ok );
  if ( !ok )
    return false;
  if ( d->start.isValid() && tt < d->startT )
    return false;
  if ( d->end.isValid() && tt > d->endT )
    return false;
  return true;
}
