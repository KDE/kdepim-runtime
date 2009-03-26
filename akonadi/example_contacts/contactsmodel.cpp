
#include "contactsmodel.h"

#include <kabc/addressee.h>
#include <kabc/contactgroup.h>

#include <kdebug.h>

using namespace Akonadi;

class ContactsModelPrivate
{
public:
  ContactsModelPrivate(ContactsModel *model)
    : q_ptr(model)
  {

  }

  Q_DECLARE_PUBLIC(ContactsModel);
  ContactsModel *q_ptr;

};

ContactsModel::ContactsModel(Session *session, Monitor *monitor, QObject *parent)
  : EntityTreeModel(session, monitor, parent)
{

}

ContactsModel::~ContactsModel()
{

}

QVariant ContactsModel::getData(Item item, int column, int role) const
{
  if ( item.mimeType() == "text/directory" )
  {
    if ( !item.hasPayload<KABC::Addressee>() )
    {
      // Pass modeltest
      if (role == Qt::DisplayRole)
        return item.remoteId();
      return QVariant();
    }
    const KABC::Addressee addr = item.payload<KABC::Addressee>();

    if ((role == Qt::DisplayRole) || (role == Qt::EditRole))
    {
      switch (column)
      {
      case 0:
        return addr.familyName();
      case 1:
        return addr.preferredEmail();
      }
    }

    if (role == EmailCompletionRole && column == 0)
    {
      return addr.givenName() + addr.familyName() + "<" + addr.preferredEmail() + ">";
    }
  }
  return EntityTreeModel::getData(item, column, role);
}

QVariant ContactsModel::getData(Collection collection, int column, int role) const
{
  if (role == Qt::DisplayRole)
  {
    switch (column)
    {
    case 0:
      return EntityTreeModel::getData(collection, column, role);
    case 1:
      return rowCount(EntityTreeModel::indexForCollection(collection));
    default:
      // Return a QString to pass modeltest.
      return QString();
  //     return QVariant();
    }
  }
  return EntityTreeModel::getData(collection, column, role);
}

int ContactsModel::columnCount(const QModelIndex &index) const
{
  Q_UNUSED(index);
  return 2;
}

QVariant ContactsModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
//   if (role == EntityTreeModel::CollectionTreeHeaderDisplayRole)
//   {
//     switch(section)
//     {
//     case 0:
//       return "Collection";
//     case 1:
//       return "Count";
//     }
//
//   } else if (role == EntityTreeModel::ItemListHeaderDisplayRole)
//   {
//     switch(section)
//     {
//     case 0:
//       return "Given Name";
//     case 1:
//       return "Family Name";
//     }
//   }
  return EntityTreeModel::headerData(section, orientation, role);
}

#include "contactsmodel.moc"
