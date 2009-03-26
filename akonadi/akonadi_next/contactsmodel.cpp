
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
    m_collectionHeaders << "Collection" << "Count";
    m_itemHeaders << "Given Name" << "Family Name" << "Email";
  }

  Q_DECLARE_PUBLIC(ContactsModel)
  ContactsModel *q_ptr;

  QStringList m_itemHeaders;
  QStringList m_collectionHeaders;

};

ContactsModel::ContactsModel(Session *session, Monitor *monitor, QObject *parent)
  : EntityTreeModel(session, monitor, parent), d_ptr(new ContactsModelPrivate(this))
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
        return addr.givenName();
      case 1:
        return addr.familyName();
      case 2:
        return addr.preferredEmail();
      case 3:
        return addr.givenName() + " " + addr.familyName() + " " + "<" + addr.preferredEmail() + ">";
      }
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
  return 4;
}

QVariant ContactsModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  Q_D(const ContactsModel);
  const int roleSectionSize = 1000;
  const int headerType = (role / roleSectionSize);

  if (orientation == Qt::Horizontal)
  {
    if (headerType == EntityTreeModel::CollectionTreeHeaders)
    {
      role %= roleSectionSize;
      if (role == Qt::DisplayRole)
      {
        if (section >= d->m_collectionHeaders.size() )
          return QVariant();
        return d->m_collectionHeaders.at(section);
      }
    } else if (headerType == EntityTreeModel::ItemListHeaders)
    {
      role %= roleSectionSize;
      if (role == Qt::DisplayRole)
      {
        if (section >= d->m_itemHeaders.size() )
          return QVariant();
        return d->m_itemHeaders.at(section);
      }
    }
  }


  return EntityTreeModel::headerData(section, orientation, role);
}

#include "contactsmodel.moc"
