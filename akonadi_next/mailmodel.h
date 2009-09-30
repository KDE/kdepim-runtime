/*
 * This file is part of the Akonadi Mail example.
 *
 * Copyright 2009  Stephen Kelly <steveire@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef MAILMODEL_H
#define MAILMODEL_H

#include <akonadi/entitytreemodel.h>

#include "akonadi_next_export.h"

using namespace Akonadi;

class MailModelPrivate;

class AKONADI_NEXT_EXPORT MailModel : public EntityTreeModel
{
  Q_OBJECT
public:

  MailModel(Session *session, ChangeRecorder *monitor, QObject *parent = 0);
  virtual ~MailModel();

  virtual QVariant getData(const Item &item, int column, int role=Qt::DisplayRole) const;

  virtual QVariant getData(const Collection &collection, int column, int role=Qt::DisplayRole) const;

  virtual int columnCount(const QModelIndex &index = QModelIndex()) const;

  virtual QVariant getHeaderData( int section, Qt::Orientation orientation, int role, int headerSet ) const;

private:
    Q_DECLARE_PRIVATE(MailModel)
    MailModelPrivate *d_ptr;
};

#endif
