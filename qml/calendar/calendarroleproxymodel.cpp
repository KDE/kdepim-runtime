#include "calendarroleproxymodel.h"

#include <kcalcore/incidence.h>

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
    
    return QVariant();
}

void CalendarRoleProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    KIdentityProxyModel::setSourceModel(sourceModel);

    QHash<int, QByteArray> roleNames = sourceModel->roleNames();

    roleNames.insert(SummaryRole, "summary");
    roleNames.insert(DescriptionRole, "description");
    roleNames.insert(MimeTypeRole, "mimeType");

    setRoleNames(roleNames);
}
