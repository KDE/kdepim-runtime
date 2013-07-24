#ifndef CALENDARROLEPROXYMODEL_H
#define CALENDARROLEPROXYMODEL_H

#include <akonadi/entitytreemodel.h>
#include <kidentityproxymodel.h>

class CalendarRoleProxyModel : public KIdentityProxyModel
{
    Q_OBJECT
public:
    enum Roles {
        SummaryRole = Akonadi::EntityTreeModel::UserRole,
        DescriptionRole,
        MimeTypeRole,
    };

    explicit CalendarRoleProxyModel(QObject *parent = 0);
    
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void setSourceModel(QAbstractItemModel *sourceModel);
};

#endif // CALENDARROLEPROXYMODEL_H
