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
#include "itemaddedjob.h"
#include "itemchangedjob.h"
#include "revertitemchangesjob.h"
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
#include <Akonadi/ItemDeleteJob>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemMoveJob>
#include <Akonadi/Session>

#include <KLocalizedString>
#include <KWindowSystem>
#include <KNotification>
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
  : ResourceBase( id ),
  mHandlerManager( new HandlerManager )
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

void KolabProxyResource::showErrorMessage(const QString &msg)
{
  KNotification *notification = new KNotification(QLatin1String("Error"), KNotification::CloseOnTimeout, 0);
  notification->setText(msg);
  notification->setComponentData(KGlobal::mainComponent());
  notification->sendEvent();
}

bool KolabProxyResource::registerHandlerForCollection(const Akonadi::Collection& imapCollection)
{
    const Kolab::Version v = SetupKolab::readKolabVersion( imapCollection.resource() );
    if ( mHandlerManager->registerHandlerForCollection( imapCollection, v ) ) {
        m_monitor->setCollectionMonitored( imapCollection );
        return true;
    }
    return false;
}

QString KolabProxyResource::imapResourceForCollection( Akonadi::Entity::Id  id)
{
    return mHandlerManager->imapResourceForCollection( id );
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
  if ( !mHandlerManager->isMonitored( imapCollection.id() ) ) {
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
  const KolabHandler::Ptr handler = mHandlerManager->getHandler( items[0].storageCollectionId() );
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
  const KolabHandler::Ptr handler = mHandlerManager->getHandler( items[0].storageCollectionId() );
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
  mHandlerManager->clear();
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

  foreach ( Akonadi::Entity::Id id, mHandlerManager->monitoredCollections() ) { //krazy:exclude=foreach
    KolabHandler::Ptr handler = mHandlerManager->getHandler( id );
    Kolab::Version v = SetupKolab::readKolabVersion( mHandlerManager->imapResourceForCollection( id ) );
    handler->setKolabFormatVersion( v );
  }

  delete kolabConfigDialog;
}

void KolabProxyResource::itemAdded( const Akonadi::Item &kolabItem,
                                    const Akonadi::Collection &collection )
{
  const Akonadi::Collection imapCollection = kolabToImap( collection );
  const KolabHandler::Ptr handler = mHandlerManager->getHandler( imapCollection.id() );
  if ( !handler ) {
    kWarning() << "Couldn't find a handler for the collection, but we should have one: " << imapCollection.id();
    showErrorMessage(i18n("An error occured while writing the item to the backend."));
    cancelTask();
    new Akonadi::ItemDeleteJob(kolabItem);
    return;
  }
  ItemAddedJob *itemAddedJob = new ItemAddedJob(kolabItem, collection, *handler, this);
  connect(itemAddedJob, SIGNAL(result(KJob*)), this, SLOT(onItemAddedDone(KJob*)));
  itemAddedJob->start();
}

void KolabProxyResource::onItemAddedDone(KJob* job)
{
  ItemAddedJob *itemAddedJob = static_cast<ItemAddedJob*>(job);
  Akonadi::Item kolabItem = itemAddedJob->kolabItem();
  const Akonadi::Item imapItem = itemAddedJob->imapItem();
  if (job->error()) {
    kWarning() << "Failed to create imap item: " << job->errorString();
    showErrorMessage(i18n("An error occured while writing the item to the backend."));
    cancelTask();
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
  ItemChangedJob *itemChangedJob = new ItemChangedJob(kolabItem, *mHandlerManager, this);
  connect(itemChangedJob, SIGNAL(result(KJob*)), this, SLOT(onItemChangedDone(KJob*)));
  itemChangedJob->start();
}

void KolabProxyResource::onItemChangedDone(KJob* job)
{
  ItemChangedJob *itemChangedJob = static_cast<ItemChangedJob*>(job);
  if ( job->error() ) {
    showErrorMessage(i18n("An error occured while writing the item to the backend."));
    cancelTask( job->errorText() );
    kWarning() << "Failed to modify item, reverting to state of imap item: " << itemChangedJob->item().id();
    RevertItemChangesJob *revertJob = new RevertItemChangesJob(itemChangedJob->item(), *mHandlerManager, this);
    connect(revertJob, SIGNAL(result(KJob*)), this, SLOT(checkResult(KJob*)));
    revertJob->start();
    return;
  }
  changeCommitted( itemChangedJob->item() );
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
      // Don't propagate access rights here, since it's already propagated using Collection::rights
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
  if ( !HandlerManager::isHandledKolabFolder( imapCollection ) ) {
    return;
  }

  if ( HandlerManager::getFolderType( imapCollection ) != Kolab::EventType ) {
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
  //We only want updates about collections that are not from this resource
  if ( collection.resource() == identifier() || collection.resource().startsWith("akonadi_kolab_resource") ) {
    return;
  }
  if ( m_excludeAppend.contains( item.id() ) )   {
    kDebug() << "item already present";
    m_excludeAppend.removeAll( item.id() );
    return;
  }
  if ( const KolabHandler::Ptr handler = mHandlerManager->getHandler( collection.id() ) ) {
    handler->imapItemAdded(item, collection);
  }
}

void KolabProxyResource::imapItemRemoved( const Akonadi::Item &item )
{
  //We only want updates about collections that are not from this resource
  if ( item.parentCollection().resource() == identifier() || item.parentCollection().resource().startsWith("akonadi_kolab_resource") ) {
    return;
  }
  if ( const KolabHandler::Ptr handler = mHandlerManager->getHandler( item.parentCollection().id() ) ) {
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
  //We only want updates about collections that are not from this resource
  if ( collectionSource.resource() == identifier() || collectionDestination.resource() == identifier() ||
       collectionSource.resource().startsWith("akonadi_kolab_resource") || collectionDestination.resource().startsWith("akonadi_kolab_resource") ) {
    return;
  }
  KJob *job = new Akonadi::ItemMoveJob( imapToKolab( item ), imapToKolab( collectionDestination ), this );
  connect(job, SIGNAL(result(KJob*)), this, SLOT(checkResult(KJob*)));
}

void KolabProxyResource::imapCollectionAdded( const Akonadi::Collection &collection,
                                              const Akonadi::Collection &parent )
{
  Q_UNUSED( parent );
  //We only want updates about collections that are not from this resource
   if ( collection.resource() == identifier() || collection.resource().startsWith("akonadi_kolab_resource") ) {
    return;
  }
  if ( mHandlerManager->isMonitored( collection.id() ) ) {
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

void KolabProxyResource::imapCollectionChanged( const Akonadi::Collection &collection )
{
  //We only want updates about collections that are not from this resource
  if ( collection.resource() == identifier() || collection.resource().startsWith("akonadi_kolab_resource") ) {
    return;
  }

  if ( !mHandlerManager->isMonitored( collection.id() ) ) {
    if ( HandlerManager::isHandledKolabFolder( collection ) ) {
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
    if ( !HandlerManager::isHandledKolabFolder( collection ) ) {
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
  //We only want updates about collections that are not from this resource
  if ( source.resource() == identifier() || destination.resource() == identifier() ||
       source.resource().startsWith("akonadi_kolab_resource") || destination.resource().startsWith("akonadi_kolab_resource") ) {
    return;
  }
  if ( mHandlerManager->isMonitored( collection.id() ) ) {
    KJob *job = new Akonadi::CollectionMoveJob( imapToKolab( collection ), imapToKolab( destination ), this );
    connect(job, SIGNAL(result(KJob*)), this, SLOT(checkResult(KJob*)));
  }
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
  mHandlerManager->removeFolder( imapCollection.id() );
  updateFreeBusyInformation( imapCollection );
}

void KolabProxyResource::imapCollectionRemoved( const Akonadi::Collection &imapCollection )
{
  //We only want updates about collections that are not from this resource
  if ( imapCollection.resource() == identifier() || imapCollection.resource().startsWith("akonadi_kolab_resource") ) {
    return;
  }
  if (mHandlerManager->isMonitored( imapCollection.id())) {
    removeFolder(imapCollection);
  } else if ( imapCollection.parentCollection() == Akonadi::Collection::root() ) {
    //we are not explicitly monitoring the top-level collection, but it should be removed anyways when the rest is gone
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
  if ( HandlerManager::isKolabFolder( imapCollection ) ) {
    KolabHandler::Ptr handler = mHandlerManager->getHandler( imapCollection.id() );
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

void KolabProxyResource::updateHiddenAttribute( const Akonadi::Collection &imapCollection )
{
  if ( HandlerManager::isKolabFolder( imapCollection ) && !imapCollection.hasAttribute<Akonadi::EntityHiddenAttribute>()) {
      Akonadi::Collection hiddenImapCol( imapCollection );
      hiddenImapCol.attribute<Akonadi::EntityHiddenAttribute>( Akonadi::Collection::AddIfMissing );
      KJob *job = new Akonadi::CollectionModifyJob( hiddenImapCol, this );
      connect( job, SIGNAL(result(KJob*)), this, SLOT(checkResult(KJob*)) );
  }
  if ( !HandlerManager::isKolabFolder( imapCollection ) && imapCollection.hasAttribute<Akonadi::EntityHiddenAttribute>()) {
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
