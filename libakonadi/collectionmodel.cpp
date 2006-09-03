/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include "collection.h"
#include "collectioncreatejob.h"
#include "collectionstatusjob.h"
#include "collectionlistjob.h"
#include "collectionmodel.h"
#include "collectionrenamejob.h"
#include "jobqueue.h"
#include "monitor.h"

#include <kdebug.h>
#include <kiconloader.h>
#include <kinstance.h>
#include <klocale.h>

#include <QDebug>
#include <QHash>
#include <QPixmap>
#include <QQueue>

using namespace PIM;

class CollectionModel::Private
{
  public:
    enum EditType { None, Rename, Create, Delete };
    QHash<QByteArray, Collection*> collections;
    QHash<QByteArray, QList<QByteArray> > childCollections;
    EditType currentEdit;
    Collection *editedCollection;
    QString editOldName;
    Monitor* monitor;
    JobQueue *queue;
};

PIM::CollectionModel::CollectionModel( QObject * parent ) :
    QAbstractItemModel( parent ),
    d( new Private() )
{
  d->currentEdit = Private::None;
  d->editedCollection = 0;

  d->queue = new JobQueue( this );

  // start a list job
  CollectionListJob *job = new CollectionListJob( Collection::prefix(), true, d->queue );
  connect( job, SIGNAL(done(PIM::Job*)), SLOT(listDone(PIM::Job*)) );
  d->queue->addJob( job );

  // monitor collection changes
  d->monitor = new Monitor();
  d->monitor->monitorCollection( Collection::root(), true );
  connect( d->monitor, SIGNAL(collectionChanged(QByteArray)), SLOT(collectionChanged(QByteArray)) );
  connect( d->monitor, SIGNAL(collectionAdded(QByteArray)), SLOT(collectionChanged(QByteArray)) );
  connect( d->monitor, SIGNAL(collectionRemoved(QByteArray)), SLOT(collectionRemoved(QByteArray)) );

  // ### Hack to get the kmail resource folder icons
  KGlobal::instance()->iconLoader()->addAppDir( "kmail" );
}

PIM::CollectionModel::~CollectionModel()
{
  d->childCollections.clear();
  qDeleteAll( d->collections );
  d->collections.clear();

  delete d->monitor;
  d->monitor = 0;

  delete d;
  d = 0;
}

int PIM::CollectionModel::columnCount( const QModelIndex & parent ) const
{
  Q_UNUSED( parent );
  return 1;
}

QVariant PIM::CollectionModel::data( const QModelIndex & index, int role ) const
{
  Collection *col = static_cast<Collection*>( index.internalPointer() );
  if ( index.column() == 0 && (role == Qt::DisplayRole || role == Qt::EditRole) ) {
    return col->name();
  }
  if ( index.column() == 0 && role == Qt::DecorationRole ) {
    if ( col->type() == Collection::Resource )
      return SmallIcon( "server" );
    if ( col->type() == Collection::VirtualParent )
      return SmallIcon( "find" );
    if ( col->type() == Collection::Virtual )
      return SmallIcon( "folder_green" );
    QList<QByteArray> content = col->contentTypes();
    if ( content.size() == 1 || (content.size() == 2 && content.contains( Collection::collectionMimeType() )) ) {
      if ( content.contains( "text/x-vcard" ) || content.contains( "text/vcard" ) )
        return SmallIcon( "kmgroupware_folder_contacts" );
      // TODO: add all other content types and/or fix their mimetypes
      if ( content.contains( "akonadi/event" ) || content.contains( "text/ical" ) )
        return SmallIcon( "kmgroupware_folder_calendar" );
      if ( content.contains( "akonadi/task" ) )
        return SmallIcon( "kmgroupware_folder_tasks" );
      return SmallIcon( "folder" );
    } else if ( content.isEmpty() ) {
      return SmallIcon( "folder_grey" );
    } else
      return SmallIcon( "folder_orange" ); // mixed stuff
  }
  return QVariant();
}

QModelIndex PIM::CollectionModel::index( int row, int column, const QModelIndex & parent ) const
{
  QList<QByteArray> list;
  if ( !parent.isValid() )
    list = d->childCollections.value( Collection::root() );
  else
    list = d->childCollections.value( static_cast<Collection*>( parent.internalPointer() )->path() );

  if ( row < 0 || row >= list.size() )
    return QModelIndex();
  if ( !d->collections.contains( list[row] ) )
    return QModelIndex();
  return createIndex( row, column, d->collections.value( list[row] ) );
}

QModelIndex PIM::CollectionModel::parent( const QModelIndex & index ) const
{
  if ( !index.isValid() )
    return QModelIndex();

  Collection *col = static_cast<Collection*>( index.internalPointer() );
  QByteArray parentPath = col->parent();
  Collection *parentCol = d->collections.value( parentPath );
  if ( !parentCol )
    return QModelIndex();

  QList<QByteArray> list;
  list = d->childCollections.value( parentCol->parent() );

  int parentRow = list.indexOf( parentPath );
  if ( parentRow < 0 )
    return QModelIndex();

  return createIndex( parentRow, 0, parentCol );
}

int PIM::CollectionModel::rowCount( const QModelIndex & parent ) const
{
  QList<QByteArray> list;
  if ( parent.isValid() )
    list = d->childCollections.value( static_cast<Collection*>( parent.internalPointer() )->path() );
  else
    list = d->childCollections.value( Collection::root() );

  return list.size();
}

QVariant PIM::CollectionModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole )
    return i18n( "Name" );
  return QAbstractItemModel::headerData( section, orientation, role );
}

bool PIM::CollectionModel::removeRows( int row, int count, const QModelIndex & parent )
{
  QList<QByteArray> list;
  QByteArray parentPath;
  if ( parent.isValid() ) {
    parentPath = static_cast<Collection*>( parent.internalPointer() )->path();
    list = d->childCollections.value( parentPath );
  } else
    list = d->childCollections.value( Collection::root() );
  if ( row < 0 || ( row + count - 1 ) >= list.size() ) {
    kWarning() << k_funcinfo << "Index out of bounds: " << row << "-" << row+count << " parent: " << parentPath << endl;
    return false;
  }

  QList<QByteArray> removeList = list.mid( row, count );
  beginRemoveRows( parent, row, row + count - 1 ); // FIXME row +/- 1??, crashs sort proxy model!
  foreach ( QByteArray path, removeList ) {
    list.removeAll( path );
    delete d->collections.take( path );
  }
  d->childCollections.insert( parentPath, list );
  endRemoveRows();
  return true;
}

void PIM::CollectionModel::collectionChanged( const QByteArray &path )
{
  if ( d->collections.contains( path ) ) {
    // update
    CollectionStatusJob *job = new CollectionStatusJob( path, d->queue );
    connect( job, SIGNAL(done(PIM::Job*)), SLOT(updateDone(PIM::Job*)) );
    d->queue->addJob( job );
  } else {
    // new collection
    int index = path.lastIndexOf( Collection::delimiter() );
    QByteArray parent;
    if ( index > 0 )
      parent = path.left( index );
    else
      parent = Collection::prefix();

    qDebug() << "Collection " << path << " changed - re-listing parent: " << parent;
    CollectionListJob *job = new CollectionListJob( parent, false, d->queue );
    connect( job, SIGNAL(done(PIM::Job*)), SLOT(listDone(PIM::Job*)) );
    d->queue->addJob( job );
  }
}

void PIM::CollectionModel::collectionRemoved( const QByteArray &path )
{
  QModelIndex colIndex = indexForPath( path );
  if ( colIndex.isValid() ) {
    QModelIndex parentIndex = parent( colIndex );
    // collection is still somewhere in the hierarchy
    removeRow( colIndex.row(), parentIndex );
  } else {
    if ( d->collections.contains( path ) )
      // collection is orphan, ie. the parent has been removed already
      delete d->collections.take( path );
  }
}

void PIM::CollectionModel::updateDone( PIM::Job * job )
{
  if ( job->error() ) {
    // TODO: handle job errors
    kWarning() << k_funcinfo << "Job error: " << job->errorText() << endl;
  } else {
    CollectionStatusJob *csjob = static_cast<CollectionStatusJob*>( job );
    QByteArray path = csjob->path();
    if ( !d->collections.contains( path ) )
      kWarning() << k_funcinfo << "Got status response for non-existing collection: " << path << endl;
    else {
      Collection *col = d->collections.value( path );
      foreach ( CollectionAttribute* attr, csjob->attributes() )
        col->addAttribute( attr );

      QModelIndex startIndex = indexForPath( path );
      QModelIndex endIndex = indexForPath( path, columnCount( parent( startIndex ) ) );
      emit dataChanged( startIndex, endIndex );
    }
  }

  job->deleteLater();
}

QModelIndex PIM::CollectionModel::indexForPath( const QByteArray &path, int column )
{
  if ( !d->collections.contains( path ) )
    return QModelIndex();

  QByteArray parentPath = d->collections.value( path )->parent();
  // check if parent still exist or if this is an orphan collection
  if ( parentPath != Collection::root() && !d->collections.contains( parentPath ) )
    return QModelIndex();

  QList<QByteArray> list = d->childCollections.value( parentPath );
  int row = list.indexOf( path );

  if ( row >= 0 )
    return createIndex( row, column, d->collections.value( list[row] ) );
  return QModelIndex();
}

void PIM::CollectionModel::listDone( PIM::Job * job )
{
  if ( job->error() ) {
    qWarning() << "Job error: " << job->errorText() << endl;
  } else {
    Collection::List collections = static_cast<CollectionListJob*>( job )->collections();

    // update model
    foreach( Collection* col, collections ) {
      if ( d->collections.contains( col->path() ) ) {
        // collection already known
        continue;
      }
      d->collections.insert( col->path(), col );
      QModelIndex parentIndex = indexForPath( col->parent() );
      if ( parentIndex.isValid() || col->parent() == Collection::root() ) {
        beginInsertRows( parentIndex, rowCount( parentIndex ), rowCount( parentIndex ) );
        d->childCollections[ col->parent() ].append( col->path() );
        endInsertRows();
      } else {
        d->childCollections[ col->parent() ].append( col->path() );
      }

      // start a status job for every collection to get message counts, etc.
      if ( col->type() != Collection::VirtualParent ) {
        CollectionStatusJob* csjob = new CollectionStatusJob( col->path(), d->queue );
        connect( csjob, SIGNAL(done(PIM::Job*)), SLOT(updateDone(PIM::Job*)) );
        d->queue->addJob( csjob );
      }
    }

  }
  job->deleteLater();
}

bool PIM::CollectionModel::setData( const QModelIndex & index, const QVariant & value, int role )
{
  if ( d->currentEdit != Private::None )
    return false;
  if ( index.column() == 0 && role == Qt::EditRole ) {
    // rename collection
    d->editedCollection = static_cast<Collection*>( index.internalPointer() );
    d->currentEdit = Private::Rename;
    d->editOldName = d->editedCollection->name();
    d->editedCollection->setName( value.toString() );
    QByteArray newPath;
    if ( d->editedCollection->parent() == Collection::root() )
      newPath = d->editedCollection->name().toLatin1(); // TODO: to utf7
    else
      newPath = d->editedCollection->parent() + Collection::delimiter() + d->editedCollection->name().toLatin1(); // TODO: to utf7
    CollectionRenameJob *job = new CollectionRenameJob( d->editedCollection->path(), newPath, d->queue );
    connect( job, SIGNAL(done(PIM::Job*)), SLOT(editDone(PIM::Job*)) );
    d->queue->addJob( job );
    emit dataChanged( index, index );
    return true;
  }
  return QAbstractItemModel::setData( index, value, role );
}

Qt::ItemFlags PIM::CollectionModel::flags( const QModelIndex & index ) const
{
  if ( index.column() == 0 ) {
    Collection *col = static_cast<Collection*>( index.internalPointer() );
    if ( col->type() != Collection::VirtualParent && d->currentEdit == Private::None )
      return QAbstractItemModel::flags( index ) | Qt::ItemIsEditable;
  }
  return QAbstractItemModel::flags( index );
}

void PIM::CollectionModel::editDone( PIM::Job * job )
{
  if ( job->error() ) {
    qWarning() << "Edit failed: " << job->errorText() << " - reverting current transaction";
    // revert current transaction
    switch ( d->currentEdit ) {
      case Private::Create:
      {
        QModelIndex index = indexForPath( d->editedCollection->path() );
        QModelIndex parent = indexForPath( d->editedCollection->parent() );
        removeRow( index.row(), parent );
        d->editedCollection = 0;
        break;
      }
      case Private::Rename:
        d->editedCollection->setName( d->editOldName );
        QModelIndex index = indexForPath( d->editedCollection->path() );
        emit dataChanged( index, index );
    }
    // TODO: revert current change
  } else {
    // transaction done
  }
  d->currentEdit = Private::None;
  job->deleteLater();
}

bool PIM::CollectionModel::createCollection( const QModelIndex & parent, const QString & name )
{
  if ( !canCreateCollection( parent ) )
    return false;
  Collection *parentCol = static_cast<Collection*>( parent.internalPointer() );

  // create temporary fake collection, will be removed on error
  d->editedCollection = new Collection( parentCol->path() + Collection::delimiter() + name.toLatin1() ); // FIXME utf-7 encoding
  d->editedCollection->setParent( parentCol->path() );
  if ( d->collections.contains( d->editedCollection->path() ) ) {
    delete d->editedCollection;
    d->editedCollection = 0;
    return false;
  }
  d->collections.insert( d->editedCollection->path(), d->editedCollection );
  beginInsertRows( parent, rowCount( parent ), rowCount( parent ) );
  d->childCollections[ d->editedCollection->parent() ].append( d->editedCollection->path() );
  endInsertRows();

  // start creation job
  CollectionCreateJob *job = new CollectionCreateJob( d->editedCollection->path(), d->queue );
  connect( job, SIGNAL(done(PIM::Job*)), SLOT(editDone(PIM::Job*)) );
  d->queue->addJob( job );

  d->currentEdit = Private::Create;
  return true;
}

bool PIM::CollectionModel::canCreateCollection( const QModelIndex & parent ) const
{
  if ( d->currentEdit != Private::None )
    return false;
  if ( !parent.isValid() )
    return false; // FIXME: creation of top-level collections??

  Collection *col = static_cast<Collection*>( parent.internalPointer() );
  if ( col->type() == Collection::Virtual || col->type() == Collection::VirtualParent )
    return false;
  if ( !col->contentTypes().contains( Collection::collectionMimeType() ) )
    return false;

  return true;
}

QByteArray PIM::CollectionModel::pathForIndex( const QModelIndex & index ) const
{
  if ( index.isValid() )
    return static_cast<Collection*>( index.internalPointer() )->path();
  return QByteArray();
}

#include "collectionmodel.moc"
