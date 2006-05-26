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
#include "collectionfetchjob.h"
#include "collectionmodel.h"
#include "monitor.h"

#include <kdebug.h>
#include <kiconloader.h>
#include <kinstance.h>
#include <klocale.h>

#include <QHash>
#include <QPixmap>
#include <QQueue>

using namespace PIM;

class CollectionModel::Private
{
  public:
    QHash<QByteArray, Collection*> collections;
    QHash<QByteArray, QList<QByteArray> > childCollections;
    Monitor* monitor;
    CollectionFetchJob *fetchJob;
    QQueue<DataReference> updateQueue;
};

PIM::CollectionModel::CollectionModel( const Collection::List &collections,
                                       QObject * parent ) :
    QAbstractItemModel( parent ),
    d( new Private() )
{
  d->fetchJob = 0;

  // build uid hash
  foreach( Collection *col, collections )
      d->collections.insert( col->path(), col );
  // build child collection hash
  foreach ( const Collection *col, d->collections )
    d->childCollections[ col->parent() ].append( col->path() );

  // monitor collection changes
  // TODO: handle errors
  // FIXME: collections don't have uids anymore!
  /*d->monitor = new Monitor( "mimetype=x-akonadi/collection" );
  connect( d->monitor, SIGNAL( changed( const DataReference::List& ) ),
           SLOT( collectionChanged( const DataReference::List& ) ) );
  connect( d->monitor, SIGNAL( added( const DataReference::List& ) ),
           SLOT( collectionChanged( const DataReference::List& ) ) );
  connect( d->monitor, SIGNAL( removed( const DataReference& ) ),
           SLOT( collectionRemoved( const DataReference::List& ) ) );
  d->monitor->start();*/

  // ### Hack to get the kmail resource folder icons
  KGlobal::instance()->iconLoader()->addAppDir( "kmail" );
}

PIM::CollectionModel::~CollectionModel()
{
  d->childCollections.clear();
  qDeleteAll( d->collections );
  d->collections.clear();

  // FIXME
/*  delete d->monitor;
  d->monitor = 0;
  delete d->fetchJob;
  d->fetchJob = 0;*/

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
  if ( index.column() == 0 && role == Qt::DisplayRole ) {
    return col->name();
  }
  if ( index.column() == 0 && role == Qt::DecorationRole ) {
    if ( col->type() == Collection::Resource )
      return SmallIcon( "server" );
    QStringList content = col->contentTypes();
    if ( content.size() == 1 ) {
      if ( content.first() == "text/x-vcard" )
        return SmallIcon( "kmgroupware_folder_contacts" );
      // TODO: add all other content types and/or fix their mimetypes
      if ( content.first() == "akonadi/event" )
        return SmallIcon( "kmgroupware_folder_calendar" );
      if ( content.first() == "akonadi/task" )
        return SmallIcon( "kmgroupware_folder_tasks" );
    } else if ( content.isEmpty() )
      return SmallIcon( "folder_grey" );
    return SmallIcon( "folder" );
  }
  return QVariant();
}

QModelIndex PIM::CollectionModel::index( int row, int column, const QModelIndex & parent ) const
{
  QList<QByteArray> list;
  if ( !parent.isValid() )
    list = d->childCollections.value( /*DataReference()*/ "/" ); // ### FIXME
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
    list = d->childCollections.value( /*DataReference()*/ "/" ); // ### FIXME

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
    list = d->childCollections.value( /*DataReference()*/ "/" ); // ### FIXME
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

// FIXME
void PIM::CollectionModel::collectionChanged( const DataReference::List& references )
{
  // TODO no need to serialize fetch jobs...
  foreach ( DataReference ref, references )
    d->updateQueue.enqueue( ref );
  processUpdates();
}

// FIXME
void PIM::CollectionModel::collectionRemoved( const DataReference::List& references )
{
  foreach ( DataReference ref, references ) {
    collectionRemoved( ref );
  }
}

// FIXME
void PIM::CollectionModel::collectionRemoved( const DataReference & ref )
{
/* QModelIndex colIndex = indexForReference( ref );
  if ( colIndex.isValid() ) {
    QModelIndex parentIndex = parent( colIndex );
    // collection is still somewhere in the hierarchy
    removeRow( colIndex.row(), parentIndex );
  } else {
    if ( d->collections.contains( ref ) )
      // collection is orphan, ie. the parent has been removed already
      delete d->collections.take( ref );
  }*/
}

// FIXME
void PIM::CollectionModel::fetchDone( Job * job )
{
/*  if ( job->error() || !d->fetchJob->collection() ) {
    // TODO: handle job errors
    kWarning() << k_funcinfo << "Job error or collection not available!" << endl;
  } else {
    Collection *col = d->fetchJob->collection();
    Collection *oldCol = d->collections.value( col->reference() );

    // update existing collection
    if ( oldCol ) {
      if ( oldCol->parent() == col->parent() ) {
        // TODO multi-column support
        QModelIndex oldIndex = indexForReference( oldCol->reference() );
        d->collections.insert( col->reference(), col );
        QModelIndex index = indexForReference( col->reference() );
        changePersistentIndex( oldIndex, index );
        emit dataChanged( index, index );
        delete oldCol;
      } else {
        // parent changed, ie. we need to remove and add
        collectionRemoved( col->reference() );
        oldCol = 0; // deleted above
      }
    }

    // add new collection
    if ( !oldCol && col ) {
      d->collections.insert( col->reference(), col );
      QModelIndex parentIndex = indexForReference( col->parent() );
      beginInsertRows( parentIndex, rowCount( parentIndex ), rowCount( parentIndex ) );
      d->childCollections[ col->parent() ].append( col->reference() );
      endInsertRows();
    }

  }

  job->deleteLater();
  d->fetchJob = 0;
  processUpdates();*/
}

void PIM::CollectionModel::processUpdates()
{
/*  if ( d->fetchJob )
    return;
  if ( d->updateQueue.isEmpty() )
    return;

  d->fetchJob = new CollectionFetchJob( d->updateQueue.dequeue() );
  connect( d->fetchJob, SIGNAL( done( PIM::Job* ) ), SLOT( fetchDone( PIM::Job* ) ) );
  d->fetchJob->start();*/
}

QModelIndex PIM::CollectionModel::indexForPath( const QByteArray &path, int column )
{
  if ( !d->collections.contains( path ) )
    return QModelIndex();

  QByteArray parentPath = d->collections.value( path )->parent();
  // check if parent still exist or if this is an orphan collection
  if ( !parentPath.isNull() && !d->collections.contains( parentPath ) )
    return QModelIndex();

  QList<QByteArray> list = d->childCollections.value( parentPath );
  int row = list.indexOf( path );

  if ( row >= 0 )
    return createIndex( row, column, d->collections.value( list[row] ) );
  return QModelIndex();
}

#include "collectionmodel.moc"
