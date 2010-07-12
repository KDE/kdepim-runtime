/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Stephen Kelly <stephen@kdab.com>

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

#ifndef INCIDENCEATTACHMENTMODEL_H
#define INCIDENCEATTACHMENTMODEL_H

#include <QtCore/QAbstractListModel>

#include "akonadi_next_export.h"
#include <akonadi/attribute.h>
#include <kcalcore/incidence.h>
#include <akonadi/item.h>

class QAbstractItemModel;

namespace Akonadi
{
class IncidenceAttachmentModelPrivate;
class Item;

class AKONADI_NEXT_EXPORT IncidenceAttachmentModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY( int attachmentCount READ rowCount NOTIFY rowCountChanged )

public:
  enum Roles
  {
    AttachmentDataRole = Qt::UserRole,
    MimeTypeRole,
    AttachmentUrl,
    AttachmentCountRole,

    UserRole = Qt::UserRole + 100
  };

  IncidenceAttachmentModel( const QPersistentModelIndex &modelIndex, QObject* parent = 0);
  IncidenceAttachmentModel( const Akonadi::Item &item, QObject* parent = 0);
  IncidenceAttachmentModel( QObject* parent = 0);

  KCalCore::Incidence::Ptr incidence() const;

  void setItem( const Akonadi::Item &item );
  void setIndex( const QPersistentModelIndex &modelIndex );

  /** @reimp */
  int rowCount( const QModelIndex& parent = QModelIndex() ) const;

  /** @reimp */
  QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;

  /** @reimp */
  QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

signals:
  void rowCountChanged();

private:
  Q_DECLARE_PRIVATE( IncidenceAttachmentModel )
  IncidenceAttachmentModelPrivate * const d_ptr;

  Q_PRIVATE_SLOT( d_func(), void resetModel() )
  Q_PRIVATE_SLOT( d_func(), void itemFetched(Akonadi::Item::List) )
};

}

#endif
