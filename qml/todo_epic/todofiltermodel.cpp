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

#include "todofiltermodel.h"
#include "todomodel.h"
#include <QVariant>
#include <QDate>

TodoFilterModel::TodoFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    m_period = 365;
    m_excludedCollections = QStringList();
    m_disabledCategories = QStringList();
}

TodoFilterModel::~TodoFilterModel()
{
}

void TodoFilterModel::setPeriod(int period)
{
    m_period = period;
    invalidateFilter();
}

void TodoFilterModel::setShowFinishedTodos(bool showFinishedTodos)
{
    m_showFinishedTodos = showFinishedTodos;
    invalidateFilter();
}

void TodoFilterModel::setDisabledTypes(QStringList types)
{
    m_disabledTypes = types;
    m_disabledTypes.sort();
    invalidateFilter();
}

void TodoFilterModel::setExcludedCollections(QStringList collections)
{
    m_excludedCollections = collections;
    m_excludedCollections.sort();
    invalidateFilter();
}

void TodoFilterModel::setDisabledCategories(QStringList categories)
{
    m_disabledCategories = categories;
    m_disabledCategories.sort();
    invalidateFilter();
}

bool TodoFilterModel::isDisabledType(QModelIndex idx) const
{
    bool isDisabled = false;
    const int type = idx.data(TodoModel::ItemTypeRole).toInt();
    if (type == TodoModel::NormalItem || type == TodoModel::BirthdayItem || type == TodoModel::AnniversaryItem) {
        if (m_disabledTypes.contains("events"))
            isDisabled = true;
    } else if (type == TodoModel::TodoItem) {
        if (m_disabledTypes.contains("todos"))
            isDisabled = true;
    }

    return isDisabled;
}

bool TodoFilterModel::isDisabledCategory(QModelIndex idx) const
{
    const QMap<QString, QVariant> values = idx.data(Qt::DisplayRole).toMap();
    QStringList itemCategories = values["categories"].toStringList();
    QStringList allCategories = m_disabledCategories + itemCategories;

    bool allItemCategoriesDisabled = allCategories.removeDuplicates() == itemCategories.count();
    return allItemCategoriesDisabled;
}

bool TodoFilterModel::filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent ) const
{
    const QModelIndex idx = sourceModel()->index( sourceRow, 0, sourceParent );

    const int itemType = idx.data(TodoModel::ItemTypeRole).toInt();
    const QString collectionRole = idx.data(TodoModel::CollectionRole).toString();

    const QVariant d = idx.data(TodoModel::SortRole);
    const QDate date= d.toDate();

    if (date.isValid()) {
        if (date > QDate::currentDate().addDays(365)) { // todos with no specified due date
            if (itemType == TodoModel::HeaderItem) {
                int rows = sourceModel()->rowCount(idx);
                for (int row = 0; row < rows; ++ row) {
                    QModelIndex childIdx = sourceModel()->index(row, 0, idx);
                    const QString cr = childIdx.data(TodoModel::CollectionRole).toString();
                    if (!m_excludedCollections.contains(cr) && !isDisabledType(childIdx) && !isDisabledCategory(childIdx)) {
                        const QMap<QString, QVariant> values = childIdx.data(Qt::DisplayRole).toMap();
                        if (m_showFinishedTodos || values["completed"].toBool() == false)
                            return true;
                    }
                }
                return false;
            } else {
                if (!m_excludedCollections.contains(collectionRole) && !isDisabledType(idx) && !isDisabledCategory(idx)) {
                    const QMap<QString, QVariant> values = idx.data(Qt::DisplayRole).toMap();
                    if (m_showFinishedTodos || values["completed"].toBool() == false) {
                        return true;
                    }
                }
                return false;
            }
        } else if (date > QDate::currentDate().addDays(m_period)) { 
            return false;
        } else if (date < QDate::currentDate()) {
            if (itemType == TodoModel::HeaderItem) {
                if (sourceModel()->hasChildren(idx)) {
                    int rows = sourceModel()->rowCount(idx);
                    for (int row = 0; row < rows; ++row) {
                        QModelIndex childIdx = sourceModel()->index(row, 0, idx);
                        const QString cr = childIdx.data(TodoModel::CollectionRole).toString();
                        if (!m_excludedCollections.contains(cr) && !isDisabledType(childIdx) && !isDisabledCategory(childIdx)) {
                            const int childType = childIdx.data(TodoModel::ItemTypeRole).toInt();
                            const QMap<QString, QVariant> values = childIdx.data(Qt::DisplayRole).toMap();
                            if (childType == TodoModel::TodoItem) { 
                                if (values["completed"].toBool() == false)
                                    return true;
                            } else {
                                if (values["endDate"].toDate() >= QDate::currentDate())
                                    return true;
                            }
                        }
                    }
                }
                return false;
            } else {
                if (m_excludedCollections.contains(collectionRole) || isDisabledType(idx) || isDisabledCategory(idx)) {
                    return false;
                }

                const QMap<QString, QVariant> values = idx.data(Qt::DisplayRole).toMap();
                if (itemType == TodoModel::TodoItem) {
                    if (values["completed"].toBool() == true) {
                        return false;
                    }
                } else if (values["endDate"].toDate() < QDate::currentDate()) {
                    return false;
                }
            }
        } else {
            if (itemType == TodoModel::HeaderItem) { 
                int rows = sourceModel()->rowCount(idx);
                for (int row = 0; row < rows; ++ row) {
                    QModelIndex childIdx = sourceModel()->index(row, 0, idx);
                    const QString cr = childIdx.data(TodoModel::CollectionRole).toString();
                    const QDate cd = childIdx.data(TodoModel::SortRole).toDate();
                    if ((!m_excludedCollections.contains(cr) && !isDisabledType(childIdx) && !isDisabledCategory(childIdx)) && cd <= QDate::currentDate().addDays(m_period)) {
                        const int childType = childIdx.data(TodoModel::ItemTypeRole).toInt();
                        const QMap<QString, QVariant> values = childIdx.data(Qt::DisplayRole).toMap();
                        if (childType != TodoModel::TodoItem) {
                            return true;
                        } else if (m_showFinishedTodos || values["completed"].toBool() == false) {
                            return true;
                        }
                    }
                }
                return false;
            } else if (m_excludedCollections.contains(collectionRole) || isDisabledType(idx) || isDisabledCategory(idx)) {
                return false;
            } else if (itemType == TodoModel::TodoItem) {
                const QMap<QString, QVariant> values = idx.data(Qt::DisplayRole).toMap();
                if (!m_showFinishedTodos && values["completed"].toBool() == true)
                    return false;
            }
        }
    } else {
        return false;
    }

    return true;
}
