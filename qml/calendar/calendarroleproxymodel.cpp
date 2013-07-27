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
    if (role == SummaryRole) {
        if(item.payload<KCalCore::Incidence::Ptr>()->summary().isEmpty())
            return "I am empty !!!!";
       return item.payload<KCalCore::Incidence::Ptr>()->summary();}
    else if (role == DescriptionRole)
        return item.payload<KCalCore::Incidence::Ptr>()->description();
    else if (role == MimeTypeRole)
        return item.mimeType();
    else if (role == StartDateRole )
        if(item.payload<KCalCore::Incidence::Ptr>()->dtStart().date().isNull()) qDebug()<<"startdate null 999999999999999999999";
        //if(item.payload<KCalCore::Incidence::Ptr>()->dtStart().date().isEmpty()) qDebug()<<"empty 7777777777777777777777";
        return item.payload<KCalCore::Incidence::Ptr>()->dtStart().date(); // QDateTime for now.
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


    setRoleNames(roleNames);

    qDebug() << "--> Rolenames: " << roleNames;
}
