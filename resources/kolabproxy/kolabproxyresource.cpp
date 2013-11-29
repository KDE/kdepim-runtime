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
#include "kolabproxyadaptor.h"
#include "setupkolab.h"
#include "imapitemaddedjob.h"
#include "imapitemremovedjob.h"
#include <akonadi/dbusconnectionpool.h>

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
    return QLatin1String( "" );
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

  new KolabproxyAdaptor( this );
  Akonadi::DBusConnectionPool::threadConnection().registerObject( QLatin1String( "/KolabProxy" ), this, QDBusConnection::ExportAdaptors );
  Akonadi::DBusConnectionPool::threadConnection().registerService( QLatin1String( "Agent.akonadi_kolabproxy_resource" ) );

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

KolabHandler::Ptr KolabProxyResource::getHandler(Akonadi::Entity::Id collectionId)
{
  return m_monitoredCollections.value(collectionId);
}

void KolabProxyResource::retrieveCollections()
{
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
  const Akonadi::Collection imapCollection = kolabToImap( collection );
  if ( !m_monitoredCollections.contains( imapCollection.id() ) ) {
    //This should never happen
    kWarning() << "received a retrieveItems request for a collection without imap counterpart" << collection.id();
    cancelTask();
    return;
  }
  
  Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob( imapCollection );
  job->fetchScope().fetchFullPayload();
  job->fetchScope().setIgnoreRetrievalErrors( true );

  connect( job, SIGNAL(result(KJob*)), this, SLOT(retrieveItemsFetchDone(KJob*)) );
}

void KolabProxyResource::retrieveItemsFetchDone( KJob *job )
{
  if ( job->error() ) {
    kWarning( ) << "Error on item fetch:" << job->errorText();
    cancelTask();
    return;
  }

  const Akonadi::Item::List items = qobject_cast<Akonadi::ItemFetchJob*>(job)->items();
  if ( items.isEmpty() ) {
    itemsRetrieved( Akonadi::Item::List() );
    return;
  }
  const KolabHandler::Ptr handler = getHandler( items[0].storageCollectionId() );
  if ( !handler ) {
    cancelTask();
    return;
  }
  const Akonadi::Item::List newItems = handler->resolveConflicts( handler->translateItems( items ) );

  itemsRetrieved( newItems );
}

bool KolabProxyResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );
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
  const Akonadi::Item::List items = qobject_cast<Akonadi::ItemFetchJob*>(job)->items();
  if ( items.isEmpty() ) {
    kWarning() << "Items is emtpy";
    cancelTask();
    return;
  }
  const KolabHandler::Ptr handler = getHandler( items[0].storageCollectionId() );
  if ( !handler ) {
    cancelTask();
    return;
  }
  const Akonadi::Item::List newItems = handler->translateItems( items );
  if ( newItems.isEmpty() ) {
    kWarning() << "Could not translate item";
    cancelTask();
    return;
  }
  Akonadi::Item item = newItems[0];
  item.setId(job->property("itemId").value<Akonadi::Item::Id>());
  itemRetrieved( item );
}

void KolabProxyResource::aboutToQuit()
{
  m_monitoredCollections.clear();
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

  kolabConfigDialog->setWindowIcon( KIcon( QLatin1String("kolab") ) );
  kolabConfigDialog->exec();
  emit configurationDialogAccepted();

  foreach ( Akonadi::Entity::Id id, m_monitoredCollections.keys() ) { //krazy:exclude=foreach
    KolabHandler::Ptr handler = m_monitoredCollections.value( id );
    Kolab::Version v = SetupKolab::readKolabVersion( m_resourceIdentifier.value( id ) );
    handler->setKolabFormatVersion( v );
  }

  delete kolabConfigDialog;
}

void KolabProxyResource::itemAdded( const Akonadi::Item &kolabItem,
                                    const Akonadi::Collection &collection )
{
  const Akonadi::Collection imapCollection = kolabToImap( collection );
  createItem( imapCollection, kolabItem );
}

void KolabProxyResource::createItem( const Akonadi::Collection &imapCollection, const Akonadi::Item &kolabItem )
{
  const KolabHandler::Ptr handler = getHandler( imapCollection.id() );
  if ( !handler ) {
    kWarning() << "could find a handler for the collection, but we should have one";
    cancelTask();
    new Akonadi::ItemDeleteJob(kolabItem);
    return;
  }
  Akonadi::Item imapItem( "message/rfc822" );
  if (!handler->toKolabFormat( kolabItem, imapItem )) {
    kWarning() << "Failed to convert item to kolab format: " << kolabItem.id();
    cancelTask();
    new Akonadi::ItemDeleteJob(kolabItem);
    return;
  }
  imapItem.setFlag( Akonadi::MessageFlags::Seen );

  Akonadi::ItemCreateJob *cjob = new Akonadi::ItemCreateJob( imapItem, imapCollection );
  cjob->setProperty( KOLAB_ITEM, QVariant::fromValue( kolabItem ) );
  connect( cjob, SIGNAL(result(KJob*)), SLOT(imapItemCreationResult(KJob*)) );
}

void KolabProxyResource::imapItemCreationResult( KJob *job )
{
  Akonadi::ItemCreateJob *cjob = static_cast<Akonadi::ItemCreateJob*>( job );
  const Akonadi::Item imapItem = cjob->item();
  Akonadi::Item kolabItem = cjob->property( KOLAB_ITEM ).value<Akonadi::Item>();

  if ( job->error() ) {
    cancelTask( job->errorText() );
    new Akonadi::ItemDeleteJob(kolabItem);
    return;
  }

  m_excludeAppend << imapItem.id();

  kolabItem.setRemoteId( QString::number( imapItem.id() ) );
  changeCommitted( kolabItem );
}

void KolabProxyResource::itemChanged( const Akonadi::Item &kolabItem,
                                      const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );
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
  if (fetchJob->items().isEmpty()) { //The corresponding imap item hasn't been created yet
    Akonadi::CollectionFetchJob *fetch =
      new Akonadi::CollectionFetchJob( Akonadi::Collection( kolabItem.storageCollectionId() ),
                                       Akonadi::CollectionFetchJob::Base, this );
    fetch->setProperty( KOLAB_ITEM, QVariant::fromValue( kolabItem ) );
    connect( fetch, SIGNAL(result(KJob*)), SLOT(imapItemUpdateCollectionFetchResult(KJob*)) );
  } else {
    Akonadi::Item imapItem = fetchJob->items().first();

    const KolabHandler::Ptr handler = getHandler( imapItem.storageCollectionId() );
    if ( !handler ) {
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
  }
}

void KolabProxyResource::imapItemUpdateCollectionFetchResult( KJob *job )
{
  Akonadi::CollectionFetchJob *fetchJob = qobject_cast<Akonadi::CollectionFetchJob*>( job );
  if ( job->error() || fetchJob->collections().isEmpty() ) {
    cancelTask( job->errorText() );
    return;
  }

  const Akonadi::Item kolabItem = job->property( KOLAB_ITEM ).value<Akonadi::Item>();
  const Akonadi::Collection kolabCollection = fetchJob->collections().first();
  const Akonadi::Collection imapCollection = kolabToImap( kolabCollection );
  createItem( imapCollection, kolabItem );
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
  KJob *job = new Akonadi::ItemMoveJob( kolabToImap( item ), kolabToImap( collectionDestination ), this );
  connect(job, SIGNAL(result(KJob*)), this, SLOT(checkResult(KJob*)));
  changeCommitted( item );
}

void KolabProxyResource::itemRemoved( const Akonadi::Item &item )
{
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
  Kolab::setFolderTypeAnnotation( annotations, KolabHandler::kolabTypeForMimeType( collection.contentMimeTypes() ) );
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

    kolabCollection.addAttribute( attr->clone() );
  }
}

void KolabProxyResource::updateFreeBusyInformation( const Akonadi::Collection &imapCollection )
{
  if ( !isHandledKolabFolder( imapCollection ) ) {
    return;
  }

  if ( getFolderType( imapCollection ) != Kolab::EventType ) {
    return;
  }

  if ( !Settings::self()->updateFreeBusy() ) {
    return; // disabled by user
  }

  Kolab::Version v = SetupKolab::readKolabVersion( imapCollection.resource() );
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
  KJob *job = new Akonadi::CollectionMoveJob( kolabToImap( collection ), kolabToImap( destination ), this );
  connect(job, SIGNAL(result(KJob*)), this, SLOT(checkResult(KJob*)));
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

void KolabProxyResource::imapItemAdded( const Akonadi::Item &item,
                                        const Akonadi::Collection &collection )
{
  if ( m_excludeAppend.contains( item.id() ) )   {
    kDebug() << "item already present";
    m_excludeAppend.removeAll( item.id() );
    return;
  }
  if ( const KolabHandler::Ptr handler = getHandler( collection.id() ) ) {
    handler->imapItemAdded(item, collection);
  }
}

void KolabProxyResource::imapItemRemoved( const Akonadi::Item &item )
{
  if ( const KolabHandler::Ptr handler = getHandler( item.parentCollection().id() ) ) {
    handler->imapItemRemoved(item);
  } else {
    //The handler is already gone,
    kWarning() << "Couldn't find handler for collection " << item.storageCollectionId();
    ImapItemRemovedJob *job = new ImapItemRemovedJob(item, this);
    connect(job, SIGNAL(result(KJob*)), this, SLOT(checkResult(KJob*)));
    job->start();
  }
}

void KolabProxyResource::imapItemMoved( const Akonadi::Item &item,
                                        const Akonadi::Collection &collectionSource,
                                        const Akonadi::Collection &collectionDestination )
{
  Q_UNUSED( collectionSource );
  KJob *job = new Akonadi::ItemMoveJob( imapToKolab( item ), imapToKolab( collectionDestination ), this );
  connect(job, SIGNAL(result(KJob*)), this, SLOT(checkResult(KJob*)));
}

void KolabProxyResource::imapCollectionAdded( const Akonadi::Collection &collection,
                                              const Akonadi::Collection &parent )
{
  Q_UNUSED( parent );
  //We are ignoring our session
  Q_ASSERT( collection.resource() != identifier() );
  if ( m_monitoredCollections.contains( collection.id() ) ) {
    // something is wrong, so better reload out collection tree
    kWarning() << "IMAPCOLLECTIONADDED ABORT";
    synchronizeCollectionTree();
    return;
  }

  updateHiddenAttribute( collection );

  if ( registerHandlerForCollection( collection ) ) {
    const Akonadi::Collection kolabCollection = createCollection( collection );
    Akonadi::CollectionCreateJob *job = new Akonadi::CollectionCreateJob( kolabCollection, this );
    connect( job, SIGNAL(result(KJob*)), SLOT(kolabFolderChangeResult(KJob*)) );
  }
}

Kolab::FolderType KolabProxyResource::getFolderType( const Akonadi::Collection& collection ) const
{
  Akonadi::CollectionAnnotationsAttribute *annotationsAttribute =
    collection.attribute<Akonadi::CollectionAnnotationsAttribute>();
  if ( annotationsAttribute ) {
    return Kolab::folderTypeFromString( Kolab::getFolderTypeAnnotation( annotationsAttribute->annotations() ) );
  }
  return Kolab::MailType;
}
 
bool KolabProxyResource::isKolabFolder(const Akonadi::Collection &collection) const
{
  return (getFolderType(collection) != Kolab::MailType);
}

bool KolabProxyResource::isHandledKolabFolder(const Akonadi::Collection& collection) const
{ 
  return KolabHandler::hasHandler(getFolderType(collection));
}

void KolabProxyResource::imapCollectionChanged( const Akonadi::Collection &collection )
{
  if ( collection.resource() == identifier() ) {
    // just to be sure...
    return;
  }

  if ( !m_monitoredCollections.contains( collection.id() ) ) {
    if ( isHandledKolabFolder( collection ) ) {
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
    if ( !isHandledKolabFolder( collection ) ) {
        //This is no longer a kolab folder, remove
        removeFolder( collection );
        return;
    }
    // Kolab folder we already have in our tree, if the update fails, reload our tree
    Akonadi::Collection kolabCollection = createCollection( collection );
    Akonadi::CollectionModifyJob *job = new Akonadi::CollectionModifyJob( kolabCollection, this );
    connect( job, SIGNAL(result(KJob*)), SLOT(kolabFolderChangeResult(KJob*)) );
  }

  updateHiddenAttribute( collection );
  updateFreeBusyInformation( collection );
}

void KolabProxyResource::imapCollectionMoved( const Akonadi::Collection &collection,
                                              const Akonadi::Collection &source,
                                              const Akonadi::Collection &destination )
{
  Q_UNUSED( source );
  KJob *job = new Akonadi::CollectionMoveJob( imapToKolab( collection ), imapToKolab( destination ), this );
  connect(job, SIGNAL(result(KJob*)), this, SLOT(checkResult(KJob*)));
}

void KolabProxyResource::kolabFolderChangeResult( KJob *job )
{
  if ( job->error() ) {
    // something went wrong or the change was too complex to handle in the above slots,
    // so re-sync the entire tree.
    kWarning() << "Re-syncing collection tree as incremental changes did not succeed."
             << job->errorText();
    synchronizeCollectionTree();
  }
}

void KolabProxyResource::removeFolder( const Akonadi::Collection &imapCollection )
{
  KJob *deleteJob = new Akonadi::CollectionDeleteJob( imapToKolab( imapCollection ) );
  connect(deleteJob, SIGNAL(result(KJob*)), this, SLOT(checkResult(KJob*)));
  m_monitoredCollections.remove( imapCollection.id() );
  updateFreeBusyInformation( imapCollection );
}

void KolabProxyResource::imapCollectionRemoved( const Akonadi::Collection &imapCollection )
{
  //we are ignoring our session
  Q_ASSERT( imapCollection.resource() != identifier() );
  if (m_monitoredCollections.contains( imapCollection.id())) {
    removeFolder(imapCollection);
  }
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
  if ( isKolabFolder( imapCollection ) ) {
    KolabHandler::Ptr handler = m_monitoredCollections.value( imapCollection.id() );
    if ( handler ) {
        contentTypes.append( handler->contentMimeTypes() );
        kolabAttr->setIconName( handler->iconName() );
    }
  }
  contentTypes.append( Akonadi::Collection::mimeType() );
  c.setContentMimeTypes( contentTypes );
  c.setRemoteId( QString::number( imapCollection.id() ) );
  return c;
}

bool KolabProxyResource::registerHandlerForCollection( const Akonadi::Collection &imapCollection )
{
  if ( isHandledKolabFolder( imapCollection ) ) {
    KolabHandler::Ptr handler =
      KolabHandler::createHandler( getFolderType( imapCollection ), imapCollection );

    if ( handler ) {
      Kolab::Version v = SetupKolab::readKolabVersion( imapCollection.resource() );
      handler->setKolabFormatVersion( v );

      m_monitor->setCollectionMonitored( imapCollection );
      m_monitoredCollections.insert( imapCollection.id(), handler );
      m_resourceIdentifier.insert( imapCollection.id(), imapCollection.resource() );
      return true;
    }
  }

  return false;
}

QString KolabProxyResource::imapResourceForCollection( Akonadi::Collection::Id id )
{
    if (m_resourceIdentifier.contains(id)) {
        return m_resourceIdentifier[id];
    }
    return QString();
}

void KolabProxyResource::updateHiddenAttribute( const Akonadi::Collection &imapCollection )
{
  if ( isKolabFolder( imapCollection ) && !imapCollection.hasAttribute<Akonadi::EntityHiddenAttribute>()) {
      Akonadi::Collection hiddenImapCol( imapCollection );
      hiddenImapCol.attribute<Akonadi::EntityHiddenAttribute>( Akonadi::Collection::AddIfMissing );
      KJob *job = new Akonadi::CollectionModifyJob( hiddenImapCol, this );
      connect( job, SIGNAL(result(KJob*)), this, SLOT(checkResult(KJob*)) );
  }
  if ( !isKolabFolder( imapCollection ) && imapCollection.hasAttribute<Akonadi::EntityHiddenAttribute>()) {
      Akonadi::Collection unhiddenImapCol( imapCollection );
      unhiddenImapCol.removeAttribute<Akonadi::EntityHiddenAttribute>();
      KJob *job = new Akonadi::CollectionModifyJob( unhiddenImapCol, this );
      connect( job, SIGNAL(result(KJob*)), this, SLOT(checkResult(KJob*)) );
  }
}

void KolabProxyResource::checkResult(KJob* job)
{
  if ( job->error() ) {
    kWarning() << "Error occurred: " << job->errorString();
  }
}

AKONADI_RESOURCE_MAIN( KolabProxyResource )
