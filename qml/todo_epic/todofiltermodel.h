/*
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 *   Copyright (C) 2014 by Heena Mahour <heena393@gmail.com>
 */

#ifndef TODOFILTERMODEL_H
#define TODOFILTERMODEL_H

#include <QSortFilterProxyModel>
#include <QStringList>

class TodoFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit TodoFilterModel(QObject *parent = 0);
    ~TodoFilterModel();

    void setPeriod(int period);
    void setShowFinishedTodos(bool showFinishedTodos);
    void setDisabledTypes(QStringList types);
    void setExcludedCollections(QStringList collections);
    void setDisabledCategories(QStringList categories);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    bool isDisabledType(QModelIndex idx) const;
    bool isDisabledCategory(QModelIndex idx) const;

private:
    int m_period;
    bool m_showFinishedTodos;
    QStringList m_disabledTypes, m_excludedCollections, m_disabledCategories;
};

#endif
