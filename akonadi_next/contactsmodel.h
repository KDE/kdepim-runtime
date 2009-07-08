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

  virtual QVariant getData(const Item &item, int column, int role=Qt::DisplayRole) const;

  virtual QVariant getData(const Collection &collection, int column, int role=Qt::DisplayRole) const;

  virtual int columnCount(const QModelIndex &index = QModelIndex()) const;

  virtual QVariant getHeaderData( int section, Qt::Orientation orientation, int role, int headerSet ) const;

  virtual QModelIndexList match(const QModelIndex & start, int role, const QVariant &value, int hits = 1,
          Qt::MatchFlags flags = Qt::MatchFlags( Qt::MatchStartsWith | Qt::MatchWrap )) const;

private:
    Q_DECLARE_PRIVATE(ContactsModel)
    ContactsModelPrivate *d_ptr;
};

#endif
