
#include "contactsmodel.h"

#include <kabc/addressee.h>
#include <kabc/contactgroup.h>

#include <kdebug.h>

using namespace Akonadi;

ContactsModel::ContactsModel(EntityUpdateAdapter *eua, Monitor *monitor, QObject *parent)
  : EntityTreeModel(eua, monitor, parent)
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
      // The payload should be a KABC::Addressee. Why does it claim not to be?
      Q_ASSERT( item.hasPayload<KABC::Addressee>() );
      // Pass modeltest
      if (role == Qt::DisplayRole)
        return item.remoteId();
      return QVariant();
    }
    const KABC::Addressee addr = item.payload<KABC::Addressee>();

    if (role == Qt::DisplayRole)
    {
      switch (column)
      {
      case 0:
        return addr.formattedName();
      case 1:
        return addr.preferredEmail();
      }
    }
  }
  return EntityTreeModel::getData(item, column, role);
}

QVariant ContactsModel::getData(Collection collection, int column, int role) const
{
  if (column ==0)
  {
    QVariant v = EntityTreeModel::getData(collection, column, role);
    return v;
  }
  else {
    // Return a QString to pass modeltest.
    return QString();
//     return QVariant();
  }
}

int ContactsModel::columnCount(const QModelIndex &index) const
{
  Q_UNUSED(index);
  return 2;
}

#include "contactsmodel.moc"
