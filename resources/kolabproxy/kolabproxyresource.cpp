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
#include "contact.h"
#include "collectionannotationsattribute.h"
#include <akonadi/attributefactory.h>

#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/monitor.h>
#include <akonadi/item.h>
#include <kabc/addressee.h>
#include <QtDBus/QDBusConnection>
#include <QApplication>

using namespace Akonadi;

KolabProxyResource::KolabProxyResource( const QString &id )
  : ResourceBase( id )
{
  Akonadi::AttributeFactory::registerAttribute<CollectionAnnotationsAttribute>();

  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

  m_monitor = new Akonadi::Monitor();
  Akonadi::ItemFetchScope scope;
  scope.fetchFullPayload();
  m_monitor->setItemFetchScope(scope);

  connect(m_monitor, SIGNAL(itemAdded(const Akonadi::Item & , const Akonadi::Collection &)), this, SLOT(imapItemAdded(const Akonadi::Item & , const Akonadi::Collection &)));
  connect(m_monitor, SIGNAL(itemRemoved(const Akonadi::Item &)), this, SLOT(imapItemRemoved(const Akonadi::Item &)));


  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
  if ( job->exec() ) {
    Collection::List collections = job->collections();
    foreach( const Collection &collection, collections ) {
      CollectionAnnotationsAttribute *annotationsAttribute =
          collection.attribute<CollectionAnnotationsAttribute>();
      if (annotationsAttribute) {
        QMap<QByteArray, QByteArray> annotations = annotationsAttribute->annotations();
        if (annotations["/vendor/kolab/folder-type"] ==  "contact.default" || annotations["/vendor/kolab/folder-type"] ==  "contact") {
          qDebug() << "Monitor folder: " << collection.name() << collection.remoteId();
           m_monitor->setCollectionMonitored(collection);
        }
      }
    }
  }
}

KolabProxyResource::~KolabProxyResource()
{
}

void KolabProxyResource::retrieveCollections()
{
  Collection::List addrCollections;
  Collection::List imapCollections = m_monitor->collectionsMonitored();

  Q_FOREACH(Collection collection, imapCollections) {

    Collection c;
    c.setParent( Collection::root() );
    QString origName = collection.name();
    QString name = origName;
    uint i = 1;
    while ( m_managedCollections.contains(name) ) {
      name = origName + "_" + QString::number(i);
    }
    m_managedCollections << name;
    c.setName( name );
    c.setContentMimeTypes( QStringList() << "text/directory" );
    c.setRights( Collection::ReadOnly );
    c.setRemoteId(QString::number(collection.id()));
    collectionsRetrievedIncremental(Collection::List() << c, Collection::List());
  }
}

void KolabProxyResource::retrieveItems( const Akonadi::Collection &collection )
{
  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( Collection(collection.remoteId().toUInt()) );
  job->fetchScope().fetchFullPayload();
  if (job->exec()) {
    Akonadi::Item::List items = job->items();
    createAddressEntry(items);
  }
}

bool KolabProxyResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( item );
  Q_UNUSED( parts );

  // TODO: this method is called when Akonadi wants more data for a given item.
  // You can only provide the parts that have been requested but you are allowed
  // to provide all in one go

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

void KolabProxyResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  Q_UNUSED( item );
  Q_UNUSED( collection );

  // TODO: this method is called when somebody else, e.g. a client application,
  // has created an item in a collection managed by your resource.

  // NOTE: There is an equivalent method for collections, but it isn't part
  // of this template code to keep it simple
}

void KolabProxyResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( item );
  Q_UNUSED( parts );

  // TODO: this method is called when somebody else, e.g. a client application,
  // has changed an item managed by your resource.

  // NOTE: There is an equivalent method for collections, but it isn't part
  // of this template code to keep it simple
}

void KolabProxyResource::itemRemoved( const Akonadi::Item &item )
{
  Q_UNUSED( item );

  // TODO: this method is called when somebody else, e.g. a client application,
  // has deleted an item managed by your resource.

  // NOTE: There is an equivalent method for collections, but it isn't part
  // of this template code to keep it simple
}

void KolabProxyResource::imapItemAdded(const Akonadi::Item& item, const Akonadi::Collection &collection)
{
  m_id = QString::number(collection.id());
  m_item = item;
//TODO: slow, would be nice if ItemCreateJob would work with a Collection having only the remoteId set
  CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
  connect(job, SIGNAL(result(KJob*)), this, SLOT(collectionFetchDone(KJob *)));
}

void KolabProxyResource::collectionFetchDone(KJob *job)
{
  if ( job->error() ) {
    kWarning( ) << "Error on collection fetch:" << job->errorText();
  } else {
    Collection c;
    Collection::List collections = qobject_cast<CollectionFetchJob*>(job)->collections();
    foreach( const Collection &col, collections ) {
      if (col.remoteId() == m_id) {
        c = col;
        break;
      }
    }
    KABC::Addressee addressee;
    if (addresseFromKolab(m_item.payloadData(), addressee)) {
      Item newItem("text/directory");
      newItem.setRemoteId(QString::number(m_item.id()));
      newItem.setPayload(addressee);
      Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob( newItem,  c );
      connect(job, SIGNAL(result(KJob*)), this, SLOT(itemCreatedDone(KJob *)));
    }
  }
}

void KolabProxyResource::itemCreatedDone(KJob *job)
{
  if ( job->error() ) {
    kWarning( ) << "Error on creating item:" << job->errorText();
  } else {
  }
}

void KolabProxyResource::imapItemRemoved(const Akonadi::Item& item)
{
  Item addr;
  addr.setRemoteId(QString::number(item.id()));
  ItemDeleteJob *job = new ItemDeleteJob( addr );
  Q_UNUSED(job);
}

bool KolabProxyResource::addresseFromKolab(const QByteArray &data, KABC::Addressee &addressee)
{
  int pos = data.indexOf("Content-Type: application/x-vnd.kolab.contact;");
  if (pos != -1) {
    pos = data.indexOf("<?xml", pos);//TODO find the encoding???

    QByteArray xmlData = data.mid( pos, data.indexOf("--Boundary-", pos) - pos);
    Kolab::Contact contact(QString::fromLatin1(xmlData));
    contact.saveTo(&addressee);
    return true;
  }
  return false;
}

void KolabProxyResource::createAddressEntry(const Akonadi::Item::List & addrs)
{
  Akonadi::Item::List items;
  Q_FOREACH(Akonadi::Item addr, addrs)
  {
    QByteArray payload = addr.payloadData();
    KABC::Addressee addressee;
    if (addresseFromKolab(payload, addressee)) {
      Item item("text/directory");
      item.setRemoteId(QString::number(addr.id()));
      item.setPayload(addressee);
      items << item;
    }
  }
  itemsRetrieved(items);
}

AKONADI_RESOURCE_MAIN( KolabProxyResource )

#include "kolabproxyresource.moc"
