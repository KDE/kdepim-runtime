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
#include <akonadi/itemmodifyjob.h>
#include <akonadi/monitor.h>
#include <akonadi/item.h>
#include <akonadi/changerecorder.h>
#include <kabc/addressee.h>
#include <krandom.h>

#include <QtDBus/QDBusConnection>
#include <QSet>

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
        if (annotations["/vendor/kolab/folder-type"] ==  "contact.default" || annotations["/vendor/kolab/folder-type"] ==  "contact") {
          qDebug() << "Monitor folder: " << collection.name() << collection.remoteId();
          m_monitor->setCollectionMonitored(collection);
        }
      }
    }
  }

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
    c.setRights( Collection::Right(Collection::CanChangeItem | Collection::CanCreateItem | Collection::CanDeleteItem | Collection::CanChangeCollection) );
    c.setRemoteId(QString::number(collection.id()));
    collectionsRetrievedIncremental(Collection::List() << c, Collection::List());
  }
}

void KolabProxyResource::retrieveItems( const Akonadi::Collection &collection )
{
  kDebug() << "RETRIEVEITEMS";
  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( Collection(collection.remoteId().toUInt()) );
  job->fetchScope().fetchFullPayload();
  if (job->exec()) {
    Akonadi::Item::List items = job->items();
    createAddressEntry(items);
  }
}

bool KolabProxyResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  kDebug() << "RETRIEVEITEM";
  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( item );
  job->fetchScope().fetchFullPayload();
  if (job->exec()) {
    Akonadi::Item::List items = job->items();
    createAddressEntry(items);
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

void KolabProxyResource::itemAdded( const Akonadi::Item &_item, const Akonadi::Collection &collection )
{

  Akonadi::Item item(_item);
  kDebug() << "Item added " << item.id() << collection.remoteId() << collection.id();
  Collection c;
  CollectionFetchJob *coljob = new CollectionFetchJob( Collection::List() << collection );
  if ( coljob->exec() ) {
    Collection::List collections = coljob->collections();
    c = collections[0];
  }

  kDebug() << c.remoteId();
  Collection imapCollection(c.remoteId().toUInt());
  coljob = new CollectionFetchJob( Collection::List() << imapCollection );
  if ( coljob->exec() ) {
    Collection::List collections = coljob->collections();
    imapCollection = collections[0];
  }else {
    kWarning() << "Can't fetch collection" << imapCollection.id();
  }
  kDebug() << "Collection got";

  Akonadi::Item addrItem;
  kDebug() << "Collection got2";
  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( item );
  job->fetchScope().fetchFullPayload();
  if (job->exec()) {
    Akonadi::Item::List items = job->items();
    if (items.isEmpty()) {
      kDebug() << "Empty fecth";
      return;
    }

    addrItem = items[0];
  } else {
    kWarning() << "Can't fetch address item " << item.id();
  }

  kDebug() << "Load payload";
  KABC::Addressee addressee = addrItem.payload<KABC::Addressee>();
  Kolab::Contact contact(&addressee, 0);
  Item imapItem;
  imapItem.setMimeType( "message/rfc822" );
  MessagePtr message(new KMime::Message);
  QString header;
  header += "From: " + addressee.fullEmail() + "\n";
  header += "Subject: " + addressee.uid() + "\n";
//   header += "Date: " + QDateTime::currentDateTime().toString(Qt::ISODate) + "\n";
  header += "User-Agent: Akonadi Kolab Proxy Resource \n";
  header += "MIME-Version: 1.0\n";
  header += "X-Kolab-Type: application/x-vnd.kolab.contact\n\n\n";
  message->setContent(header.toLatin1());

  KMime::Content *content = new KMime::Content();
  QByteArray contentData = QByteArray("Content-Type: text/plain; charset=\"us-ascii\"\nContent-Transfer-Encoding: 7bit\n\n") +
  "This is a Kolab Groupware object.\n" +
  "To view this object you will need an email client that can understand the Kolab Groupware format.\n" +
  "For a list of such email clients please visit\n"
  "http://www.kolab.org/kolab2-clients.html\n";
  content->setContent(contentData);
  message->addContent(content);

  content = new KMime::Content();
  header = "Content-Type: application/x-vnd.kolab.contact; name=\"kolab.xml\"\n";
  header += "Content-Transfer-Encoding: 7bit\n"; //FIXME 7bit???
  header += "Content-Disposition: attachment; filename=\"kolab.xml\"";
  content->setHead(header.toLatin1());
  content->setBody(contact.saveXML().toUtf8());
  message->addContent(content);

  imapItem.setPayload<MessagePtr>(message);


  Akonadi::ItemCreateJob *cjob = new Akonadi::ItemCreateJob(imapItem, imapCollection);
  if (!cjob->exec()) {
    kWarning() << "Can't create imap item " << imapItem.id() << cjob->errorString();
  }

  imapItem = cjob->item();
  m_excludeAppend << imapItem.id();

  addrItem.setRemoteId(QString::number(imapItem.id()));
  changeCommitted( addrItem );
}

void KolabProxyResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Akonadi::Item addrItem;
  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( item );
  job->fetchScope().fetchFullPayload();
  if (job->exec()) {
    Akonadi::Item::List items = job->items();
    addrItem = items[0];
  } else {
    kWarning() << "Can't fetch address item " << item.id();
  }
  Item imapItem;
  KABC::Addressee addressee = addrItem.payload<KABC::Addressee>();
  job = new Akonadi::ItemFetchJob( Item(addrItem.remoteId().toUInt()) );
  job->fetchScope().fetchFullPayload();
  if (job->exec()) {
    Akonadi::Item::List items = job->items();
    imapItem = items[0];
  } else {
    kWarning() << "Can't fetch imap item " << addrItem.remoteId();
  }
  MessagePtr message = imapItem.payload<MessagePtr>();
  KMime::Content *xmlContent = findContent(message, "application/x-vnd.kolab.contact");
  if (xmlContent) {
    Kolab::Contact contact(&addressee, 0);
    xmlContent->setBody(contact.saveXML().toUtf8());
    Akonadi::ItemModifyJob *mjob = new Akonadi::ItemModifyJob( imapItem );
    if (!mjob->exec()) {
      kWarning() << "Can't modify imap item " << imapItem.id();
    }
  }
  changeCommitted( addrItem );

}

void KolabProxyResource::itemRemoved( const Akonadi::Item &item )
{
  kDebug() << "Item removed " << item.id() << item.remoteId();
  Item imapItem(item.remoteId().toUInt());
  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( imapItem );
  if (job->exec()) {
    Akonadi::Item::List items = job->items();
    imapItem = items[0];
  } else {
    kWarning() << "Can't fetch imap item " << imapItem.id();
  }
  ItemDeleteJob *djob = new ItemDeleteJob( imapItem );
  changeCommitted( item );
  Q_UNUSED(djob);
}

void KolabProxyResource::imapItemAdded(const Akonadi::Item& item, const Akonadi::Collection &collection)
{
  kDebug() << "imapItemAdded " << item.id();
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
    KABC::Addressee addressee;
    MessagePtr message = m_items[job].payload<MessagePtr>();
    if (addresseFromKolab(message, addressee)) {
      Item newItem("text/directory");
      kDebug() << "m_item " << m_items[job].id();
      newItem.setRemoteId(QString::number(m_items[job].id()));
      newItem.setPayload(addressee);
      Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob( newItem,  c );
      connect(job, SIGNAL(result(KJob*)), this, SLOT(itemCreatedDone(KJob *)));
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

void KolabProxyResource::imapItemRemoved(const Akonadi::Item& item)
{
  Item addr;
  addr.setRemoteId(QString::number(item.id()));
  ItemDeleteJob *job = new ItemDeleteJob( addr );
  Q_UNUSED(job);
}

bool KolabProxyResource::addresseFromKolab(MessagePtr data, KABC::Addressee &addressee)
{
  KMime::Content *xmlContent  = findContent(data, "application/x-vnd.kolab.contact");
  if (xmlContent) {
    QByteArray xmlData = xmlContent->decodedContent();
//     kDebug() << "xmlData " << xmlData;
    Kolab::Contact contact(QString::fromLatin1(xmlData));
    QString pictureAttachmentName = contact.pictureAttachmentName();
    if (!pictureAttachmentName.isEmpty()) {
      KMime::Content *imgContent = findContent(data, "image/png");
      if (imgContent) {
        QByteArray imgData = imgContent->decodedContent();
        QBuffer buffer(&imgData);
        buffer.open(QIODevice::ReadOnly);
        QImage image;
        image.load(&buffer, "PNG");
        contact.setPicture(image);
      }
    }
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
    kDebug() << addr.id();
    MessagePtr payload = addr.payload<MessagePtr>();
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

KMime::Content *KolabProxyResource::findContent(MessagePtr data, const QByteArray &type)
{
  KMime::Content::List list = data->contents();
  Q_FOREACH(KMime::Content *c, list)
  {
    if (c->contentType()->mimeType() ==  type)
      return c;
  }
  return 0L;

}

AKONADI_RESOURCE_MAIN( KolabProxyResource )

#include "kolabproxyresource.moc"
