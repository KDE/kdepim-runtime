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

#ifndef DATETIMERANGEFILTERMODEL_H
#define DATETIMERANGEFILTERMODEL_H

#include <QDate>
#include <QSortFilterProxyModel>

class DateTimeRangeFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit DateTimeRangeFilterModel(QObject *parent = 0);

    void setStartDate(const QDate &date);
    void setEndDate(const QDate &date);
    void includeOverlappingEvents(bool overlapping);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

private:
    QDate m_startDate;
    QDate m_endDate;
    bool m_overlapping;
};

#endif
