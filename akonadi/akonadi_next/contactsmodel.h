
#ifndef CONTACTSMODEL_H
#define CONTACTSMODEL_H

#include "entitytreemodel.h"

#include "akonadi_next_export.h"

using namespace Akonadi;

class ContactsModelPrivate;

class AKONADI_NEXT_EXPORT ContactsModel : public EntityTreeModel
{
  Q_OBJECT
public:

  enum Roles
  {
    EmailCompletionRole = EntityTreeModel::UserRole
  };

  ContactsModel(Session *session, Monitor *monitor, QObject *parent = 0);
  virtual ~ContactsModel();

  virtual QVariant getData(Item item, int column, int role=Qt::DisplayRole) const;

  virtual QVariant getData(Collection collection, int column, int role=Qt::DisplayRole) const;

  virtual int columnCount(const QModelIndex &index = QModelIndex()) const;

  virtual QVariant headerData( int section, Qt::Orientation orientation, int role ) const;

private:
    Q_DECLARE_PRIVATE(ContactsModel);
    ContactsModelPrivate *d_ptr;
};

#endif
