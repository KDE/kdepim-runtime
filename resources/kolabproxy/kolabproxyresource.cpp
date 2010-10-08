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
#include "setupkolab.h"
#include "settingsadaptor.h"
#include "collectionannotationsattribute.h"
#include "addressbookhandler.h"
#include "collectiontreebuilder.h"

#include <akonadi/attributefactory.h>
#include <akonadi/cachepolicy.h>
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
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/entityhiddenattribute.h>
#include <akonadi/session.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/collectionmovejob.h>
#include <akonadi/itemmovejob.h>

#include <KLocale>

#include <QtDBus/QDBusConnection>
#include <QSet>

#ifdef RUNTIME_PLUGINS_STATIC
#include <QtPlugin>

Q_IMPORT_PLUGIN(akonadi_serializer_mail)
Q_IMPORT_PLUGIN(akonadi_serializer_addressee)
Q_IMPORT_PLUGIN(akonadi_serializer_kcalcore)
Q_IMPORT_PLUGIN(akonadi_serializer_contactgroup)
#endif

using namespace Akonadi;

static const char KOLAB_COLLECTION[] = "KolabCollection";
static const char KOLAB_ITEM[] = "KolabItem";
static const char IMAP_COLLECTION[] = "ImapCollection";

template <typename T>
static inline T kolabToImap( const T &kolabObject )
{
  return T( kolabObject.remoteId().toLongLong() );
}

template <typename T>
static inline T imapToKolab( const T &imapObject )
{
  T kolabObject;
  kolabObject.setRemoteId( QString::number( imapObject.id() ) );
  return kolabObject;
}

KolabProxyResource::KolabProxyResource( const QString &id )
  : ResourceBase( id )
{
  AttributeFactory::registerAttribute<CollectionAnnotationsAttribute>();

  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

  changeRecorder()->fetchCollection( true );
  changeRecorder()->itemFetchScope().fetchFullPayload();

  m_monitor = new Monitor( this );
  m_monitor->itemFetchScope().fetchFullPayload();

  m_collectionMonitor = new Monitor( this );
  m_collectionMonitor->fetchCollection( true );
  m_collectionMonitor->setCollectionMonitored(Collection::root());
  m_collectionMonitor->ignoreSession( Session::defaultSession() );

  connect(m_monitor, SIGNAL(itemAdded(const Akonadi::Item & , const Akonadi::Collection &)), this, SLOT(imapItemAdded(const Akonadi::Item & , const Akonadi::Collection &)));
  connect(m_monitor, SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)), this, SLOT(imapItemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)));
  connect(m_monitor, SIGNAL(itemRemoved(const Akonadi::Item &)), this, SLOT(imapItemRemoved(const Akonadi::Item &)));
  connect(m_collectionMonitor, SIGNAL(collectionAdded(const Akonadi::Collection &, const Akonadi::Collection &)), this, SLOT(imapCollectionAdded(const Akonadi::Collection &, const Akonadi::Collection &)));
  connect(m_collectionMonitor, SIGNAL(collectionRemoved(const Akonadi::Collection &)), this, SLOT(imapCollectionRemoved(const Akonadi::Collection &)));
  connect(m_collectionMonitor, SIGNAL(collectionChanged(const Akonadi::Collection &)), this, SLOT(imapCollectionChanged(const Akonadi::Collection &)));
  connect(m_collectionMonitor, SIGNAL(collectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)), this, SLOT(imapCollectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)) );

  setName( i18n("Kolab") );

  // among other things, this ensures that m_root actually exists when a new imap folder is added
  synchronizeCollectionTree();
}

KolabProxyResource::~KolabProxyResource()
{
}

void KolabProxyResource::retrieveCollections()
{
  kDebug() << "RETRIEVECOLLECTIONS ";
  CollectionTreeBuilder *job = new CollectionTreeBuilder( this );
  connect(job, SIGNAL(result(KJob*)), this, SLOT(retrieveCollectionsTreeDone(KJob *)));
}

void KolabProxyResource::retrieveCollectionsTreeDone(KJob* job)
{
  if ( job->error() ) {
    kWarning( ) << "Error on collection fetch:" << job->errorText();
    cancelTask( job->errorText() );
  } else {
    Collection::List imapCollections = qobject_cast<CollectionTreeBuilder*>( job )->allCollections();

    Collection::List kolabCollections;
    Q_FOREACH(const Collection &collection, imapCollections)
      kolabCollections.append( createCollection(collection) );
    collectionsRetrieved( kolabCollections );
  }

}

void KolabProxyResource::retrieveItems( const Collection &collection )
{
  kDebug() << "RETRIEVEITEMS";
  m_retrieveState = RetrieveItems;
  ItemFetchJob *job = new ItemFetchJob( kolabToImap( collection ) );
  job->fetchScope().fetchFullPayload();
  connect(job, SIGNAL(result(KJob*)), this, SLOT(retrieveItemFetchDone(KJob *)));
}

bool KolabProxyResource::retrieveItem( const Item &item, const QSet<QByteArray> &parts )
{
  kDebug() << "RETRIEVEITEM";
  m_retrieveState = RetrieveItem;
  ItemFetchJob *job = new ItemFetchJob( kolabToImap( item ) );
  job->fetchScope().fetchFullPayload();
  connect(job, SIGNAL(result(KJob*)), this, SLOT(retrieveItemFetchDone(KJob *)));
  return true;
}

void KolabProxyResource::retrieveItemFetchDone(KJob *job)
{
  if ( job->error() ) {
    kWarning( ) << "Error on item fetch:" << job->errorText();
    cancelTask();
  } else {
    Item::Id collectionId = -1;
    Item::List items = qobject_cast<ItemFetchJob*>(job)->items();
    if (items.size() < 1) {
      kWarning() << "Items is emtpy";
      cancelTask();
      return;
    }
    collectionId = items[0].storageCollectionId();
    KolabHandler *handler = m_monitoredCollections.value(collectionId);
    if (handler) {
      if (m_retrieveState == DeleteItem) {
        kDebug() << "m_retrieveState = DeleteItem";
        handler->itemDeleted(items[0]);
      } else {
        Item::List newItems = handler->translateItems(items);
        if (m_retrieveState == RetrieveItems) {
          itemsRetrieved(newItems);
        } else
          itemRetrieved(newItems[0]);
      }
      kDebug() << "RETRIEVEITEM DONE";
    } else {
      cancelTask();
    }
  }
}

void KolabProxyResource::aboutToQuit()
{
  qDeleteAll(m_monitoredCollections);
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

  QPointer<SetupKolab> kolabConfigDialog( new SetupKolab( this, windowId ) );
  if ( kolabConfigDialog->exec() == QDialog::Accepted ) {
    emit configurationDialogAccepted();
  }
  else {
    emit configurationDialogRejected();
  }

  delete kolabConfigDialog;
}

void KolabProxyResource::itemAdded( const Item &item, const Collection &collection )
{
  kDebug() << "ITEMADDED";

  Item kolabItem( item );
//   kDebug() << "Item added " << item.id() << collection.remoteId() << collection.id();

  const Collection imapCollection = kolabToImap( collection );

  KolabHandler *handler  = m_monitoredCollections.value(imapCollection.id());
  if ( !handler ) {
    kWarning() << "No handler found for collection" << collection << ", available handlers: " << m_monitoredCollections;
    cancelTask();
    return;
  }
  Item imapItem(handler->contentMimeTypes()[0]);
  handler->toKolabFormat( kolabItem, imapItem );

  ItemCreateJob *cjob = new ItemCreateJob(imapItem, imapCollection);
  cjob->setProperty( KOLAB_ITEM, QVariant::fromValue( kolabItem ) );
  cjob->setProperty( IMAP_COLLECTION, QVariant::fromValue( imapCollection ) );
  connect( cjob, SIGNAL(result(KJob*)), SLOT(imapItemCreationResult(KJob*)) );
}

void KolabProxyResource::imapItemCreationResult(KJob* job)
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }

  ItemCreateJob *cjob = qobject_cast<ItemCreateJob*>( job );
  const Item imapItem = cjob->item();
  Item kolabItem = cjob->property( KOLAB_ITEM ).value<Item>();
  // TODO add accessor to ItemCreateJob for the parent collection
  const Collection imapCollection = cjob->property( IMAP_COLLECTION ).value<Collection>();

  KolabHandler *handler  = m_monitoredCollections.value(imapCollection.id());
  Q_ASSERT( handler );
  handler->itemAdded(imapItem);
  m_excludeAppend << imapItem.id();

  kolabItem.setRemoteId( QString::number( imapItem.id() ) );
  changeCommitted( kolabItem );
}

void KolabProxyResource::itemChanged( const Item &kolabItem, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );
  kDebug() << "ITEMCHANGED" << kolabItem.id() << kolabItem.remoteId();

  ItemFetchJob* job = new ItemFetchJob( kolabToImap( kolabItem ), this );
  job->setProperty( KOLAB_ITEM, QVariant::fromValue( kolabItem ) );
  connect( job, SIGNAL(result(KJob*)), SLOT(imapItemUpdateFetchResult(KJob*)) );
}

void KolabProxyResource::imapItemUpdateFetchResult(KJob* job)
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }

  const Item kolabItem = job->property( KOLAB_ITEM ).value<Item>();

  ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>( job );
  Q_ASSERT( fetchJob->items().size() <= 1 );
  if ( fetchJob->items().size() == 1 ) {
    Item imapItem = fetchJob->items().first();

    KolabHandler *handler = m_monitoredCollections.value( imapItem.storageCollectionId() );
    if (!handler) {
      kWarning() << "No handler found";
      cancelTask();
      return;
    }

    handler->toKolabFormat( kolabItem , imapItem );
    ItemModifyJob *mjob = new ItemModifyJob( imapItem );
    mjob->setProperty( KOLAB_ITEM, fetchJob->property( KOLAB_ITEM ) );
    connect( mjob, SIGNAL(result(KJob*)), SLOT(imapItemUpdateResult(KJob*)) );
  } else {
    // HACK FIXME how can that happen at all?
    CollectionFetchJob *fetch = new CollectionFetchJob( Collection( kolabItem.storageCollectionId() ), CollectionFetchJob::Base, this );
    fetch->setProperty( KOLAB_ITEM, QVariant::fromValue( kolabItem ) );
    connect( fetch, SIGNAL(result(KJob*)), SLOT(imapItemUpdateCollectionFetchResult(KJob*)) );
  }
}

void KolabProxyResource::imapItemUpdateCollectionFetchResult( KJob* job )
{
  CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob*>( job );
  if ( job->error() || fetchJob->collections().size() != 1 ) {
    cancelTask( job->errorText() );
    return;
  }

  const Item kolabItem = job->property( KOLAB_ITEM ).value<Item>();
  const Collection kolabCollection = fetchJob->collections().first();
  const Collection imapCollection = kolabToImap( kolabCollection );

  KolabHandler *handler  = m_monitoredCollections.value(imapCollection.id());
  if ( !handler ) {
    kWarning() << "No handler found";
    cancelTask();
    return;
  }
  Item imapItem(handler->contentMimeTypes()[0]);
  handler->toKolabFormat( kolabItem, imapItem );

  ItemCreateJob *cjob = new ItemCreateJob(imapItem, imapCollection);
  cjob->setProperty( KOLAB_ITEM, QVariant::fromValue( kolabItem ) );
  cjob->setProperty( IMAP_COLLECTION, QVariant::fromValue( imapCollection ) );
  connect( cjob, SIGNAL(result(KJob*)), SLOT(imapItemCreationResult(KJob*)) );
}

void KolabProxyResource::imapItemUpdateResult(KJob* job)
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }
  const Item kolabItem = job->property( KOLAB_ITEM ).value<Item>();
  changeCommitted( kolabItem );
}

void KolabProxyResource::itemMoved(const Akonadi::Item& item, const Akonadi::Collection& collectionSource, const Akonadi::Collection& collectionDestination)
{
  Q_UNUSED( collectionSource );
  new ItemMoveJob( kolabToImap( item ), kolabToImap( collectionDestination ), this );
  changeCommitted( item );
}

void KolabProxyResource::itemRemoved( const Item &item )
{
  kDebug() << "ITEMREMOVED";
  kDebug() << "Item removed " << item.id() << item.remoteId();
  const Item imapItem( item.remoteId().toUInt() );
  ItemDeleteJob *djob = new ItemDeleteJob( imapItem );
  changeCommitted( item );
  Q_UNUSED(djob);
}

void KolabProxyResource::collectionAdded(const Akonadi::Collection& collection, const Akonadi::Collection& parent)
{
  Q_ASSERT( !KolabHandler::kolabTypeForCollection( collection ).isEmpty() );

  Collection imapCollection( collection );
  imapCollection.setId( -1 );
  imapCollection.setRemoteId( QString() );
  imapCollection.setContentMimeTypes( QStringList() << Collection::mimeType() << QLatin1String( "message/rfc822" ) );
  const Collection imapParent = kolabToImap( parent );
  imapCollection.setParentCollection( imapParent );
  CollectionAnnotationsAttribute* attr =
    imapCollection.attribute<CollectionAnnotationsAttribute>( Collection::AddIfMissing );
  QMap<QByteArray, QByteArray> annotations = attr->annotations();
  annotations["/vendor/kolab/folder-type"] = KolabHandler::kolabTypeForCollection( collection );
  attr->setAnnotations( annotations );

  CollectionCreateJob *job = new CollectionCreateJob( imapCollection, this );
  job->setProperty( KOLAB_COLLECTION, QVariant::fromValue( collection ) );
  connect( job, SIGNAL(result(KJob*)), SLOT(imapFolderCreateResult(KJob*)) );
}

void KolabProxyResource::imapFolderCreateResult(KJob* job)
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
  } else {
    const Collection imapCollection = qobject_cast<CollectionCreateJob*>( job )->collection();
    Collection kolabCollection = job->property( KOLAB_COLLECTION ).value<Collection>();
    kolabCollection.setRemoteId( QString::number( imapCollection.id() ) );
    changeCommitted( kolabCollection );
  }
}

void KolabProxyResource::applyAttributesToImap( Collection &imapCollection, const Akonadi::Collection &kolabCollection )
{
  static const EntityDisplayAttribute eda;
  static const EntityHiddenAttribute hidden;
  foreach( const Akonadi::Attribute *attr, kolabCollection.attributes() )
  {
    if ( attr->type() == hidden.type() )
      // Don't propagate HIDDEN because that would hide collections in korg, kab too.
      continue;

    if ( attr->type() == eda.type() )
      // Don't propagate DISPLAYATTRIBUTE because that would cause icons from the imap resource to use kolab icons.
      continue;

    if ( attr->type() == "AccessRights" )
      continue;

    //kDebug() << "cloning" << attr->type();
    imapCollection.addAttribute( attr->clone() );
  }
}

void KolabProxyResource::applyAttributesFromImap( Collection &kolabCollection, const Akonadi::Collection &imapCollection )
{
  static const EntityDisplayAttribute eda;
  static const EntityHiddenAttribute hidden;
  foreach( const Akonadi::Attribute *attr, imapCollection.attributes() )
  {
    if ( attr->type() == hidden.type() )
      continue;

    if ( attr->type() == eda.type() )
      continue;

    if ( attr->type() == "AccessRights" )
      continue;

    //kDebug() << "cloning" << attr->type();
    kolabCollection.addAttribute( attr->clone() );
  }
}


void KolabProxyResource::collectionChanged(const Akonadi::Collection& collection)
{
  Collection imapCollection;
  imapCollection.setId( collection.remoteId().toLongLong() );
  imapCollection.setName( collection.name() );
  imapCollection.setCachePolicy( collection.cachePolicy() );
  imapCollection.setRights( collection.rights() );

  applyAttributesToImap( imapCollection, collection );

  CollectionModifyJob *job = new CollectionModifyJob( imapCollection, this );
  // TODO wait for the result
  changeCommitted( collection );
}

void KolabProxyResource::collectionMoved(const Akonadi::Collection& collection, const Akonadi::Collection& source, const Akonadi::Collection& destination)
{
  Q_UNUSED( source );
  new CollectionMoveJob( kolabToImap( collection ), kolabToImap( destination ), this );
  changeCommitted( collection );
}

void KolabProxyResource::collectionRemoved(const Akonadi::Collection& collection)
{
  Collection imapCollection = kolabToImap( collection );

  CollectionDeleteJob *job = new CollectionDeleteJob( imapCollection, this );
  // TODO wait for result
  changeCommitted( collection );
}

void KolabProxyResource::deleteImapItem(const Item& item)
{
  kDebug() << "DELETEIMAPITEM";
  ItemDeleteJob *djob = new ItemDeleteJob( item );
  Q_UNUSED(djob);
}

void KolabProxyResource::addImapItem(const Item& item, Akonadi::Entity::Id collectionId)
{
  kDebug() << "ADDITEMTOIMAP";
  new ItemCreateJob( item, Collection(collectionId) );
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
  const Collection kolabCol = imapToKolab( collection );
  CollectionFetchJob *job = new CollectionFetchJob( kolabCol, CollectionFetchJob::Base, this );
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
  kDebug() << "IMAPITEMREMOVED";
  const Item kolabItem = imapToKolab( item );
  Q_FOREACH(KolabHandler *handler, m_monitoredCollections) {
    handler->itemDeleted(item);
  }
  ItemDeleteJob *job = new ItemDeleteJob( kolabItem, this );
}

void KolabProxyResource::imapItemMoved(const Akonadi::Item& item, const Akonadi::Collection& collectionSource, const Akonadi::Collection& collectionDestination)
{
  kDebug();
  Q_UNUSED( collectionSource );
  new ItemMoveJob( imapToKolab( item ), imapToKolab( collectionDestination ), this );
}

void KolabProxyResource::imapCollectionAdded(const Collection &collection, const Collection &parent)
{
  Q_UNUSED( parent );
  if ( collection.resource() == identifier() ) // just to be sure...
    return;

  kDebug() << "IMAPCOLLECTIONADDED";
  if ( m_monitoredCollections.contains(collection.id()) ) {
    // something is wrong, so better reload out collection tree
    kDebug() << "IMAPCOLLECTIONADDED ABORT";
    synchronizeCollectionTree();
    return;
  }

  if ( registerHandlerForCollection( collection ) ) {
    const Collection kolabCollection = createCollection( collection );
    CollectionCreateJob *job = new CollectionCreateJob( kolabCollection, this );
    connect( job, SIGNAL(result(KJob*)), SLOT(kolabFolderChangeResult(KJob*)) );
  }
}

void KolabProxyResource::imapCollectionChanged(const Collection &collection)
{
  if ( collection.resource() == identifier() ) // just to be sure...
    return;

  //kDebug() << "IMAPCOLLECTIONCHANGED";
  if ( !m_monitoredCollections.contains(collection.id()) ) {
    // check if this is a Kolab folder at all, if yet something is wrong
    CollectionAnnotationsAttribute *annotationsAttribute =
     collection.attribute<CollectionAnnotationsAttribute>();
    bool isKolabFolder = false;
    if ( annotationsAttribute ) {
      const QMap<QByteArray, QByteArray> annotations = annotationsAttribute->annotations();
      QByteArray folderType = annotations[ "/vendor/kolab/folder-type" ];
      isKolabFolder = !folderType.isEmpty() && folderType != "mail";
    }

    if ( isKolabFolder ) {
      synchronizeCollectionTree();
      return;
    }
    // not a Kolab folder, no need to resync the tree, just try to update a possible structural collection
    // if that fails it's not in our tree -> we don't care
    Collection kolabCollection = createCollection( collection );
    CollectionModifyJob *job = new CollectionModifyJob( kolabCollection, this );
  } else {
    // Kolab folder we already have in our tree, if the update fails, reload our tree
    Collection kolabCollection = createCollection( collection );
    CollectionModifyJob *job = new CollectionModifyJob( kolabCollection, this );
    connect( job, SIGNAL(result(KJob*)), SLOT(kolabFolderChangeResult(KJob*)) );
  }
}

void KolabProxyResource::imapCollectionMoved(const Akonadi::Collection& collection, const Akonadi::Collection& source, const Akonadi::Collection& destination)
{
  kDebug();
  Q_UNUSED( source );
  new CollectionMoveJob( imapToKolab( collection ), imapToKolab( destination ), this );
}

void KolabProxyResource::kolabFolderChangeResult(KJob* job)
{
  if ( job->error() ) {
    // something went wrong or the change was too complex to handle in the above slots,
    // so re-sync the entire tree.
    kDebug() << "Re-syncing collection tree as incremental changes did not succeed." << job->errorText();
    synchronizeCollectionTree();
  }
}

void KolabProxyResource::imapCollectionRemoved(const Collection &imapCollection)
{
  if ( imapCollection.resource() == identifier() ) // just to be sure...
    return;

  kDebug() << "IMAPCOLLECTIONREMOVED";
  Collection kolabCollection;
  kolabCollection.setRemoteId( QString::number( imapCollection.id() ) );
  new CollectionDeleteJob( kolabCollection );

  KolabHandler *handler = m_monitoredCollections.value(imapCollection.id());
  delete handler;
  m_monitoredCollections.remove(imapCollection.id());
}

Collection KolabProxyResource::createCollection(const Collection& imapCollection)
{
  Collection c;
  if ( imapCollection.parentCollection() == Collection::root() ) {
    c.setParentCollection( Collection::root() );
    CachePolicy policy;
    policy.setInheritFromParent( false );
    policy.setCacheTimeout( -1 );
    policy.setLocalParts( QStringList() << QLatin1String( "ALL" ) );
    c.setCachePolicy( policy );
  } else {
    c.parentCollection().setRemoteId( QString::number( imapCollection.parentCollection().id() ) );
  }
  c.setName( imapCollection.name() );
  EntityDisplayAttribute *imapAttr = imapCollection.attribute<EntityDisplayAttribute>();
  EntityDisplayAttribute *kolabAttr = c.attribute<EntityDisplayAttribute>( Collection::AddIfMissing );
  if ( imapAttr ) {
    if ( imapAttr->iconName() == QLatin1String( "mail-folder-inbox" ) ) {
      kolabAttr->setDisplayName( i18n( "My Data" ) );
      kolabAttr->setIconName( QLatin1String( "view-pim-summary" ) );
    } else if ( imapCollection.parentCollection() == Collection::root() ) {
      c.setName( i18n( "Kolab (%1)", imapAttr->displayName() ) );
      kolabAttr->setIconName( QLatin1String( "kolab" ) );
    } else {
      kolabAttr->setDisplayName( imapAttr->displayName() );
      kolabAttr->setIconName( imapAttr->iconName() );
    }
  } else {
    if ( imapCollection.parentCollection() == Collection::root() ) {
      c.setName( i18n( "Kolab (%1)", imapCollection.name() ) );
      kolabAttr->setIconName( QLatin1String( "kolab" ) );
    }
  }
  applyAttributesFromImap( c, imapCollection );
  KolabHandler *handler = m_monitoredCollections.value(imapCollection.id());
  QStringList contentTypes;
  contentTypes.append( Collection::mimeType() );
  if ( handler ) {
    contentTypes.append( handler->contentMimeTypes() );
    kolabAttr->setIconName( handler->iconName() );

    // hide Kolab folders on the IMAP server
    if ( !imapCollection.hasAttribute<EntityHiddenAttribute>() ) {
      Collection hiddenImapCol( imapCollection );
      hiddenImapCol.attribute<EntityHiddenAttribute>( Collection::AddIfMissing );
      new CollectionModifyJob( hiddenImapCol, this );
    }
  }
  c.setContentMimeTypes( contentTypes );
  c.setRights( imapCollection.rights() );
  c.setRemoteId(QString::number(imapCollection.id()));
  return c;
}

bool KolabProxyResource::registerHandlerForCollection(const Akonadi::Collection& imapCollection)
{
  CollectionAnnotationsAttribute *annotationsAttribute =
      imapCollection.attribute<CollectionAnnotationsAttribute>();
  if ( annotationsAttribute ) {
    QMap<QByteArray, QByteArray> annotations = annotationsAttribute->annotations();

    KolabHandler *handler = KolabHandler::createHandler(annotations["/vendor/kolab/folder-type"]);
    if ( handler ) {
      connect(handler, SIGNAL(deleteItemFromImap(const Akonadi::Item&)), this, SLOT(deleteImapItem(const Akonadi::Item&)));
      connect(handler, SIGNAL(addItemToImap(const Akonadi::Item&, Akonadi::Entity::Id)), this, SLOT(addImapItem(const Akonadi::Item&, Akonadi::Entity::Id)));
      m_monitor->setCollectionMonitored(imapCollection);
      m_monitoredCollections.insert(imapCollection.id(), handler);
      return true;
    }
  }

  return false;
}

AKONADI_RESOURCE_MAIN( KolabProxyResource )

#include "kolabproxyresource.moc"
