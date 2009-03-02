/*
    This file is part of KJots.

    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

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

#include "kjotsresource.h"

#include <QtCore/QDir>
#include <QtDBus/QDBusConnection>
#include <QtXml/QDomDocument>

#include <akonadi/changerecorder.h>
#include <akonadi/attributefactory.h>
#include <akonadi/entitydisplayattribute.h>
#include "collectionchildorderattribute.h"
#include <akonadi/itemfetchscope.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemfetchjob.h>

#include <KUrl>
#include <KStandardDirs>
#include <kfiledialog.h>
#include <klocalizedstring.h>
#include <KTemporaryFile>

#include <kdebug.h>

#include "kjotspage.h"
#include "kjotsakonadipage.h"
#include "kjotsbook.h"

#include "datadir.h"

using namespace Akonadi;

QString KJotsResource::findParent( const QString &remoteId )
{
  QHashIterator<QString, QStringList> i( m_parentBook );
  while ( i.hasNext() ) {
    i.next();
    if ( i.value().contains( remoteId ) ) {
      return i.key();
    }
  }
  return QString();
}

KJotsResource::KJotsResource( const QString &id )
    : ResourceBase( id )
{
  kDebug() << DATADIR;
  changeRecorder()->fetchCollection( true );
  changeRecorder()->itemFetchScope().fetchFullPayload();
  changeRecorder()->itemFetchScope().fetchAttribute<CollectionChildOrderAttribute>();
  changeRecorder()->itemFetchScope().fetchAttribute<EntityDisplayAttribute>();

  // Temporary for development.
  m_rootDataPath = QString( DATADIR );
  //     m_rootDataPath = KStandardDirs::locateLocal( "data", "kjots/" );

  AttributeFactory::registerAttribute<CollectionChildOrderAttribute>();
}


KJotsResource::~KJotsResource()
{
}

void KJotsResource::retrieveCollections()
{
  kDebug();
  Collection::List list;

  // The bookshelf is not really a collection.
  KJotsBook book = KJotsBook::getBookshelf();
  book.setCollection( Collection::root() );
//   Collection::List colList = book.getCollections();

//   Collection col = book.getCollection();
//   col.setParent( Collection::root() );

//   No, that's something the resource should do.
//   Collection::List list = KJotsBook::getTopLevelCollections();



//   m_parentBook[ "Collection::root" ].append( col.remoteId() );

  Collection::List colList = book.getCollections( KJotsBook::ReadCollectionsRecursive );

  // parentRemoteId doesn't survive until we need it in removeCollection.
  // Persist it now.
  foreach( Collection col, colList ) {
    kDebug() << col.remoteId();
    m_parentBook[ col.parentRemoteId() ].append( col.remoteId() );
  }

//   list << col;
  list << colList;

  collectionsRetrieved( list );

}

void KJotsResource::retrieveItems( const Akonadi::Collection &collection )
{
  kDebug();
  QString colRId = collection.remoteId();
  KJotsBook book( colRId );
  Item::List items = book.retrieveItems();

  foreach( Item item, items ) {
    m_parentBook[ colRId ].append( item.remoteId() );
  }

  itemsRetrieved( items );
}

bool KJotsResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );

  Item newItem = item;
  itemRetrieved( newItem );
  return true;
}

void KJotsResource::aboutToQuit()
{
  // TODO: any cleanup you need to do while there is still an active
  // event loop. The resource will terminate after this method returns
}

void KJotsResource::configure( WId windowId )
{
  Q_UNUSED( windowId );

  // This resource doesn't need to be configured.

  synchronize();
}

void KJotsResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  kDebug() << collection.remoteId() << item.remoteId() << item.id();
  KJotsBook parent( collection.remoteId() );
  Item newItem = parent.addItem( item );
  m_parentBook[ collection.remoteId()].append( newItem.remoteId() );

  changeCommitted( newItem );
}

void KJotsResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  KJotsPage page = item.payload<KJotsPage>();

  // FIXME: page should already know its remoteId... ?
  page.setRemoteId(item.remoteId());

  if ( page.save() )
  {
    Item newItem = item;
    changeCommitted( newItem );
  }
}

void KJotsResource::itemRemoved( const Akonadi::Item &item )
{
  QString parentBookRemoteId = findParent( item.remoteId() );
  kDebug() << parentBookRemoteId << item.remoteId();
  KJotsBook book = KJotsBook( parentBookRemoteId );
  Item newItem = book.removeItem( item );
  m_parentBook[ parentBookRemoteId ].removeOne( item.remoteId() );

  changeCommitted( newItem );

}

void KJotsResource::itemMoved( const Akonadi::Item &item,
                               const Akonadi::Collection &source,
                               const Akonadi::Collection &destination )
{
  kDebug() << item.remoteId() << source.remoteId() << destination.remoteId() << destination.id();

  if ( source != destination )
  {
    KJotsBook sourceBook (source.remoteId() );
    KJotsBook destinationBook (destination.remoteId() );
    sourceBook.removeEntity( item.remoteId() );
    kDebug() << "removed";
    destinationBook.addEntity( item.remoteId() );
    kDebug() << "added";
    Item newItem = item;
    changeCommitted( newItem );
  }
  // Reordering is done on the collectionChanged signal?

}

void KJotsResource::collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent )
{
  kDebug() << collection.name() << collection.remoteId() << parent.remoteId();
  KJotsBook parentBook( parent.remoteId() );
  Collection newCollection = parentBook.addCollection( collection );
  newCollection.setParent( parent );

  changeCommitted( newCollection );

}

void KJotsResource::collectionRemoved( const Akonadi::Collection &collection )
{
  QString parentBookRemoteId = findParent( collection.remoteId() );
  kDebug() << collection.remoteId() << collection.parentRemoteId() << parentBookRemoteId;
//   QString parentBookRemoteId = collection.parentRemoteId();
  KJotsBook book = KJotsBook( parentBookRemoteId );
  Collection newCollection = book.removeCollection( collection );

  changeCommitted( newCollection );
}

void KJotsResource::collectionChanged( const Akonadi::Collection &collection )
{

  QString parentBookRemoteId = findParent( collection.remoteId() );

  kDebug() << collection.remoteId() << "in" << parentBookRemoteId << "changed.";

  KJotsBook book = KJotsBook( collection.remoteId() );

  Collection newCollection = book.updateCollection( collection );


  changeCommitted( newCollection );

}

void KJotsResource::collectionMoved( const Akonadi::Collection &collection,
                                     const Akonadi::Collection &source,
                                     const Akonadi::Collection &destination )
{
  kDebug() << collection.remoteId() << source.remoteId() << destination.remoteId() << destination.id();

  // Temporary for development.
  QString rootDataPath = QString( DATADIR );
  //     rootDataPath = KStandardDirs::locateLocal( "data", "kjots/" );

  if ( source != destination )
  {
    KJotsBook sourceBook (source.remoteId() );
    KJotsBook destinationBook (destination.remoteId() );
    sourceBook.removeEntity( collection.remoteId() );
    destinationBook.addEntity( collection.remoteId() );

    Collection newCollection = collection;
    changeCommitted( newCollection );
  }
  // Reordering is done on the collectionChanged signal?

}

AKONADI_RESOURCE_MAIN( KJotsResource )

#include "kjotsresource.moc"



