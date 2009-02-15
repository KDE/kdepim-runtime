/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "blogmodel.h"
#include "../statusitem.h"

#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>
#include <akonadi/session.h>

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kio/job.h>

#include <QtCore/QDebug>

using namespace Akonadi;

class Akonadi::BlogModel::Private
{
  public:
};

BlogModel::BlogModel( QObject *parent ) :
    ItemModel( parent ),
    d( new Private() )
{
    fetchScope().fetchFullPayload();
}

BlogModel::~BlogModel( )
{
  delete d;
}

int BlogModel::columnCount( const QModelIndex & parent ) const
{
  if ( !parent.isValid() )
    return 3; // keep in sync with the column type enum

  return 0;
}

QVariant BlogModel::data( const QModelIndex & index, int role ) const
{
  if ( role != Qt::DisplayRole ) 
      return QVariant(); 
  if ( !index.isValid() )
    return QVariant();
  if ( index.row() >= rowCount() )
    return QVariant();
  Item item = itemForIndex( index );
  kDebug() << item.hasPayload<StatusItem>();
  if ( !item.hasPayload<StatusItem>() )
    return QVariant();
  StatusItem msg = item.payload<StatusItem>();
//  kDebug() << index.column() <<  msg.user() <<  msg.text() << msg.date();
  switch ( index.column() ) {
      case User:
        kDebug() <<  msg.user();
        return msg.user();
      case Text:
        kDebug() <<  msg.text();
        return msg.text();
      case Date:
        kDebug() << msg.date();
        return msg.date();
      default:
        return QVariant();
    }
  return ItemModel::data( index, role );
}

QVariant BlogModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
    switch ( section ) {
      case User:
        return i18nc( "@title:column, message (e.g. email) subject", "User" );
      case Text:
        return i18nc( "@title:column, sender of message (e.g. email)", "Text" );
      case Date:
        return i18nc( "@title:column, message (e.g. email) timestamp", "Date" );
      default:
        return QString();
    }
  }
  return ItemModel::headerData( section, orientation, role );
}

#include "blogmodel.moc"
