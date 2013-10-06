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

#include "calendarroleproxymodel.h"

#include <kcalcore/incidence.h>
#include <KDateTime>
#include <QDebug>

CalendarRoleProxyModel::CalendarRoleProxyModel(QObject *parent) :
    KIdentityProxyModel(parent)
{
}

QVariant CalendarRoleProxyModel::data(const QModelIndex &index, int role) const
{
    if (role < SummaryRole)
        return KIdentityProxyModel::data(index, role);

    const Akonadi::Item item = index.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();


    if (role == SummaryRole)
        return item.payload<KCalCore::Incidence::Ptr>()->summary();
    else if (role == DescriptionRole)
        return item.payload<KCalCore::Incidence::Ptr>()->description();
    else if (role == MimeTypeRole)
        return item.mimeType();
    else if (role == StartDateRole)
        return item.payload<KCalCore::Incidence::Ptr>()->dtStart().dateTime(); // QDateTime for now.
    else if (role == IdRole)
        return item.id();
    
    return QVariant();
}

void CalendarRoleProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    KIdentityProxyModel::setSourceModel(sourceModel);

    QHash<int, QByteArray> roleNames = sourceModel->roleNames();

    roleNames.insert(SummaryRole, "summary");
    roleNames.insert(DescriptionRole, "description");
    roleNames.insert(MimeTypeRole, "mimeType");
    roleNames.insert(StartDateRole, "startDate");
    roleNames.insert(IdRole, "id");

    setRoleNames(roleNames);

    qDebug() << "--> Rolenames: " << roleNames;
}
