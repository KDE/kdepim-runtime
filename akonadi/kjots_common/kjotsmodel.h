/*
    This file is part of KJots.

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

#ifndef KJOTSMODEL_H
#define KJOTSMODEL_H

#include "entitytreemodel.h"

namespace Akonadi
{
class Monitor;
class Session;
}

using namespace Akonadi;

/**
 * A wrapper QObject making some book and page properties available to Grantlee.
 */
class KJotsEntity : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString title READ title)
  Q_PROPERTY(QString content READ content)
  Q_PROPERTY(bool isBook READ isBook)
  Q_PROPERTY(bool isPage READ isPage)
  Q_PROPERTY(QVariantList entities READ entities)

public:
  KJotsEntity(const QModelIndex &index, QObject *parent = 0);

  bool isBook();
  bool isPage();

  QString title();

  QString content();

  QVariantList entities();

private:
  QPersistentModelIndex m_index;
};

class KJotsModel : public EntityTreeModel
{
  Q_OBJECT
public:
  KJotsModel(Session *session, Monitor *monitor, QObject *parent = 0);
  virtual ~KJotsModel();

  enum KJotsRoles
  {
    GrantleeObjectRole = EntityTreeModel::UserRole
  };

  QVariant data(const QModelIndex &index, int role) const;
};

#endif

