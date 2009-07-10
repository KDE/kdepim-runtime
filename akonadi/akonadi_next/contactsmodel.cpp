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

  /**
    Returns true if @p matchdata matches @p item using @p flags.
  */
  bool match(Item item, const QString &matchData, int flags ) const;

  /**
    Returns true if @p matchdata matches @p col using @p flags.
  */
  bool match(Collection col, const QString &matchData, int flags ) const;

  Q_DECLARE_PUBLIC(ContactsModel)
  ContactsModel *q_ptr;

  QStringList m_itemHeaders;
  QStringList m_collectionHeaders;

};


bool ContactsModelPrivate::match(Item item, const QString& matchData, int flags) const
{

  if (!item.hasPayload<KABC::Addressee>())
    return false;

  const KABC::Addressee addressee = item.payload<KABC::Addressee>();

  if ( addressee.familyName().startsWith(matchData, Qt::CaseInsensitive)
      || addressee.givenName().startsWith(matchData, Qt::CaseInsensitive)
      || addressee.preferredEmail().startsWith(matchData, Qt::CaseInsensitive))
    return true;


  if (item.hasAttribute<EntityDisplayAttribute>() &&
    !item.attribute<EntityDisplayAttribute>()->displayName().isEmpty() )
  {
    if (item.attribute<EntityDisplayAttribute>()->displayName().startsWith(matchData))
      return true;
  }

  return false;
}


bool ContactsModelPrivate::match(Collection col, const QString& matchData, int flags) const
{
  if (col.hasAttribute<EntityDisplayAttribute>() &&
      !col.attribute<EntityDisplayAttribute>()->displayName().isEmpty() )
    return col.attribute<EntityDisplayAttribute>()->displayName().startsWith(matchData);
  return col.name().startsWith(matchData);
}


ContactsModel::ContactsModel(Session *session, Monitor *monitor, QObject *parent)
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
        return addr.givenName() + " " + addr.familyName() + " " + "<" + addr.preferredEmail() + ">";
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


QModelIndexList ContactsModel::match(const QModelIndex& start, int role, const QVariant& value, int hits, Qt::MatchFlags flags) const
{
  Q_D(const ContactsModel);
  if (role != AmazingCompletionRole)
    return Akonadi::EntityTreeModel::match(start, role, value, hits, flags);

  if (QVariant::String != value.type())
    return QModelIndexList();

  QString matchData = value.toString();

  // Try to match names, and email addresses.
  QModelIndexList list;
  const int column = 0;
  int row = start.row();
  QModelIndex parentIdx = start.parent();
  int parentRowCount = rowCount(parentIdx);

  while (row < parentRowCount && (hits == -1 || list.size() < hits))
  {
    QModelIndex idx = index(row, column, parentIdx);
    Item item = idx.data(ItemRole).value<Item>();
    if (!item.isValid())
    {
      Collection col = idx.data(CollectionRole).value<Collection>();
      if (!col.isValid())
      {
        return QModelIndexList();
      }
      if (d->match(col, matchData, flags))
        list << idx;
    } else {
      if (d->match(item, matchData, flags))
      {
        list << idx;
      }
    }
    ++row;
  }
  return list;
}



#include "contactsmodel.moc"
