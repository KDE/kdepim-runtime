/*
    Copyright (c) 2009 Andras Mantia <amantia@kde.org>

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

#include "kolabproxyresource.h"

#include "settings.h"
#include "settingsadaptor.h"
#include "collectionannotationsattribute.h"
#include "addressbookhandler.h"
#include <akonadi/attributefactory.h>

#include <akonadi/collectioncreatejob.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/monitor.h>
#include <akonadi/item.h>
#include <akonadi/changerecorder.h>

#include <QtDBus/QDBusConnection>
#include <QSet>

using namespace Akonadi;

KolabProxyResource::KolabProxyResource( const QString &id )
  : ResourceBase( id )
{
  AttributeFactory::registerAttribute<CollectionAnnotationsAttribute>();

  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

  m_monitor = new Monitor();
  ItemFetchScope scope;
  scope.fetchFullPayload();
  m_monitor->setItemFetchScope(scope);

  m_colectionMonitor = new Monitor();
  m_colectionMonitor->setCollectionMonitored(Collection::root());

  connect(m_monitor, SIGNAL(itemAdded(const Item & , const Collection &)), this, SLOT(imapItemAdded(const Item & , const Collection &)));
  connect(m_monitor, SIGNAL(itemRemoved(const Item &)), this, SLOT(imapItemRemoved(const Item &)));
  connect(m_colectionMonitor, SIGNAL(collectionAdded(const Collection &, const Collection &)), this, SLOT(imapCollectionAdded(const Collection &, const Collection &)));
  connect(m_colectionMonitor, SIGNAL(collectionRemoved(const Collection &)), this, SLOT(imapCollectionRemoved(const Collection &)));
  connect(m_colectionMonitor, SIGNAL(collectionChanged(const Collection &)), this, SLOT(imapCollectionChanged(const Collection &)));

}

KolabProxyResource::~KolabProxyResource()
{
}

void KolabProxyResource::retrieveCollections()
{
  kDebug() << "RETRIEVECOLLECTIONS ";
  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
  if ( job->exec() ) {
    Collection::List collections = job->collections();
    foreach( const Collection &collection, collections ) {
      CollectionAnnotationsAttribute *annotationsAttribute =
          collection.attribute<CollectionAnnotationsAttribute>();
      if (annotationsAttribute) {
        QMap<QByteArray, QByteArray> annotations = annotationsAttribute->annotations();
        KolabHandler *handler = KolabHandler::createHandler(annotations["/vendor/kolab/folder-type"]);
        if (handler) {
          kDebug() << "Monitor folder: " << collection.name() << collection.remoteId();
          m_monitor->setCollectionMonitored(collection);
          m_monitoredCollections.insert(collection.id(), handler);
        }
      }
    }
  }

  Collection::List addrCollections;
  Collection::List imapCollections = m_monitor->collectionsMonitored();

  Q_FOREACH(Collection collection, imapCollections) {
    Collection c = createCollection(collection);
    collectionsRetrievedIncremental(Collection::List() << c, Collection::List());
  }
}

void KolabProxyResource::retrieveItems( const Collection &collection )
{
  kDebug() << "RETRIEVEITEMS";
  ItemFetchJob *job = new ItemFetchJob( Collection(collection.remoteId().toUInt()) );
  job->fetchScope().fetchFullPayload();
  if (job->exec()) {
    Item::List items = job->items();
    KolabHandler *handler = m_monitoredCollections.value(collection.remoteId().toUInt());
    if (handler) {
      Item::List newItems = handler->translateItems(items);
      itemsRetrieved(newItems);
    }
  }
}

bool KolabProxyResource::retrieveItem( const Item &item, const QSet<QByteArray> &parts )
{
  kDebug() << "RETRIEVEITEM";
  ItemFetchJob *job = new ItemFetchJob( item );
  job->fetchScope().fetchFullPayload();
  Item::Id collectionId = -1;
  if (job->exec()) {
    Item::List items = job->items();
    collectionId = items[0].collectionId();
    KolabHandler *handler = m_monitoredCollections.value(collectionId);
    if (handler) {
      Item::List newItems = handler->translateItems(items);
      itemsRetrieved(newItems);
    }
  }
  return true;
}

void KolabProxyResource::aboutToQuit()
{
  // TODO: any cleanup you need to do while there is still an active
  // event loop. The resource will terminate after this method returns
}

void KolabProxyResource::configure( WId windowId )
{
  Q_UNUSED( windowId );

  // TODO: this method is usually called when a new resource is being
  // added to the Akonadi setup. You can do any kind of user interaction here,
  // e.g. showing dialogs.
  // The given window ID is usually useful to get the correct
  // "on top of parent" behavior if the running window manager applies any kind
  // of focus stealing prevention technique
}

void KolabProxyResource::itemAdded( const Item &_item, const Collection &collection )
{

  Item item(_item);
//   kDebug() << "Item added " << item.id() << collection.remoteId() << collection.id();
  Collection c;
  CollectionFetchJob *coljob = new CollectionFetchJob( Collection::List() << collection );
  if ( coljob->exec() ) {
    Collection::List collections = coljob->collections();
    c = collections[0];
  }

  Collection imapCollection(c.remoteId().toUInt());
  coljob = new CollectionFetchJob( Collection::List() << imapCollection );
  if ( coljob->exec() ) {
    Collection::List collections = coljob->collections();
    imapCollection = collections[0];
  }else {
    kWarning() << "Can't fetch collection" << imapCollection.id();
  }

  Item addrItem;
  ItemFetchJob *job = new ItemFetchJob( item );
  job->fetchScope().fetchFullPayload();
  if (job->exec()) {
    Item::List items = job->items();
    if (items.isEmpty()) {
      kDebug() << "Empty fecth";
      return;
    }

    addrItem = items[0];
  } else {
    kWarning() << "Can't fetch address item " << item.id();
  }

  KolabHandler *handler  = m_monitoredCollections.value(imapCollection.id());
  if (!handler) {
    kWarning() << "No handler found";
    return;
  }
  Item imapItem = handler->toKolabFormat(addrItem);
  
  ItemCreateJob *cjob = new ItemCreateJob(imapItem, imapCollection);
  if (!cjob->exec()) {
    kWarning() << "Can't create imap item " << imapItem.id() << cjob->errorString();
  }

  imapItem = cjob->item();
  m_excludeAppend << imapItem.id();

  addrItem.setRemoteId(QString::number(imapItem.id()));
  changeCommitted( addrItem );
}

void KolabProxyResource::itemChanged( const Item &item, const QSet<QByteArray> &parts )
{
  Item addrItem;
  ItemFetchJob *job = new ItemFetchJob( item );
  job->fetchScope().fetchFullPayload();
  if (job->exec()) {
    Item::List items = job->items();
    addrItem = items[0];
  } else {
    kWarning() << "Can't fetch address item " << item.id();
  }

  Item imapItem;
  job = new ItemFetchJob( Item(addrItem.remoteId().toUInt()) );
  job->fetchScope().fetchFullPayload();
  if (job->exec()) {
    Item::List items = job->items();
    imapItem = items[0];
  } else {
    kWarning() << "Can't fetch imap item " << addrItem.remoteId();
  }

  KolabHandler *handler = m_monitoredCollections.value(imapItem.collectionId());
  if (!handler) {
    kWarning() << "No handler found";
    return;
  }
  
  imapItem = handler->toKolabFormat(addrItem);
  ItemModifyJob *mjob = new ItemModifyJob( imapItem );
    if (!mjob->exec()) {
      kWarning() << "Can't modify imap item " << imapItem.id();
    }

  changeCommitted( addrItem );

}

void KolabProxyResource::itemRemoved( const Item &item )
{
  kDebug() << "Item removed " << item.id() << item.remoteId();
  Item imapItem(item.remoteId().toUInt());
  ItemFetchJob *job = new ItemFetchJob( imapItem );
  if (job->exec()) {
    Item::List items = job->items();
    imapItem = items[0];
  } else {
    kWarning() << "Can't fetch imap item " << imapItem.id();
  }
  ItemDeleteJob *djob = new ItemDeleteJob( imapItem );
  changeCommitted( item );
  Q_UNUSED(djob);
}

void KolabProxyResource::imapItemAdded(const Item& item, const Collection &collection)
{
  kDebug() << "imapItemAdded " << item.id() << collection.id() << Collection::root().id();
  if (m_excludeAppend.contains(item.id()))   {
    kDebug() << "item already present";
    m_excludeAppend.removeAll(item.id());
    return;
  }
//TODO: slow, would be nice if ItemCreateJob would work with a Collection having only the remoteId set
  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
  connect(job, SIGNAL(result(KJob*)), this, SLOT(collectionFetchDone(KJob *)));
  m_ids[job] = QString::number(collection.id());
  m_items[job] = item;
}

void KolabProxyResource::collectionFetchDone(KJob *job)
{
  if ( job->error() ) {
    kWarning( ) << "Error on collection fetch:" << job->errorText();
  } else {
    Collection c;
    Collection::List collections = qobject_cast<CollectionFetchJob*>(job)->collections();
    foreach( const Collection &col, collections ) {
      if (col.remoteId() == m_ids[job]) {
        c = col;
        break;
      }
    }
    
    KolabHandler *handler = m_monitoredCollections.value(c.remoteId().toUInt());
    if (!handler) {
      kWarning() << "No handler found";
      return;
    }

    Item::List newItems = handler->translateItems(Item::List() << m_items[job]);
    if (!newItems.isEmpty()) {
      ItemCreateJob *cjob = new ItemCreateJob( newItems[0],  c );
      connect(cjob, SIGNAL(result(KJob*)), this, SLOT(itemCreatedDone(KJob *)));
    }
  }
  m_ids.remove(job);
  m_items.remove(job);
}

void KolabProxyResource::itemCreatedDone(KJob *job)
{
  if ( job->error() ) {
    kWarning( ) << "Error on creating item:" << job->errorText();
  } else {
  }
}

void KolabProxyResource::imapItemRemoved(const Item& item)
{
  Item addr;
  addr.setRemoteId(QString::number(item.id()));
  ItemDeleteJob *job = new ItemDeleteJob( addr );
  Q_UNUSED(job);
}

void KolabProxyResource::imapCollectionAdded(const Collection &collection, const Collection &parent)
{
  if (m_monitoredCollections.contains(collection.id()))
    return;

  Collection imapCollection;
  CollectionFetchJob *coljob = new CollectionFetchJob( Collection::List() << collection );
  if ( coljob->exec() ) {
    Collection::List collections = coljob->collections();
    imapCollection = collections[0];
  }else {
    kWarning() << "Can't fetch collection" << imapCollection.id();
  }
  CollectionAnnotationsAttribute *annotationsAttribute =
      imapCollection.attribute<CollectionAnnotationsAttribute>();
  if (annotationsAttribute) {
    QMap<QByteArray, QByteArray> annotations = annotationsAttribute->annotations();

    KolabHandler *handler = KolabHandler::createHandler(annotations["/vendor/kolab/folder-type"]);
    if (handler) {
      m_monitor->setCollectionMonitored(imapCollection);
      m_monitoredCollections.insert(imapCollection.id(), handler);
      Collection c = createCollection(imapCollection);
      new CollectionCreateJob(c);
     
    }
  }
}

void KolabProxyResource::imapCollectionRemoved(const Collection &collection)
{
  Collection c;
  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
  if ( job->exec() ) {
    Collection::List collections = job->collections();

    foreach( const Collection &col, collections ) {
      if (col.remoteId() == QString::number(collection.id())) {
        c = col;
        break;
      }
    }
  } else {
    kWarning() << "Can't find the matching collection for  " << collection.id();
    return;
  }

  KolabHandler *handler = m_monitoredCollections.value(collection.id());
  delete handler;
  m_monitoredCollections.remove(collection.id());
  new CollectionDeleteJob(c);
}

void KolabProxyResource::imapCollectionChanged(const Collection &collection)
{
//   kDebug() << "imapCollectionChanged" << collection.id();
  if (m_monitoredCollections.contains(collection.id()))
    return;

  Collection imapCollection;
  CollectionFetchJob *coljob = new CollectionFetchJob( Collection::List() << collection );
  if ( coljob->exec() ) {
    Collection::List collections = coljob->collections();
    imapCollection = collections[0];
  }else {
    kWarning() << "Can't fetch collection" << imapCollection.id();
  }
  CollectionAnnotationsAttribute *annotationsAttribute =
      imapCollection.attribute<CollectionAnnotationsAttribute>();
  if (annotationsAttribute) {
    QMap<QByteArray, QByteArray> annotations = annotationsAttribute->annotations();
    KolabHandler *handler = KolabHandler::createHandler(annotations["/vendor/kolab/folder-type"]);
    if (handler) {
      m_monitor->setCollectionMonitored(imapCollection);
      m_monitoredCollections.insert(imapCollection.id(), handler);
      Collection c = createCollection(imapCollection);
      new CollectionCreateJob(c);
     
    }
  }
}

Collection KolabProxyResource::createCollection(const Collection& imapCollection)
{
  Collection c;
  c.setParent( Collection::root() );
  QString origName = imapCollection.name();
  QString name = origName;
  uint i = 1;
  while ( m_managedCollections.contains(name) ) {
    name = origName + "_" + QString::number(i);
  }
  m_managedCollections << name;
  c.setName( name );
  c.setContentMimeTypes( QStringList() << "text/directory" );
  c.setRights( Collection::Right(Collection::CanChangeItem | Collection::CanCreateItem | Collection::CanDeleteItem | Collection::CanChangeCollection) );
  c.setRemoteId(QString::number(imapCollection.id()));

  return c;
}

AKONADI_RESOURCE_MAIN( KolabProxyResource )

#include "kolabproxyresource.moc"
