/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

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

#include "agentinstancemodel.h"
#include "agentmanager.h"

#include <QtCore/QStringList>
#include <QtGui/QIcon>

#include <kdebug.h>
#include <klocale.h>

using namespace Akonadi;

class AgentInstanceInfo
{
  public:
    QString identifier;
    QString type;
    QString name;
    QString comment;
    QIcon icon;
    QStringList mimeTypes;
    QStringList capabilities;
    QString instanceName;
    AgentManager::Status status;
    QString statusMessage;
    uint progress;
    QString progressMessage;
    bool online;
};

class AgentInstanceModel::Private
{
  public:
    Private( AgentInstanceModel *parent )
      : mParent( parent )
    {
    }

    AgentInstanceModel *mParent;
    QList<AgentInstanceInfo> mInfos;
    AgentManager mManager;

    void agentInstanceAdded( const QString &agentInstance );
    void agentInstanceRemoved( const QString &agentInstance );
    void agentInstanceStatusChanged( const QString&, AgentManager::Status, const QString& );
    void agentInstanceProgressChanged( const QString&, uint, const QString& );
    void agentInstanceNameChanged( const QString&, const QString& );

    void addAgentInstance( const QString &agentInstance );
};

void AgentInstanceModel::Private::addAgentInstance( const QString &agentInstance )
{
  AgentInstanceInfo info;
  info.identifier = agentInstance;
  info.type = mManager.agentInstanceType( agentInstance );
  info.name = mManager.agentName( info.type );
  info.comment = mManager.agentComment( info.type );
  info.icon = mManager.agentIcon( info.type );
  info.mimeTypes = mManager.agentMimeTypes( info.type );
  info.capabilities = mManager.agentCapabilities( info.type );
  info.status = mManager.agentInstanceStatus( agentInstance );
  info.statusMessage = mManager.agentInstanceStatusMessage( agentInstance );
  info.progress = mManager.agentInstanceProgress( agentInstance );
  info.progressMessage = mManager.agentInstanceProgressMessage( agentInstance );
  info.instanceName = mManager.agentInstanceName( agentInstance );
  info.online = mManager.agentInstanceOnline( agentInstance );

  mInfos.append( info );
}

void AgentInstanceModel::Private::agentInstanceAdded( const QString &agentInstance )
{
  addAgentInstance( agentInstance );

  emit mParent->layoutChanged();
}

void AgentInstanceModel::Private::agentInstanceRemoved( const QString &agentInstance )
{
  for ( int i = 0; i < mInfos.count(); ++i ) {
    if ( mInfos[ i ].identifier == agentInstance ) {
      mInfos.removeAt( i );
      break;
    }
  }

  emit mParent->layoutChanged();
}

void AgentInstanceModel::Private::agentInstanceStatusChanged( const QString &agentInstance,
                                                              AgentManager::Status status,
                                                              const QString &message )
{
  for ( int i = 0; i < mInfos.count(); ++i ) {
    if ( mInfos[ i ].identifier == agentInstance ) {
      mInfos[ i ].status = status;
      mInfos[ i ].statusMessage = message;

      const QModelIndex idx = mParent->index( i, 0 );
      emit mParent->dataChanged( idx, idx );

      return;
    }
  }
}

void AgentInstanceModel::Private::agentInstanceProgressChanged( const QString &agentInstance,
                                                                uint progress,
                                                                const QString &message )
{
  for ( int i = 0; i < mInfos.count(); ++i ) {
    if ( mInfos[ i ].identifier == agentInstance ) {
      mInfos[ i ].progress = progress;
      mInfos[ i ].progressMessage = message;

      const QModelIndex idx = mParent->index( i, 0 );
      emit mParent->dataChanged( idx, idx );

      return;
    }
  }
}

void AgentInstanceModel::Private::agentInstanceNameChanged( const QString &agentInstance,
                                                              const QString &name )
{
  for ( int i = 0; i < mInfos.count(); ++i ) {
    if ( mInfos[ i ].identifier == agentInstance ) {
      mInfos[ i ].instanceName = name;

      const QModelIndex idx = mParent->index( i, 0 );
      emit mParent->dataChanged( idx, idx );

      return;
    }
  }
}

AgentInstanceModel::AgentInstanceModel( QObject *parent )
  : QAbstractItemModel( parent ), d( new Private( this ) )
{
  const QStringList agentInstances = d->mManager.agentInstances();
  for ( int i = 0; i < agentInstances.count(); ++i )
    d->addAgentInstance( agentInstances[ i ] );

  connect( &d->mManager, SIGNAL( agentInstanceAdded( const QString& ) ),
           this, SLOT( agentInstanceAdded( const QString& ) ) );
  connect( &d->mManager, SIGNAL( agentInstanceRemoved( const QString& ) ),
           this, SLOT( agentInstanceRemoved( const QString& ) ) );
  connect( &d->mManager, SIGNAL( agentInstanceStatusChanged( const QString&, AgentManager::Status, const QString& ) ),
           this, SLOT( agentInstanceStatusChanged( const QString&, AgentManager::Status, const QString& ) ) );
  connect( &d->mManager, SIGNAL( agentInstanceProgressChanged( const QString&, uint, const QString& ) ),
           this, SLOT( agentInstanceProgressChanged( const QString&, uint, const QString& ) ) );
  connect( &d->mManager, SIGNAL( agentInstanceNameChanged( const QString&, const QString& ) ),
           this, SLOT( agentInstanceNameChanged( const QString&, const QString& ) ) );
}

AgentInstanceModel::~AgentInstanceModel()
{
  delete d;
}

int AgentInstanceModel::columnCount( const QModelIndex& ) const
{
  return 1;
}

int AgentInstanceModel::rowCount( const QModelIndex& ) const
{
  return d->mInfos.count();
}

QVariant AgentInstanceModel::data( const QModelIndex &index, int role ) const
{
  if ( !index.isValid() )
    return QVariant();

  if ( index.row() < 0 || index.row() >= d->mInfos.count() )
    return QVariant();

  const AgentInstanceInfo info = d->mInfos[ index.row() ];

  switch ( role ) {
    case Qt::DisplayRole:
      return info.instanceName;
    case Qt::DecorationRole:
      return info.icon;
    case InstanceIdentifierRole:
      return info.identifier;
    case Qt::ToolTipRole:
      return QString::fromLatin1( "<qt><h4>%1</h4>%2</qt>" ).arg( info.name, info.comment );
    case StatusRole:
      return info.status;
    case StatusMessageRole:
      return info.statusMessage;
    case ProgressRole:
      return info.progress;
    case ProgressMessageRole:
      return info.progressMessage;
    case OnlineRole:
      return info.online;
    case CapabilitiesRole:
      return info.capabilities;
    case TypeIdentifierRole:
      return info.type;
    case MimeTypesRole:
      return info.mimeTypes;
  }
  return QVariant();
}

QVariant AgentInstanceModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( orientation == Qt::Vertical )
    return QVariant();

  if ( role != Qt::DisplayRole )
    return QVariant();

  switch ( section ) {
    case 0:
      return i18nc( "@title:column, name of a thing", "Name" );
      break;
    default:
      return QVariant();
      break;
  }
}

QModelIndex AgentInstanceModel::index( int row, int column, const QModelIndex& ) const
{
  if ( row < 0 || row >= d->mInfos.count() )
    return QModelIndex();

  if ( column != 0 )
    return QModelIndex();

  return createIndex( row, column, 0 );
}

QModelIndex AgentInstanceModel::parent( const QModelIndex& ) const
{
  return QModelIndex();
}

Qt::ItemFlags AgentInstanceModel::flags(const QModelIndex & index) const
{
  if ( !index.isValid() || index.row() < 0 || index.row() >= d->mInfos.count() )
    return QAbstractItemModel::flags( index );

  return QAbstractItemModel::flags( index ) | Qt::ItemIsEditable;
}

bool AgentInstanceModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
  if ( !index.isValid() )
    return false;

  if ( index.row() < 0 || index.row() >= d->mInfos.count() )
    return false;

  AgentInstanceInfo info = d->mInfos[ index.row() ];

  switch ( role ) {
    case OnlineRole:
      d->mManager.setAgentInstanceOnline( info.identifier, value.toBool() );
      info.online = d->mManager.agentInstanceOnline( info.identifier );
      emit dataChanged( index, index );
      return true;
    default:
      return false;
  }

  return false;
}

#include "agentinstancemodel.moc"
