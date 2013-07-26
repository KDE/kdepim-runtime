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
