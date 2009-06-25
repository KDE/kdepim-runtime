/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "notificationmodel.h"
#include "notificationmanagerinterface.h"

#include <akonadi/private/imapparser_p.h>

#include <KDebug>
#include <KGlobal>
#include <KLocale>
#include <QDBusConnection>

using Akonadi::NotificationMessage;

NotificationModel::NotificationBlock::NotificationBlock( const NotificationMessage::List &_msgs ) :
  msgs( _msgs )
{
  timestamp = QDateTime::currentDateTime();
}

NotificationModel::NotificationModel(QObject* parent) :
  QAbstractItemModel( parent ),
  m_enabled( true )
{
  NotificationMessage::registerDBusTypes();

  org::freedesktop::Akonadi::NotificationManager* nm
    = new org::freedesktop::Akonadi::NotificationManager( QLatin1String( "org.freedesktop.Akonadi" ),
                                                          QLatin1String( "/notifications" ),
                                                          QDBusConnection::sessionBus(), this );
  if ( !nm ) {
    kWarning( 5250 ) << "Unable to connect to notification manager";
  } else {
    connect( nm, SIGNAL(notify(Akonadi::NotificationMessage::List)), this, SLOT(slotNotify(Akonadi::NotificationMessage::List)) );
  }
}

int NotificationModel::columnCount(const QModelIndex& parent) const
{
  Q_UNUSED( parent );
  return 10;
}

int NotificationModel::rowCount(const QModelIndex& parent) const
{
  if ( !parent.isValid() )
    return m_data.size();
  if ( parent.internalId() >= 0 )
    return 0;
  if ( parent.row() < m_data.size() ) {
    const NotificationBlock block = m_data.at( parent.row() );
    return block.msgs.count();
  }
  return 0;
}

QModelIndex NotificationModel::index(int row, int column, const QModelIndex& parent) const
{
  if ( !parent.isValid() ) {
    if ( row < 0 || row >= m_data.size() )
      return QModelIndex();
    return createIndex( row, column, -1 );
  }

  if ( parent.row() < 0 || parent.row() >= m_data.size() )
    return QModelIndex();
  const NotificationBlock block = m_data.at( parent.row() );
  if ( row >= block.msgs.size() )
    return QModelIndex();
  return createIndex( row, column, parent.row() );
}

QModelIndex NotificationModel::parent(const QModelIndex& child) const
{
  if ( !child.isValid() || child.internalId() < 0 || child.internalId() > m_data.count() )
    return QModelIndex();
  return createIndex( child.internalId(), 0, -1 );
}

QVariant NotificationModel::data(const QModelIndex& index, int role) const
{
  if ( index.parent().isValid() ) {
    if ( index.parent().row() < 0 || index.parent().row() >= m_data.size() )
      return QVariant();
    const NotificationBlock block = m_data.at( index.parent().row() );
    if ( index.row() < 0 || index.row() >= block.msgs.size() )
      return QVariant();
    const NotificationMessage msg = block.msgs.at( index.row() );
    if ( role == Qt::DisplayRole ) {
      switch ( index.column() ) {
        case 0:
        {
          switch ( msg.operation() ) {
            case NotificationMessage::Add: return QString( "Add" );
            case NotificationMessage::Modify: return QString( "Modify" );
            case NotificationMessage::Move: return QString( "Move" );
            case NotificationMessage::Remove: return QString( "Delete" );
            default: return QString( "Invalid" );
          }
        }
        case 1:
        {
          switch ( msg.type() ) {
            case NotificationMessage::Collection: return QString( "Collection" );
            case NotificationMessage::Item: return QString( "Item" );
            default: return QString( "Invalid" );
          }
        }
        case 2:
          return QString::number( msg.uid() );
        case 3:
          return msg.remoteId();
        case 4:
          return QString::fromUtf8( msg.sessionId() );
        case 5:
          return QString::fromUtf8( msg.resource() );
        case 6:
          return msg.mimeType();
        case 7:
          return QString::number( msg.parentCollection() );
        case 8:
          return QString::number( msg.parentDestCollection() );
        case 9:
          return QString::fromUtf8( Akonadi::ImapParser::join( msg.itemParts(), ", " ) );
      }
    }
  } else {
    if ( index.row() < 0 || index.row() >= m_data.size() )
      return QVariant();
    const NotificationBlock block = m_data.at( index.row() );
    if ( role == Qt::DisplayRole ) {
      if ( index.column() == 0 ) {
        return KGlobal::locale()->formatTime( block.timestamp.time(), true )
          + QString::fromLatin1( ".%1" ).arg( block.timestamp.time().msec(), 3, 10, QLatin1Char('0') );
      } else if ( index.column() == 1 ) {
        return block.msgs.size();
      }
    }
  }
  return QVariant();
}

QVariant NotificationModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if ( role == Qt::DisplayRole && orientation == Qt::Horizontal ) {
    switch ( section ) {
      case 0: return QString( "Operation" );
      case 1: return QString( "Type" );
      case 2: return QString( "UID" );
      case 3: return QString( "RID" );
      case 4: return QString( "Session" );
      case 5: return QString( "Resource" );
      case 6: return QString( "Mimetype" );
      case 7: return QString( "Parent" );
      case 8: return QString( "Destination" );
      case 9: return QString( "Parts" );
    }
  }
  return QAbstractItemModel::headerData(section, orientation, role);
}


void NotificationModel::slotNotify(const Akonadi::NotificationMessage::List& msgs)
{
  if ( !m_enabled )
    return;
  beginInsertRows( QModelIndex(), m_data.size(), m_data.size() );
  m_data.append( NotificationBlock( msgs ) );
  endInsertRows();
}

void NotificationModel::clear()
{
  m_data.clear();
  reset();
}


#include "notificationmodel.moc"




