/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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

#include "contactsmodel.h"

#include <kabc/addressee.h>
#include <kabc/contactgroup.h>

#include <kdebug.h>
#include <akonadi/entitydisplayattribute.h>

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


bool ContactsModel::match(Item item, const QVariant& matchData, Qt::MatchFlags flags) const
{
  if (!item.hasPayload<KABC::Addressee>())
    return false;

  if (!matchData.canConvert(QVariant::String))
    return false;

  const QString matchString = matchData.toString();

  const KABC::Addressee addressee = item.payload<KABC::Addressee>();

  if ( addressee.familyName().startsWith(matchString, Qt::CaseInsensitive)
      || addressee.givenName().startsWith(matchString, Qt::CaseInsensitive)
      || addressee.preferredEmail().startsWith(matchString, Qt::CaseInsensitive))
    return true;


  if (item.hasAttribute<EntityDisplayAttribute>() &&
    !item.attribute<EntityDisplayAttribute>()->displayName().isEmpty() )
  {
    if (item.attribute<EntityDisplayAttribute>()->displayName().startsWith(matchString))
      return true;
  }

  return false;
}


bool ContactsModel::match(Collection col, const QVariant& matchData, Qt::MatchFlags flags) const
{
  if (!matchData.canConvert(QVariant::String))
    return false;

  const QString matchString = matchData.toString();

  if (col.hasAttribute<EntityDisplayAttribute>() &&
      !col.attribute<EntityDisplayAttribute>()->displayName().isEmpty() )
    return col.attribute<EntityDisplayAttribute>()->displayName().startsWith(matchString);
  return col.name().startsWith(matchString);
}


ContactsModel::ContactsModel(Session *session, ChangeRecorder *monitor, QObject *parent)
  : EntityTreeModel(session, monitor, parent), d_ptr(new ContactsModelPrivate(this))
{

}

ContactsModel::~ContactsModel()
{
   delete d_ptr;
}

QVariant ContactsModel::getData(const Item &item, int column, int role) const
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
        return addr.givenName() + ' ' + addr.familyName() + ' ' + '<' + addr.preferredEmail() + '>';
      }
    }
  }
  return EntityTreeModel::getData(item, column, role);
}

QVariant ContactsModel::getData(const Collection &collection, int column, int role) const
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

QVariant ContactsModel::getHeaderData( int section, Qt::Orientation orientation, int role, int headerSet ) const
{
  Q_D(const ContactsModel);

  if (orientation == Qt::Horizontal)
  {
    if ( headerSet == EntityTreeModel::CollectionTreeHeaders )
    {
      if (role == Qt::DisplayRole)
      {
        if (section >= d->m_collectionHeaders.size() )
          return QVariant();
        return d->m_collectionHeaders.at(section);
      }
    } else if (headerSet == EntityTreeModel::ItemListHeaders)
    {
      if (role == Qt::DisplayRole)
      {
        if (section >= d->m_itemHeaders.size() )
          return QVariant();
        return d->m_itemHeaders.at(section);
      }
    }
  }

  return EntityTreeModel::getHeaderData(section, orientation, role, headerSet);
}

#include "contactsmodel.moc"
