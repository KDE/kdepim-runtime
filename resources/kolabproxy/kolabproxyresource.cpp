/*
  Copyright (c) 2009 Andras Mantia <amantia@kde.org>
  Copyright (c) 2012 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include "collectiontreebuilder.h"
#include "freebusyupdatehandler.h"
#include "settings.h"
#include "settingsadaptor.h"
#include "setupkolab.h"

#include "collectionannotationsattribute.h" //from shared

#include <Akonadi/AttributeFactory>
#include <Akonadi/CachePolicy>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/CollectionCreateJob>
#include <Akonadi/CollectionDeleteJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/CollectionModifyJob>
#include <Akonadi/CollectionMoveJob>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/EntityHiddenAttribute>
#include <Akonadi/ItemCreateJob>
#include <Akonadi/ItemDeleteJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/ItemMoveJob>
#include <Akonadi/Session>
#include <Akonadi/KMime/MessageFlags>

#include <KLocale>
#include <KWindowSystem>
#include <QDBusInterface>
#include <QDBusReply>

#ifdef RUNTIME_PLUGINS_STATIC
#include <QtPlugin>

Q_IMPORT_PLUGIN(akonadi_serializer_mail)
Q_IMPORT_PLUGIN(akonadi_serializer_addressee)
Q_IMPORT_PLUGIN(akonadi_serializer_kcalcore)
Q_IMPORT_PLUGIN(akonadi_serializer_contactgroup)
#endif

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

static QString mailBoxForImapCollection( const Akonadi::Collection &imapCollection,
                                         bool showWarnings )
{
  if ( imapCollection.remoteId().isEmpty() ) {
    if ( showWarnings ) {
      kWarning() << "Got incomplete ancestor chain:" << imapCollection;
    }
    return QString();
  }

  if ( imapCollection.parentCollection() == Akonadi::Collection::root() ) {
    return QString( "" );
  }

  const QString parentMailbox =
    mailBoxForImapCollection( imapCollection.parentCollection(), showWarnings );

  if ( parentMailbox.isNull() ) {
    // invalid, != isEmpty() here!
    return QString();
  }

  const QString mailbox =  parentMailbox + imapCollection.remoteId();

  return mailbox;
}

KolabProxyResource::KolabProxyResource( const QString &id )
  : ResourceBase( id )
{
  Akonadi::AttributeFactory::registerAttribute<Akonadi::CollectionAnnotationsAttribute>();

  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject(
    QLatin1String( "/Settings" ),
    Settings::self(), QDBusConnection::ExportAdaptors );

  changeRecorder()->fetchCollection( true );
  changeRecorder()->itemFetchScope().fetchFullPayload();

  m_monitor = new Akonadi::Monitor( this );
  m_monitor->itemFetchScope().fetchFullPayload();
  m_monitor->itemFetchScope().setAncestorRetrieval( Akonadi::ItemFetchScope::All );

  m_collectionMonitor = new Akonadi::Monitor( this );
  m_collectionMonitor->fetchCollection( true );
  m_collectionMonitor->setCollectionMonitored( Akonadi::Collection::root() );
  m_collectionMonitor->ignoreSession( Akonadi::Session::defaultSession() );
  m_collectionMonitor->collectionFetchScope().setAncestorRetrieval(
    Akonadi::CollectionFetchScope::All );

  m_freeBusyUpdateHandler = new FreeBusyUpdateHandler( this );

  connect( m_monitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)),
           this, SLOT(imapItemAdded(Akonadi::Item,Akonadi::Collection)) );

  connect( m_monitor, SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)),
           this, SLOT(imapItemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)) );

  connect( m_monitor, SIGNAL(itemRemoved(Akonadi::Item)),
           this, SLOT(imapItemRemoved(Akonadi::Item)) );

  //We don't connect to changed because an edit results in a new item (append/delete) on imap

  connect( m_collectionMonitor, SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)),
           this, SLOT(imapCollectionAdded(Akonadi::Collection,Akonadi::Collection)) );

  connect( m_collectionMonitor, SIGNAL(collectionRemoved(Akonadi::Collection)),
           this, SLOT(imapCollectionRemoved(Akonadi::Collection)) );

  connect( m_collectionMonitor, SIGNAL(collectionChanged(Akonadi::Collection)),
           this, SLOT(imapCollectionChanged(Akonadi::Collection)) );

  connect( m_collectionMonitor,
           SIGNAL(collectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)),
           this,
           SLOT(imapCollectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)) );

  setName( i18n( "Kolab" ) );

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
  connect( job, SIGNAL(result(KJob*)), this, SLOT(retrieveCollectionsTreeDone(KJob*)) );
}

void KolabProxyResource::retrieveCollectionsTreeDone( KJob *job )
{
  if ( job->error() ) {
    kWarning( ) << "Error on collection fetch:" << job->errorText();
    cancelTask( job->errorText() );
  } else {
    Akonadi::Collection::List imapCollections =
      qobject_cast<CollectionTreeBuilder*>( job )->allCollections();

    Akonadi::Collection::List kolabCollections;
    Q_FOREACH ( const Akonadi::Collection &collection, imapCollections ) {
      kolabCollections.append( createCollection( collection ) );
    }
    collectionsRetrieved( kolabCollections );
  }
}

void KolabProxyResource::retrieveItems( const Akonadi::Collection &collection )
{
  kDebug() << "RETRIEVEITEMS";
  m_retrieveState = RetrieveItems;
  const Akonadi::Collection imapCollection = kolabToImap( collection );
  KolabHandler::Ptr handler = m_monitoredCollections.value( imapCollection.id() );
  Q_ASSERT( handler );
  handler->reset();
  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( imapCollection );
  job->fetchScope().fetchFullPayload();
  job->setProperty( "resultCanBeEmpty", true );

  connect( job, SIGNAL(result(KJob*)), this, SLOT(retrieveItemFetchDone(KJob*)) );
}

bool KolabProxyResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );
  kDebug() << "RETRIEVEITEM";
  m_retrieveState = RetrieveItem;
  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( kolabToImap( item ) );
  job->fetchScope().fetchFullPayload();
  job->setProperty( "itemId", item.id() );
  connect( job, SIGNAL(result(KJob*)), this, SLOT(retrieveItemFetchDone(KJob*)) );
  return true;
}

void KolabProxyResource::retrieveItemFetchDone( KJob *job )
{
  if ( job->error() ) {
    kWarning( ) << "Error on item fetch:" << job->errorText();
    cancelTask();
    return;
  }
  const bool resultCanBeEmpty = job->property( "resultCanBeEmpty" ).isValid();

  Akonadi::Item::Id collectionId = -1;
  const Akonadi::Item::List items = qobject_cast<Akonadi::ItemFetchJob*>(job)->items();
  if ( items.size() < 1 ) {
    if ( resultCanBeEmpty ) {
      itemsRetrieved( Akonadi::Item::List() );
    } else {
      kWarning() << "Items is emtpy";
      cancelTask();
    }
    return;
  }
  collectionId = items[0].storageCollectionId();
  KolabHandler::Ptr handler = m_monitoredCollections.value(collectionId);
  if ( !handler ) {
    kWarning() << "No handler for collection available: " << collectionId;
    cancelTask();
    return;
  }
  if ( m_retrieveState == DeleteItem ) {
    kDebug() << "m_retrieveState = DeleteItem";
    handler->itemDeleted( items[0] );
    return;
  }
  Akonadi::Item::List newItems = handler->translateItems( items );
  if ( m_retrieveState == RetrieveItems ) {
    itemsRetrieved( newItems );
  } else { //RetrieveItem
    if ( !newItems.isEmpty() ) {
      Akonadi::Item item = newItems[0];
      item.setId(job->property("itemId").value<Akonadi::Item::Id>());
      itemRetrieved( item );
    } else {
      kWarning() << "Could not translate item";
      cancelTask();
      return;
    }
  }
  kDebug() << "RETRIEVEITEM DONE";
}

void KolabProxyResource::aboutToQuit()
{
  m_monitoredCollections.clear();
}

Kolab::Version readKolabVersion( const QString &resourceIdentifier )
{
  KConfigGroup grp( KGlobal::mainComponent().config(), "KolabProxyResourceSettings" );
  return static_cast<Kolab::Version>(
    grp.readEntry<int>( "KolabFormatVersion" + resourceIdentifier,
                        static_cast<int>( Kolab::KolabV2 ) ) );
}

void KolabProxyResource::configure( WId windowId )
{
  // TODO: this method is usually called when a new resource is being
  // added to the Akonadi setup. You can do any kind of user interaction here,
  // e.g. showing dialogs.
  // The given window ID is usually useful to get the correct
  // "on top of parent" behavior if the running window manager applies any kind
  // of focus stealing prevention technique

  QPointer<SetupKolab> kolabConfigDialog( new SetupKolab( this ) );
  if ( windowId )
    KWindowSystem::setMainWindow( kolabConfigDialog, windowId );

  kolabConfigDialog->setWindowIcon( KIcon( "kolab" ) );
  kolabConfigDialog->exec();
  emit configurationDialogAccepted();

  foreach ( Akonadi::Entity::Id id, m_monitoredCollections.keys() ) { //krazy:exclude=foreach
    KolabHandler::Ptr handler = m_monitoredCollections.value( id );
    Kolab::Version v = readKolabVersion( m_resourceIdentifier.value( id ) );
    handler->setKolabFormatVersion( v );
  }

  delete kolabConfigDialog;
}

void KolabProxyResource::itemAdded( const Akonadi::Item &item,
                                    const Akonadi::Collection &collection )
{
  kDebug() << "ITEMADDED";

  Akonadi::Item kolabItem( item );
//   kDebug() << "Item added " << item.id() << collection.remoteId() << collection.id();

  const Akonadi::Collection imapCollection = kolabToImap( collection );

  KolabHandler::Ptr handler = m_monitoredCollections.value( imapCollection.id() );
  if ( !handler ) {
    kWarning() << "No handler found for collection" << collection
               << ", available handlers: " << m_monitoredCollections;
    cancelTask();
    return;
  }
  Akonadi::Item imapItem( handler->contentMimeTypes()[0] );
  if (!handler->toKolabFormat( kolabItem, imapItem )) {
    kWarning() << "Failed to convert item to kolab format: " << kolabItem.id();
    cancelTask();
    return;
  }
  imapItem.setFlag( Akonadi::MessageFlags::Seen );

  Akonadi::ItemCreateJob *cjob = new Akonadi::ItemCreateJob( imapItem, imapCollection );
  cjob->setProperty( KOLAB_ITEM, QVariant::fromValue( kolabItem ) );
  cjob->setProperty( IMAP_COLLECTION, QVariant::fromValue( imapCollection ) );
  connect( cjob, SIGNAL(result(KJob*)), SLOT(imapItemCreationResult(KJob*)) );
}

void KolabProxyResource::imapItemCreationResult( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }

  Akonadi::ItemCreateJob *cjob = qobject_cast<Akonadi::ItemCreateJob*>( job );
  const Akonadi::Item imapItem = cjob->item();
  Akonadi::Item kolabItem = cjob->property( KOLAB_ITEM ).value<Akonadi::Item>();

  // TODO add accessor to ItemCreateJob for the parent collection
  const Akonadi::Collection imapCollection =
    cjob->property( IMAP_COLLECTION ).value<Akonadi::Collection>();

  KolabHandler::Ptr handler = m_monitoredCollections.value( imapCollection.id() );
  Q_ASSERT( handler );
  handler->itemAdded( imapItem );
  m_excludeAppend << imapItem.id();

  kolabItem.setRemoteId( QString::number( imapItem.id() ) );
  changeCommitted( kolabItem );
}

void KolabProxyResource::itemChanged( const Akonadi::Item &kolabItem,
                                      const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );
  kDebug() << "ITEMCHANGED" << kolabItem.id() << kolabItem.remoteId();

  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( kolabToImap( kolabItem ), this );
  job->setProperty( KOLAB_ITEM, QVariant::fromValue( kolabItem ) );
  connect( job, SIGNAL(result(KJob*)), SLOT(imapItemUpdateFetchResult(KJob*)) );
}

void KolabProxyResource::imapItemUpdateFetchResult( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }

  const Akonadi::Item kolabItem = job->property( KOLAB_ITEM ).value<Akonadi::Item>();

  Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>( job );
  Q_ASSERT( fetchJob->items().size() <= 1 );
  if ( fetchJob->items().size() == 1 ) { //TODO remove this hack
    Akonadi::Item imapItem = fetchJob->items().first();

    KolabHandler::Ptr handler = m_monitoredCollections.value( imapItem.storageCollectionId() );
    if ( !handler ) {
      kWarning() << "No handler found";
      cancelTask();
      return;
    }

    if (!handler->toKolabFormat( kolabItem, imapItem )) {
      kWarning() << "Failed to convert item to kolab format: " << kolabItem.id();
      cancelTask();
      return;
    }
    Akonadi::ItemModifyJob *mjob = new Akonadi::ItemModifyJob( imapItem );
    mjob->setProperty( KOLAB_ITEM, fetchJob->property( KOLAB_ITEM ) );
    connect( mjob, SIGNAL(result(KJob*)), SLOT(imapItemUpdateResult(KJob*)) );
  } else {
    // HACK FIXME how can that happen at all?
    Akonadi::CollectionFetchJob *fetch =
      new Akonadi::CollectionFetchJob( Akonadi::Collection( kolabItem.storageCollectionId() ),
                                       Akonadi::CollectionFetchJob::Base, this );
    fetch->setProperty( KOLAB_ITEM, QVariant::fromValue( kolabItem ) );
    connect( fetch, SIGNAL(result(KJob*)), SLOT(imapItemUpdateCollectionFetchResult(KJob*)) );
  }
}

void KolabProxyResource::imapItemUpdateCollectionFetchResult( KJob *job )
{
  Akonadi::CollectionFetchJob *fetchJob = qobject_cast<Akonadi::CollectionFetchJob*>( job );
  if ( job->error() || fetchJob->collections().size() != 1 ) {
    cancelTask( job->errorText() );
    return;
  }

  const Akonadi::Item kolabItem = job->property( KOLAB_ITEM ).value<Akonadi::Item>();
  const Akonadi::Collection kolabCollection = fetchJob->collections().first();
  const Akonadi::Collection imapCollection = kolabToImap( kolabCollection );

  KolabHandler::Ptr handler = m_monitoredCollections.value( imapCollection.id() );
  if ( !handler ) {
    kWarning() << "No handler found";
    cancelTask();
    return;
  }
  Akonadi::Item imapItem( handler->contentMimeTypes()[0] );
  if (!handler->toKolabFormat( kolabItem, imapItem )) {
    kWarning() << "Failed to convert item to kolab format: " << kolabItem.id();
    cancelTask();
    return;
  }
  imapItem.setFlag( Akonadi::MessageFlags::Seen );

  Akonadi::ItemCreateJob *cjob = new Akonadi::ItemCreateJob( imapItem, imapCollection );
  cjob->setProperty( KOLAB_ITEM, QVariant::fromValue( kolabItem ) );
  cjob->setProperty( IMAP_COLLECTION, QVariant::fromValue( imapCollection ) );
  connect( cjob, SIGNAL(result(KJob*)), SLOT(imapItemCreationResult(KJob*)) );
}

void KolabProxyResource::imapItemUpdateResult( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
    return;
  }
  const Akonadi::Item kolabItem = job->property( KOLAB_ITEM ).value<Akonadi::Item>();
  changeCommitted( kolabItem );
}

void KolabProxyResource::itemMoved( const Akonadi::Item &item,
                                    const Akonadi::Collection &collectionSource,
                                    const Akonadi::Collection &collectionDestination )
{
  Q_UNUSED( collectionSource );
  new Akonadi::ItemMoveJob( kolabToImap( item ), kolabToImap( collectionDestination ), this );
  changeCommitted( item );
}

void KolabProxyResource::itemRemoved( const Akonadi::Item &item )
{
  kDebug() << "ITEMREMOVED";
  kDebug() << "Item removed " << item.id() << item.remoteId();
  const Akonadi::Item imapItem( item.remoteId().toUInt() );
  Akonadi::ItemDeleteJob *djob = new Akonadi::ItemDeleteJob( imapItem );
  changeCommitted( item );
  Q_UNUSED(djob);
}

void KolabProxyResource::collectionAdded( const Akonadi::Collection &collection,
                                          const Akonadi::Collection &parent )
{
  if ( KolabHandler::kolabTypeForMimeType( collection.contentMimeTypes() ).isEmpty() ) {
    kWarning() << "Collection " << collection.name() << collection.id() << collection.isValid()
               << "doesn't have kolab type set. isValid = "
               << "; parent is " << parent.name() << parent.id() << parent.isValid();
    cancelTask( QLatin1String( "Collection doesn't have kolab type." ) );
    Q_ASSERT_X( false, "collectionAdded", "Collection doesn't have kolab type set. Crashing..." );
    return;
  }

  Akonadi::Collection imapCollection( collection );
  imapCollection.setId( -1 );
  imapCollection.setRemoteId( QString() );
  imapCollection.setContentMimeTypes( QStringList()
                                      << Akonadi::Collection::mimeType()
                                      << QLatin1String( "message/rfc822" ) );
  const Akonadi::Collection imapParent = kolabToImap( parent );
  imapCollection.setParentCollection( imapParent );

  Akonadi::CollectionAnnotationsAttribute *attr =
    imapCollection.attribute<Akonadi::CollectionAnnotationsAttribute>(
      Akonadi::Collection::AddIfMissing );

  QMap<QByteArray, QByteArray> annotations = attr->annotations();

  annotations[KOLAB_FOLDER_TYPE_ANNOTATION] =
    KolabHandler::kolabTypeForMimeType( collection.contentMimeTypes() );

  attr->setAnnotations( annotations );

  Akonadi::CollectionCreateJob *job = new Akonadi::CollectionCreateJob( imapCollection, this );
  job->setProperty( KOLAB_COLLECTION, QVariant::fromValue( collection ) );
  connect( job, SIGNAL(result(KJob*)), SLOT(imapFolderCreateResult(KJob*)) );
}

void KolabProxyResource::imapFolderCreateResult( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorText() );
  } else {
    const Akonadi::Collection imapCollection =
      qobject_cast<Akonadi::CollectionCreateJob*>( job )->collection();

    registerHandlerForCollection( imapCollection );
    Akonadi::Collection kolabCollection =
      job->property( KOLAB_COLLECTION ).value<Akonadi::Collection>();
    kolabCollection.setRemoteId( QString::number( imapCollection.id() ) );
    changeCommitted( kolabCollection );
  }
}

void KolabProxyResource::applyAttributesToImap( Akonadi::Collection &imapCollection,
                                                const Akonadi::Collection &kolabCollection )
{
  static const Akonadi::EntityDisplayAttribute eda;
  static const Akonadi::EntityHiddenAttribute hidden;
  foreach ( const Akonadi::Attribute *attr, kolabCollection.attributes() ) {
    if ( attr->type() == hidden.type() ) {
      // Don't propagate HIDDEN because that would hide collections in korg, kab too.
      continue;
    }

    if ( attr->type() == eda.type() ) {
      // Don't propagate DISPLAYATTRIBUTE because that would cause icons
      // from the imap resource to use kolab icons.
      Akonadi::EntityDisplayAttribute *imapEda =
        imapCollection.attribute<Akonadi::EntityDisplayAttribute>( Akonadi::Entity::AddIfMissing );

      const QString dName =
        static_cast<const Akonadi::EntityDisplayAttribute*>( attr )->displayName();
      imapEda->setDisplayName( dName );
      continue;
    }

    if ( attr->type() == "AccessRights" ) {
      continue;
    }

    //kDebug() << "cloning" << attr->type();
    imapCollection.addAttribute( attr->clone() );
  }
}

void KolabProxyResource::applyAttributesFromImap( Akonadi::Collection &kolabCollection,
                                                  const Akonadi::Collection &imapCollection )
{
  static const Akonadi::EntityDisplayAttribute eda;
  static const Akonadi::EntityHiddenAttribute hidden;
  foreach ( const Akonadi::Attribute *attr, imapCollection.attributes() ) {
    if ( attr->type() == hidden.type() ) {
      continue;
    }

    if ( attr->type() == eda.type() ) {
      continue;
    }

    if ( attr->type() == "AccessRights" ) {
      continue;
    }

    //kDebug() << "cloning" << attr->type();
    kolabCollection.addAttribute( attr->clone() );
  }
}

void KolabProxyResource::updateFreeBusyInformation( const Akonadi::Collection &imapCollection )
{
  const Akonadi::CollectionAnnotationsAttribute *annotationsAttribute =
    imapCollection.attribute<Akonadi::CollectionAnnotationsAttribute>();

  if ( annotationsAttribute ) {
    const QMap<QByteArray, QByteArray> annotations = annotationsAttribute->annotations();
    const QByteArray folderType = annotations[ KOLAB_FOLDER_TYPE_ANNOTATION ];
    if ( folderType != KOLAB_FOLDER_TYPE_EVENT &&
         folderType != KOLAB_FOLDER_TYPE_EVENT KOLAB_FOLDER_TYPE_DEFAULT_SUFFIX ) {
      return; // no kolab calendar collection
    }
  } else {
    return; // no kolab collection
  }

  if ( !Settings::self()->updateFreeBusy() ) {
    return; // disabled by user
  }

  Kolab::Version v = readKolabVersion( imapCollection.resource() );
  if (v != Kolab::KolabV2) {
    return;
  }

  const QString path = mailBoxForImapCollection( imapCollection, true );
  if ( path.isEmpty() ) {
    return;
  }

  const QString resourceId = imapCollection.resource();

  QDBusInterface settingsInterface(
    QString::fromLatin1( "org.freedesktop.Akonadi.Agent.%1" ).arg( resourceId ),
    QLatin1String( "/Settings" ), QLatin1String( "org.kde.Akonadi.Imap.Settings" ) );

  QDBusInterface walletInterface(
    QString::fromLatin1( "org.freedesktop.Akonadi.Agent.%1" ).arg( resourceId ),
    QLatin1String( "/Settings" ), QLatin1String( "org.kde.Akonadi.Imap.Wallet" ) );

  if ( !settingsInterface.isValid() || !walletInterface.isValid() ) {
    kWarning() << "unable to retrieve imap resource settings interface";
    return;
  }

  const QDBusReply<QString> userNameReply = settingsInterface.call( QLatin1String( "userName" ) );
  if ( !userNameReply.isValid() ) {
    kWarning() << "unable to retrieve user name from imap resource settings";
    return;
  }

  const QDBusReply<QString> passwordReply = walletInterface.call( QLatin1String( "password" ) );
  if ( !passwordReply.isValid() ) {
    kWarning() << "unable to retrieve password from imap resource settings";
    return;
  }

  const QDBusReply<QString> hostReply = settingsInterface.call( QLatin1String( "imapServer" ) );
  if ( !hostReply.isValid() ) {
    kWarning() << "unable to retrieve host from imap resource settings";
    return;
  }

  m_freeBusyUpdateHandler->updateFolder( path,
                                         userNameReply.value(),
                                         passwordReply.value(),
                                         hostReply.value() );
}

void KolabProxyResource::collectionChanged( const Akonadi::Collection &collection )
{
  Akonadi::Collection imapCollection;
  imapCollection.setId( collection.remoteId().toLongLong() );
  imapCollection.setName( collection.name() );
  imapCollection.setCachePolicy( collection.cachePolicy() );
  imapCollection.setRights( collection.rights() );

  applyAttributesToImap( imapCollection, collection );

  Akonadi::CollectionModifyJob *job = new Akonadi::CollectionModifyJob( imapCollection, this );
  Q_UNUSED( job );
  // TODO wait for the result
  changeCommitted( collection );
}

void KolabProxyResource::collectionMoved( const Akonadi::Collection &collection,
                                          const Akonadi::Collection &source,
                                          const Akonadi::Collection &destination )
{
  Q_UNUSED( source );
  new Akonadi::CollectionMoveJob( kolabToImap( collection ), kolabToImap( destination ), this );
  changeCommitted( collection );
}

void KolabProxyResource::collectionRemoved( const Akonadi::Collection &collection )
{
  Akonadi::Collection imapCollection = kolabToImap( collection );

  Akonadi::CollectionDeleteJob *job = new Akonadi::CollectionDeleteJob( imapCollection, this );
  Q_UNUSED( job );
  // TODO wait for result
  changeCommitted( collection );
}

void KolabProxyResource::deleteImapItem( const Akonadi::Item &item )
{
  kDebug() << "DELETEIMAPITEM";
  Akonadi::ItemDeleteJob *djob = new Akonadi::ItemDeleteJob( item );
  Q_UNUSED( djob );
}

void KolabProxyResource::addImapItem( const Akonadi::Item &item,
                                      Akonadi::Entity::Id collectionId )
{
  kDebug() << "ADDITEMTOIMAP";
  new Akonadi::ItemCreateJob( item, Akonadi::Collection( collectionId ) );
}

void KolabProxyResource::imapItemAdded( const Akonadi::Item &item,
                                        const Akonadi::Collection &collection )
{
  kDebug() << item.id() << collection.id() << Akonadi::Collection::root().id();
  if ( m_excludeAppend.contains( item.id() ) )   {
    kDebug() << "item already present";
    m_excludeAppend.removeAll( item.id() );
    return;
  }
  //TODO: slow, would be nice if ItemCreateJob would work with a Collection
  //      having only the remoteId set
  const Akonadi::Collection kolabCol = imapToKolab( collection );
  Akonadi::CollectionFetchJob *job =
    new Akonadi::CollectionFetchJob( kolabCol, Akonadi::CollectionFetchJob::Base, this );
  connect( job, SIGNAL(result(KJob*)), this, SLOT(collectionFetchDone(KJob*)) );
  m_ids[job] = QString::number( collection.id() );
  m_items[job] = item;
}

void KolabProxyResource::collectionFetchDone( KJob *job )
{
  if ( job->error() ) {
    kWarning( ) << "Error on collection fetch:" << job->errorText();
  } else {
    Akonadi::Collection c;
    Akonadi::Collection::List collections =
      qobject_cast<Akonadi::CollectionFetchJob*>(job)->collections();
    foreach ( const Akonadi::Collection &col, collections ) {
      if ( col.remoteId() == m_ids[job] ) {
        c = col;
        break;
      }
    }

    KolabHandler::Ptr handler = m_monitoredCollections.value( c.remoteId().toUInt() );
    if ( !handler ) {
      kWarning() << "No handler found";
      m_ids.remove( job );
      m_items.remove( job );
      return;
    }

    Akonadi::Item::List newItems = handler->translateItems( Akonadi::Item::List() << m_items[job] );
    if ( !newItems.isEmpty() ) {
      Akonadi::ItemCreateJob *cjob = new Akonadi::ItemCreateJob( newItems[0], c );
      connect( cjob, SIGNAL(result(KJob*)), this, SLOT(itemCreatedDone(KJob*)) );
    }
  }
  m_ids.remove( job );
  m_items.remove( job );
}

void KolabProxyResource::itemCreatedDone( KJob *job )
{
  if ( job->error() ) {
    kWarning( ) << "Error on creating item:" << job->errorText();
  } else {
    //?
  }
}

void KolabProxyResource::imapItemRemoved( const Akonadi::Item &item )
{
  kDebug() << "IMAPITEMREMOVED";
  const Akonadi::Item kolabItem = imapToKolab( item );
  Q_FOREACH ( KolabHandler::Ptr handler, m_monitoredCollections ) {
    handler->itemDeleted( item );
  }
  Akonadi::ItemDeleteJob *job = new Akonadi::ItemDeleteJob( kolabItem, this );
  Q_UNUSED( job );
}

void KolabProxyResource::imapItemMoved( const Akonadi::Item &item,
                                        const Akonadi::Collection &collectionSource,
                                        const Akonadi::Collection &collectionDestination )
{
  kDebug();
  Q_UNUSED( collectionSource );
  new Akonadi::ItemMoveJob( imapToKolab( item ), imapToKolab( collectionDestination ), this );
}

void KolabProxyResource::imapCollectionAdded( const Akonadi::Collection &collection,
                                              const Akonadi::Collection &parent )
{
  Q_UNUSED( parent );
  if ( collection.resource() == identifier() ) {
    // just to be sure...
    return;
  }

  kDebug() << "IMAPCOLLECTIONADDED";
  if ( m_monitoredCollections.contains( collection.id() ) ) {
    // something is wrong, so better reload out collection tree
    kDebug() << "IMAPCOLLECTIONADDED ABORT";
    synchronizeCollectionTree();
    return;
  }

  if ( registerHandlerForCollection( collection ) ) {
    const Akonadi::Collection kolabCollection = createCollection( collection );
    Akonadi::CollectionCreateJob *job = new Akonadi::CollectionCreateJob( kolabCollection, this );
    connect( job, SIGNAL(result(KJob*)), SLOT(kolabFolderChangeResult(KJob*)) );
  }
}

void KolabProxyResource::imapCollectionChanged( const Akonadi::Collection &collection )
{
  if ( collection.resource() == identifier() ) {
    // just to be sure...
    return;
  }

  //kDebug() << "IMAPCOLLECTIONCHANGED";
  if ( !m_monitoredCollections.contains( collection.id() ) ) {
    // check if this is a Kolab folder at all, if yet something is wrong
    Akonadi::CollectionAnnotationsAttribute *annotationsAttribute =
      collection.attribute<Akonadi::CollectionAnnotationsAttribute>();
    bool isKolabFolder = false;
    if ( annotationsAttribute ) {
      const QMap<QByteArray, QByteArray> annotations = annotationsAttribute->annotations();
      QByteArray folderType = annotations[ KOLAB_FOLDER_TYPE_ANNOTATION ];
      isKolabFolder = !folderType.isEmpty() && folderType != KOLAB_FOLDER_TYPE_MAIL;
    }

    if ( isKolabFolder ) {
      synchronizeCollectionTree();
      return;
    }
    // not a Kolab folder, no need to resync the tree.
    // just try to update a possible structural collection.
    // if that fails it's not in our tree -> we don't care
    Akonadi::Collection kolabCollection = createCollection( collection );
    Akonadi::CollectionModifyJob *job = new Akonadi::CollectionModifyJob( kolabCollection, this );
    Q_UNUSED( job );
  } else {
    // Kolab folder we already have in our tree, if the update fails, reload our tree
    Akonadi::Collection kolabCollection = createCollection( collection );
    Akonadi::CollectionModifyJob *job = new Akonadi::CollectionModifyJob( kolabCollection, this );
    connect( job, SIGNAL(result(KJob*)), SLOT(kolabFolderChangeResult(KJob*)) );
  }

  updateFreeBusyInformation( collection );
}

void KolabProxyResource::imapCollectionMoved( const Akonadi::Collection &collection,
                                              const Akonadi::Collection &source,
                                              const Akonadi::Collection &destination )
{
  kDebug();
  Q_UNUSED( source );
  new Akonadi::CollectionMoveJob( imapToKolab( collection ), imapToKolab( destination ), this );
}

void KolabProxyResource::kolabFolderChangeResult( KJob *job )
{
  if ( job->error() ) {
    // something went wrong or the change was too complex to handle in the above slots,
    // so re-sync the entire tree.
    kDebug() << "Re-syncing collection tree as incremental changes did not succeed."
             << job->errorText();
    synchronizeCollectionTree();
  }
}

void KolabProxyResource::imapCollectionRemoved( const Akonadi::Collection &imapCollection )
{
  if ( imapCollection.resource() == identifier() ) {
    // just to be sure...
    return;
  }

  kDebug() << "IMAPCOLLECTIONREMOVED";
  Akonadi::Collection kolabCollection;
  kolabCollection.setRemoteId( QString::number( imapCollection.id() ) );
  new Akonadi::CollectionDeleteJob( kolabCollection );

  m_monitoredCollections.remove( imapCollection.id() );

  updateFreeBusyInformation( imapCollection );
}

Akonadi::Collection KolabProxyResource::createCollection(
  const Akonadi::Collection &imapCollection )
{
  Akonadi::Collection c;
  QStringList contentTypes;
  if ( imapCollection.parentCollection() == Akonadi::Collection::root() ) {
    c.setParentCollection( Akonadi::Collection::root() );
    Akonadi::CachePolicy policy;
    policy.setInheritFromParent( false );
    policy.setCacheTimeout( -1 );
    policy.setLocalParts( QStringList() << QLatin1String( "ALL" ) );
    c.setCachePolicy( policy );
  } else {
    c.parentCollection().setRemoteId( QString::number( imapCollection.parentCollection().id() ) );
  }
  c.setName( imapCollection.name() );
  c.setRights( imapCollection.rights() );

  Akonadi::EntityDisplayAttribute *imapAttr =
    imapCollection.attribute<Akonadi::EntityDisplayAttribute>();
  Akonadi::EntityDisplayAttribute *kolabAttr =
    c.attribute<Akonadi::EntityDisplayAttribute>( Akonadi::Collection::AddIfMissing );

  if ( imapAttr ) {
    if ( imapAttr->iconName() == QLatin1String( "mail-folder-inbox" ) ) {
      kolabAttr->setDisplayName( i18n( "My Data" ) );
      kolabAttr->setIconName( QLatin1String( "view-pim-summary" ) );
      //contentTypes << KolabHandler::allSupportedMimeTypes();
      c.setRights( Akonadi::Collection::ReadOnly | Akonadi::Collection::CanCreateCollection );
    } else if ( imapCollection.parentCollection() == Akonadi::Collection::root() ) {
      c.setName( i18n( "Kolab (%1)", imapAttr->displayName() ) );
      kolabAttr->setIconName( QLatin1String( "kolab" ) );
    } else {
      kolabAttr->setDisplayName( imapAttr->displayName() );
      kolabAttr->setIconName( imapAttr->iconName() );
    }
  } else {
    if ( imapCollection.parentCollection() == Akonadi::Collection::root() ) {
      c.setName( i18n( "Kolab (%1)", imapCollection.name() ) );
      kolabAttr->setIconName( QLatin1String( "kolab" ) );
    }
  }
  applyAttributesFromImap( c, imapCollection );
  KolabHandler::Ptr handler = m_monitoredCollections.value( imapCollection.id() );
  contentTypes.append( Akonadi::Collection::mimeType() );
  if ( handler ) {
    contentTypes.append( handler->contentMimeTypes() );
    kolabAttr->setIconName( handler->iconName() );

    // hide Kolab folders on the IMAP server
    if ( !imapCollection.hasAttribute<Akonadi::EntityHiddenAttribute>() ) {
      Akonadi::Collection hiddenImapCol( imapCollection );
      hiddenImapCol.attribute<Akonadi::EntityHiddenAttribute>( Akonadi::Collection::AddIfMissing );
      new Akonadi::CollectionModifyJob( hiddenImapCol, this );
    }
  }
  c.setContentMimeTypes( contentTypes );
  c.setRemoteId( QString::number( imapCollection.id() ) );
  return c;
}

bool KolabProxyResource::registerHandlerForCollection( const Akonadi::Collection &imapCollection )
{
  Akonadi::CollectionAnnotationsAttribute *annotationsAttribute =
    imapCollection.attribute<Akonadi::CollectionAnnotationsAttribute>();
  if ( annotationsAttribute ) {
    QMap<QByteArray, QByteArray> annotations = annotationsAttribute->annotations();

    KolabHandler::Ptr handler =
      KolabHandler::createHandler(
        annotations[KOLAB_FOLDER_TYPE_ANNOTATION], imapCollection );

    if ( handler ) {
      Kolab::Version v = readKolabVersion( imapCollection.resource() );
      handler->setKolabFormatVersion( v );
      connect( handler.data(), SIGNAL(deleteItemFromImap(Akonadi::Item)),
               this, SLOT(deleteImapItem(Akonadi::Item)));
      connect( handler.data(), SIGNAL(addItemToImap(Akonadi::Item,Akonadi::Entity::Id)),
               this, SLOT(addImapItem(Akonadi::Item,Akonadi::Entity::Id)));
      m_monitor->setCollectionMonitored( imapCollection );
      m_monitoredCollections.insert( imapCollection.id(), handler );
      m_resourceIdentifier.insert( imapCollection.id(), imapCollection.resource() );
      return true;
    }
  }

  return false;
}

AKONADI_RESOURCE_MAIN( KolabProxyResource )

#include "kolabproxyresource.moc"
