/*
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>
    Copyright (C) 2009 Kevin Ottens <ervin@kde.org>

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

#include "imapresource.h"
#include <qglobal.h>
#include "setupserver.h"
#include "settings.h"
#include "uidvalidityattribute.h"
#include "uidnextattribute.h"
#include "noselectattribute.h"

#include <QtCore/QDebug>
#include <QtDBus/QDBusConnection>
#include <QtNetwork/QSslSocket>

#include <kdebug.h>
#include <klocale.h>
#include <kpassworddialog.h>
#include <kmessagebox.h>
#include <KWindowSystem>
#include <KAboutData>

#include <kimap/session.h>
#include <kimap/sessionuiproxy.h>

#include <kimap/appendjob.h>
#include <kimap/capabilitiesjob.h>
#include <kimap/createjob.h>
#include <kimap/deletejob.h>
#include <kimap/expungejob.h>
#include <kimap/fetchjob.h>
#include <kimap/getacljob.h>
#include <kimap/getmetadatajob.h>
#include <kimap/getquotarootjob.h>
#include <kimap/listjob.h>
#include <kimap/loginjob.h>
#include <kimap/logoutjob.h>
#include <kimap/myrightsjob.h>
#include <kimap/renamejob.h>
#include <kimap/selectjob.h>
#include <kimap/storejob.h>

#include <kmime/kmime_message.h>

typedef boost::shared_ptr<KMime::Message> MessagePtr;

#include <akonadi/attributefactory.h>
#include <akonadi/cachepolicy.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/collectionstatisticsjob.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/monitor.h>
#include <akonadi/changerecorder.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/session.h>
#include <akonadi/transactionsequence.h>

#include "collectionflagsattribute.h"
#include "collectionannotationsattribute.h"
#include "imapaclattribute.h"
#include "imapquotaattribute.h"

#include "imapaccount.h"

using namespace Akonadi;

ImapResource::ImapResource( const QString &id )
        :ResourceBase( id ), m_account( 0 )
{
  Akonadi::AttributeFactory::registerAttribute<UidValidityAttribute>();
  Akonadi::AttributeFactory::registerAttribute<UidNextAttribute>();
  Akonadi::AttributeFactory::registerAttribute<NoSelectAttribute>();
  Akonadi::AttributeFactory::registerAttribute<CollectionFlagsAttribute>();
  Akonadi::AttributeFactory::registerAttribute<CollectionAnnotationsAttribute>();
  Akonadi::AttributeFactory::registerAttribute<ImapAclAttribute>();
  Akonadi::AttributeFactory::registerAttribute<ImapQuotaAttribute>();

  changeRecorder()->fetchCollection( true );
  changeRecorder()->itemFetchScope().fetchFullPayload( true );

  startConnect();
}

ImapResource::~ImapResource()
{
}

bool ImapResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
    const QString remoteId = item.remoteId();
    const QStringList temp = remoteId.split( "-+-" );
    const QString mailBox = mailBoxForRemoteId( temp[0] );
    const qint64 uid = temp[1].toLongLong();

    KIMAP::SelectJob *select = new KIMAP::SelectJob( m_account->session() );
    select->setMailBox( mailBox );
    select->start();
    KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_account->session() );
    fetch->setProperty( "akonadiItem", QVariant::fromValue( item ) );
    KIMAP::FetchJob::FetchScope scope;
    fetch->setUidBased( true );
    fetch->setSequenceSet( KIMAP::ImapSet( uid ) );
    scope.parts.clear();// = parts.toList();
    scope.mode = KIMAP::FetchJob::FetchScope::Content;
    fetch->setScope( scope );
    connect( fetch, SIGNAL( messagesReceived( QString, QMap<qint64, qint64>, QMap<qint64, KIMAP::MessagePtr> ) ),
             this, SLOT( onMessagesReceived( QString, QMap<qint64, qint64>, QMap<qint64, KIMAP::MessagePtr> ) ) );
    connect( fetch, SIGNAL( partsReceived( QString, QMap<qint64, qint64>, QMap<qint64, KIMAP::MessageParts> ) ),
             this, SLOT( onPartsReceived( QString, QMap<qint64, qint64>, QMap<qint64, KIMAP::MessageParts> ) ) );
    connect( fetch, SIGNAL( result( KJob* ) ),
             this, SLOT( onContentFetchDone( KJob* ) ) );
    fetch->start();
    return true;
}

void ImapResource::onMessagesReceived( const QString &mailBox, const QMap<qint64, qint64> &uids,
                                       const QMap<qint64, KIMAP::MessagePtr> &messages )
{
  KIMAP::FetchJob *fetch = qobject_cast<KIMAP::FetchJob*>( sender() );
  Q_ASSERT( fetch!=0 );
  Q_ASSERT( uids.size()==1 );
  Q_ASSERT( messages.size()==1 );

  Item i = fetch->property( "akonadiItem" ).value<Item>();

  kDebug() << "MESSAGE from Imap server" << i.remoteId();
  Q_ASSERT( i.isValid() );

  KIMAP::MessagePtr message = messages[messages.keys().first()];

  i.setMimeType( "message/rfc822" );
  i.setPayload( MessagePtr( message ) );

  kDebug() << "Has Payload: " << i.hasPayload();
  kDebug() << message->head().isEmpty() << message->body().isEmpty() << message->contents().isEmpty() << message->hasContent() << message->hasHeader("Message-ID");

  itemRetrieved( i );
}

void ImapResource::onContentFetchDone( KJob *job )
{
  if ( job->error() ) {
    cancelTask( job->errorString() );
  } else {
    KIMAP::FetchJob *fetch = qobject_cast<KIMAP::FetchJob*>( job );
    if ( fetch->messages().isEmpty() && fetch->parts().isEmpty() ) {
      cancelTask( i18n("No message retrieved, server reply was empty.") );
    }
  }
}

void ImapResource::configure( WId windowId )
{
  SetupServer dlg( windowId );
  KWindowSystem::setMainWindow( &dlg, windowId );

  dlg.exec();
  if ( dlg.shouldClearCache() ) {
    clearCache();
  }

  if ( !Settings::self()->imapServer().isEmpty() && !Settings::self()->userName().isEmpty() ) {
    setName( Settings::self()->imapServer() + '/' + Settings::self()->userName() );
  } else {
    setName( KGlobal::mainComponent().aboutData()->appName() );
  }

  startConnect();
}

void ImapResource::startConnect( bool forceManualAuth )
{
  if ( Settings::self()->imapServer().isEmpty() ) {
    return;
  }

  QString password = Settings::self()->password();

  if ( password.isEmpty() || forceManualAuth ) {
    if ( !manualAuth( Settings::self()->userName(), password ) ) {
      return;
    }
  }

  delete m_account;
  m_account = new ImapAccount( Settings::self(), this );

  connect( m_account, SIGNAL( success() ),
           this, SLOT( onConnectSuccess() ) );
  connect( m_account, SIGNAL( error( int, const QString& ) ),
           this, SLOT( onConnectError( int, const QString& ) ) );

  m_account->connect( password );
}

void ImapResource::itemAdded( const Item &item, const Collection &collection )
{
  const QString mailBox = mailBoxForRemoteId( collection.remoteId() );

  // save message to the server.
  MessagePtr msg = item.payload<MessagePtr>();

  KIMAP::AppendJob *job = new KIMAP::AppendJob( m_account->session() );
  job->setProperty( "akonadiCollection", QVariant::fromValue( collection ) );
  job->setProperty( "akonadiItem", QVariant::fromValue( item ) );
  job->setMailBox( mailBox );
  job->setContent( msg->encodedContent( true ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onAppendMessageDone( KJob* ) ) );
  job->start();
}

void ImapResource::onAppendMessageDone( KJob *job )
{
  KIMAP::AppendJob *append = qobject_cast<KIMAP::AppendJob*>( job );

  const QString collectionRemoteId = remoteIdForMailBox( append->mailBox() );
  Item item = job->property( "akonadiItem" ).value<Item>();

  if ( append->error() ) {
    deferTask();
  }

  qint64 uid = append->uid();
  Q_ASSERT( uid > 0 );

  const QString remoteId =  collectionRemoteId + "-+-" + QString::number( uid );
  kDebug() << "Setting remote ID to " << remoteId;
  item.setRemoteId( remoteId );

  changeCommitted( item );

  // Check if it we got here because an itemChanged() call
  // (since in IMAP you're forced to append+remove in this case)
  qint64 oldUid = job->property( "oldUid" ).toLongLong();
  if ( oldUid ) {
    // OK it's indeed a content change, so we've to mark the old version as deleted
    KIMAP::StoreJob *store = new KIMAP::StoreJob( m_account->session() );
    store->setUidBased( true );
    store->setSequenceSet( KIMAP::ImapSet( oldUid ) );
    store->setFlags( QList<QByteArray>() << "\\Deleted" );
    store->setMode( KIMAP::StoreJob::AppendFlags );
    store->start();
  }

  Collection collection = job->property( "akonadiCollection" ).value<Collection>();
  if ( !collection.isValid() ) {
    collection = collectionFromRemoteId( collectionRemoteId );
  }

  // Get the current uid next value and store it
  UidNextAttribute *uidAttr = 0;
  int oldNextUid = 0;
  if ( collection.hasAttribute( "uidnext" ) ) {
    uidAttr = static_cast<UidNextAttribute*>( collection.attribute( "uidnext" ) );
    oldNextUid = uidAttr->uidNext();
  }

  // If the uid we just got back is the expected next one of the box
  // then update the property to the probable next uid to keep the cache in sync.
  // If not something happened in our back, so we don't update and a refetch will
  // happen at some point.
  if ( uid==oldNextUid ) {
    if ( uidAttr==0 ) {
      uidAttr = new UidNextAttribute( uid+1 );
      collection.addAttribute( uidAttr );
    } else {
      uidAttr->setUidNext( uid+1 );
    }

    CollectionModifyJob *modify = new CollectionModifyJob( collection );
    modify->exec();
  }
}

void ImapResource::itemChanged( const Item &item, const QSet<QByteArray> &parts )
{
  kDebug() << item.remoteId() << parts;

  const QString remoteId = item.remoteId();
  const QStringList temp = remoteId.split( "-+-" );
  const QString mailBox = mailBoxForRemoteId( temp[0] );
  const qint64 uid = temp[1].toLongLong();

  if ( parts.contains( "PLD:RFC822" ) ) {
    // save message to the server.
    MessagePtr msg = item.payload<MessagePtr>();

    KIMAP::AppendJob *job = new KIMAP::AppendJob( m_account->session() );
    job->setProperty( "akonadiItem", QVariant::fromValue( item ) );
    job->setProperty( "oldUid", uid ); // Will be used in onAppendMessageDone
    job->setMailBox( mailBox );
    job->setContent( msg->encodedContent( true ) );
    job->setFlags( item.flags().toList() );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( onAppendMessageDone( KJob* ) ) );
    job->start();

  } else if ( parts.contains( "FLAGS" ) ) {
    KIMAP::SelectJob *select = new KIMAP::SelectJob( m_account->session() );
    select->setMailBox( mailBox );
    select->start();
    KIMAP::StoreJob *store = new KIMAP::StoreJob( m_account->session() );
    store->setProperty( "akonadiItem", QVariant::fromValue( item ) );
    store->setProperty( "itemUid", uid );
    store->setUidBased( true );
    store->setSequenceSet( KIMAP::ImapSet( uid ) );
    store->setFlags( item.flags().toList() );
    store->setMode( KIMAP::StoreJob::SetFlags );
    connect( store, SIGNAL( result( KJob* ) ), SLOT( onStoreFlagsDone( KJob* ) ) );
    store->start();
  }
}

void ImapResource::onStoreFlagsDone( KJob *job )
{
  KIMAP::StoreJob *store = qobject_cast<KIMAP::StoreJob*>( job );

  if ( store->error() ) {
    deferTask();
  }

  Item item = job->property( "akonadiItem" ).value<Item>();
  qint64 uid = job->property( "itemUid" ).toLongLong();
  bool itemRemoval = job->property( "itemRemoval" ).toBool();

  if ( !itemRemoval ) {
    item.setFlags( store->resultingFlags()[uid].toSet() );
    changeCommitted( item );
  } else {
    changeProcessed();
  }
}

void ImapResource::itemRemoved( const Akonadi::Item &item )
{
  // The imap specs do not allow for a single message to be deleted. We can only
  // set the \Deleted flag. The message will actually be deleted when EXPUNGE will
  // be issued on the next retrieveItems().

  const QString remoteId = item.remoteId();
  const QStringList temp = remoteId.split( "-+-" );
  const QString mailBox = mailBoxForRemoteId( temp[0] );
  const qint64 uid = temp[1].toLongLong();

  KIMAP::SelectJob *select = new KIMAP::SelectJob( m_account->session() );
  select->setMailBox( mailBox );
  select->start();
  KIMAP::StoreJob *store = new KIMAP::StoreJob( m_account->session() );
  store->setProperty( "akonadiItem", QVariant::fromValue( item ) );
  store->setProperty( "itemRemoval", true );
  store->setUidBased( true );
  store->setSequenceSet( KIMAP::ImapSet( uid ) );
  store->setFlags( QList<QByteArray>() << "\\Deleted" );
  store->setMode( KIMAP::StoreJob::AppendFlags );
  connect( store, SIGNAL( result( KJob* ) ), SLOT( onStoreFlagsDone( KJob* ) ) );
  store->start();
}

void ImapResource::retrieveCollections()
{
  if ( !m_account || !m_account->session() ) {
    kDebug() << "Ignoring this request. Probably there is no connection.";
    cancelTask();
    return;
  }

  Collection root;
  root.setName( m_account->server() + '/' + m_account->userName() );
  root.setRemoteId( rootRemoteId() );
  root.setContentMimeTypes( QStringList( Collection::mimeType() ) );

  CachePolicy policy;
  policy.setInheritFromParent( false );
  policy.setSyncOnDemand( true );
  root.setCachePolicy( policy );

  setCollectionStreamingEnabled( true );
  collectionsRetrievedIncremental( Collection::List() << root, Collection::List() );

  KIMAP::ListJob *listJob = new KIMAP::ListJob( m_account->session() );
  listJob->setIncludeUnsubscribed( !m_account->isSubscriptionEnabled() );
  connect( listJob, SIGNAL( mailBoxesReceived(QList<KIMAP::MailBoxDescriptor>, QList< QList<QByteArray> >) ),
           this, SLOT( onMailBoxesReceived(QList<KIMAP::MailBoxDescriptor>, QList< QList<QByteArray> >) ) );
  connect( listJob, SIGNAL(result(KJob*)), SLOT(onMailBoxesReceiveDone(KJob*)) );
  listJob->start();
}

void ImapResource::onMailBoxesReceived( const QList< KIMAP::MailBoxDescriptor > &descriptors,
                                        const QList< QList<QByteArray> > &flags )
{
  QStringList reportedPaths = sender()->property("reportedPaths").toStringList();

  Collection::List collections;
  QStringList contentTypes;
  contentTypes << "message/rfc822" << Collection::mimeType();

  for ( int i=0; i<descriptors.size(); ++i ) {
    KIMAP::MailBoxDescriptor descriptor = descriptors[i];

    QStringList pathParts = descriptor.name.split(descriptor.separator);
    QString separator = descriptor.separator;

    QString parentPath;
    QString currentPath;

    foreach ( const QString &pathPart, pathParts ) {
      currentPath+='/'+pathPart;
      if ( currentPath.startsWith( '/' ) ) {
        currentPath.remove( 0, 1 );
      }

      if ( reportedPaths.contains( currentPath ) ) {
        parentPath = currentPath;
        continue;
      } else {
        reportedPaths << currentPath;
      }

      Collection c;
      c.setName( pathPart );
      c.setRemoteId( remoteIdForMailBox( currentPath ) );
      c.setParentRemoteId( remoteIdForMailBox( parentPath ) );
      c.setRights( Collection::AllRights );
      c.setContentMimeTypes( contentTypes );

      CachePolicy cachePolicy;
      cachePolicy.setInheritFromParent( false );
      cachePolicy.setIntervalCheckTime( -1 );
      cachePolicy.setSyncOnDemand( true );

      // If the folder is the Inbox, make some special settings.
      if ( currentPath.compare( QLatin1String("INBOX") , Qt::CaseInsensitive ) == 0 ) {
        cachePolicy.setIntervalCheckTime( 1 );
        c.setName( "Inbox" );
      }

      // If this folder is a noselect folder, make some special settings.
      if ( flags[i].contains( "\\NoSelect" ) ) {
        cachePolicy.setSyncOnDemand( false );
        c.addAttribute( new NoSelectAttribute( true ) );
      }

      c.setCachePolicy( cachePolicy );

      collections << c;
      parentPath = currentPath;
    }
  }

  sender()->setProperty("reportedPaths", reportedPaths);
  collectionsRetrievedIncremental( collections, Collection::List() );
}

void ImapResource::onMailBoxesReceiveDone(KJob* job)
{
  // TODO error handling
  collectionsRetrievalDone();
}

// ----------------------------------------------------------------------------------

void ImapResource::retrieveItems( const Collection &col )
{
  kDebug( ) << col.remoteId();

  // Prevent fetching items from noselect folders.
  if ( col.hasAttribute( "noselect" ) ) {
    NoSelectAttribute* noselect = static_cast<NoSelectAttribute*>( col.attribute( "noselect" ) );
    if ( noselect->noSelect() ) {
      kDebug() << "No Select folder";
      itemsRetrievalDone();
      return;
    }
  }

  const QString mailBox = mailBoxForRemoteId( col.remoteId() );
  const QStringList capabilities = m_account->capabilities();

  // First get the annotations from the mailbox if it's supported
  if ( capabilities.contains( "METADATA" ) || capabilities.contains( "ANNOTATEMORE" ) ) {
    KIMAP::GetMetaDataJob *meta = new KIMAP::GetMetaDataJob( m_account->session() );
    meta->setProperty( "akonadiCollection", QVariant::fromValue( col ) );
    meta->setMailBox( mailBox );
    if ( capabilities.contains( "METADATA" ) ) {
      meta->setServerCapability( KIMAP::MetaDataJobBase::Metadata );
      meta->addEntry( "*" );
    } else {
      meta->setServerCapability( KIMAP::MetaDataJobBase::Annotatemore );
      meta->addEntry( "*", "value.shared" );
    }
    connect( meta, SIGNAL( result( KJob* ) ), SLOT( onGetMetaDataDone( KJob* ) ) );
    meta->start();
  }

  // Get the ACLs from the mailbox if it's supported
  if ( capabilities.contains( "ACL" ) ) {
    KIMAP::GetAclJob *acl = new KIMAP::GetAclJob( m_account->session() );
    acl->setProperty( "akonadiCollection", QVariant::fromValue( col ) );
    acl->setMailBox( mailBox );
    connect( acl, SIGNAL( result( KJob* ) ), SLOT( onGetAclDone( KJob* ) ) );
    acl->start();

    KIMAP::MyRightsJob *rights = new KIMAP::MyRightsJob( m_account->session() );
    rights->setProperty( "akonadiCollection", QVariant::fromValue( col ) );
    rights->setMailBox( mailBox );
    connect( rights, SIGNAL( result( KJob* ) ), SLOT( onRightsReceived( KJob* ) ) );
    rights->start();
  }

  // Get the QUOTA info from the mailbox if it's supported
  if ( capabilities.contains( "QUOTA" ) ) {
    KIMAP::GetQuotaRootJob *quota = new KIMAP::GetQuotaRootJob( m_account->session() );
    quota->setProperty( "akonadiCollection", QVariant::fromValue( col ) );
    quota->setMailBox( mailBox );
    connect( quota, SIGNAL( result( KJob* ) ), SLOT( onQuotasReceived( KJob* ) ) );
    quota->start();
  }

  // Now is the right time to expunge the messages marked \\Deleted from this mailbox.
  KIMAP::SelectJob *select = new KIMAP::SelectJob( m_account->session() );
  select->setMailBox( mailBox );
  select->start();
  KIMAP::ExpungeJob *expunge = new KIMAP::ExpungeJob( m_account->session() );
  expunge->start();

  // Issue another select to get the updated info from the mailbox
  select = new KIMAP::SelectJob( m_account->session() );
  select->setMailBox( mailBox );
  connect( select, SIGNAL( result( KJob* ) ),
           this, SLOT( onSelectDone( KJob* ) ) );
  select->start();
}

void ImapResource::onHeadersReceived( const QString &mailBox, const QMap<qint64, qint64> &uids,
                                      const QMap<qint64, qint64> &sizes,
                                      const QMap<qint64, KIMAP::MessageFlags> &flags,
                                      const QMap<qint64, KIMAP::MessagePtr> &messages )
{
  Item::List addedItems;

  foreach ( qint64 number, uids.keys() ) {
    Akonadi::Item i;
    i.setRemoteId( remoteIdForMailBox( mailBox ) + "-+-" + QString::number( uids[number] ) );
    i.setMimeType( "message/rfc822" );
    i.setPayload( MessagePtr( messages[number] ) );
    i.setSize( sizes[number] );

    foreach( const QByteArray &flag, flags[number] ) {
      i.setFlag( flag );
    }
    kDebug() << "Flags: " << i.flags();
    addedItems << i;
  }

  itemsRetrievedIncremental( addedItems, Item::List() );
}

void ImapResource::onHeadersFetchDone( KJob */*job*/ )
{
  itemsRetrievalDone();
}


// ----------------------------------------------------------------------------------

void ImapResource::collectionAdded( const Collection & collection, const Collection &parent )
{
  const QString remoteName = parent.remoteId() + '/' + collection.name();

  kDebug( ) << "New folder: " << remoteName;

  Collection c = collection;
  c.setRemoteId( remoteName );

  const QString mailBox = mailBoxForRemoteId( remoteName );

  KIMAP::CreateJob *job = new KIMAP::CreateJob( m_account->session() );
  job->setProperty( "akonadiCollection", QVariant::fromValue( c ) );
  job->setMailBox( mailBox );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onCreateMailBoxDone( KJob* ) ) );
  job->start();
}

void ImapResource::onCreateMailBoxDone( KJob *job )
{
  Collection collection = job->property( "akonadiCollection" ).value<Collection>();

  if ( !job->error() ) {
    changeCommitted( collection );
  } else {
    // remove the collection again.
    kDebug() << "Failed to create the folder, deleting it in akonadi again";
    emit warning( i18n( "Failed to create the folder, restoring folder list." ) );
    new CollectionDeleteJob( collection, this );
  }
}

void ImapResource::collectionChanged( const Collection & collection )
{
  QString oldRemoteId = collection.remoteId();
  QString parentRemoteId = oldRemoteId.mid( 0, oldRemoteId.lastIndexOf('/') );

  QString newRemoteId = parentRemoteId + '/' + collection.name();

  Collection c = collection;
  c.setRemoteId( newRemoteId );

  const QString oldMailBox = mailBoxForRemoteId( oldRemoteId );
  const QString newMailBox = mailBoxForRemoteId( newRemoteId );

  KIMAP::RenameJob *job = new KIMAP::RenameJob( m_account->session() );
  job->setProperty( "akonadiCollection", QVariant::fromValue( c ) );
  job->setSourceMailBox( oldMailBox );
  job->setDestinationMailBox( newMailBox );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onRenameMailBoxDone( KJob* ) ) );
  job->start();
}

void ImapResource::onRenameMailBoxDone( KJob *job )
{
  Collection collection = job->property( "akonadiCollection" ).value<Collection>();

  if ( !job->error() ) {
    changeCommitted( collection );
  } else {
    KIMAP::RenameJob *rename = qobject_cast<KIMAP::RenameJob*>( job );

    // rename the collection again.
    kDebug() << "Failed to rename the folder, resetting it in akonadi again";
    collection.setName( rename->sourceMailBox().split('/').last() );
    collection.setRemoteId( remoteIdForMailBox( rename->sourceMailBox() ) );
    emit warning( i18n( "Failed to rename the folder, restoring folder list." ) );
    changeCommitted( collection );
  }
}

void ImapResource::collectionRemoved( const Collection &collection )
{
  const QString mailBox = mailBoxForRemoteId( collection.remoteId() );

  KIMAP::DeleteJob *job = new KIMAP::DeleteJob( m_account->session() );
  job->setProperty( "akonadiCollection", QVariant::fromValue( collection ) );
  job->setMailBox( mailBox );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onDeleteMailBoxDone( KJob* ) ) );
  job->start();
}

void ImapResource::onDeleteMailBoxDone( KJob *job )
{
  // finish the task.
  changeProcessed();

    if ( !job->error() ) {
        kDebug() << "Failed to delete the folder, resync the folder tree";
        emit warning( i18n( "Failed to delete the folder, restoring folder list." ) );
        synchronizeCollectionTree();
    }
}

/******************* Slots  ***********************************************/

void ImapResource::onConnectError( int code, const QString &message )
{
  if ( code==ImapAccount::LoginFailError ) {
    // the credentials where not ok....
    int i = KMessageBox::questionYesNoCancelWId( winIdForDialogs(),
                                                 i18n( "The server refused the supplied username and password. "
                                                       "Do you want to go to the settings, have another attempt "
                                                       "at logging in, or do nothing?" ),
                                                 i18n( "Could Not Authenticate" ),
                                                 KGuiItem( i18n( "Settings" ) ),
                                                 KGuiItem( i18n( "Single Input" ) ) );
    if ( i == KMessageBox::Yes ) {
      configure( winIdForDialogs() );
      return;
    } else if ( i == KMessageBox::No ) {
      startConnect( true );
      return;
    } else {
      KIMAP::LogoutJob *logout = new KIMAP::LogoutJob( m_account->session() );
      logout->start();
      emit warning( i18n( "Could not connect to the IMAP-server %1.", m_account->server() ) );
    }
  }

  m_account->disconnect();
  emit error( message );
}

void ImapResource::onConnectSuccess()
{
  synchronizeCollectionTree();
}

void ImapResource::onGetAclDone( KJob *job )
{
  if ( job->error() ) {
    return; // Well, no metadata for us then...
  }

  KIMAP::GetAclJob *acl = qobject_cast<KIMAP::GetAclJob*>( job );
  Collection collection = job->property( "akonadiCollection" ).value<Collection>();

  // Store the mailbox ACLs
  if ( !collection.hasAttribute( "imapacl" ) ) {
    ImapAclAttribute *aclAttribute  = new ImapAclAttribute( acl->allRights() );
    collection.addAttribute( aclAttribute );
  } else {
    ImapAclAttribute *aclAttribute =
      static_cast<ImapAclAttribute*>( collection.attribute( "imapacl" ) );
    const QMap<QByteArray, KIMAP::Acl::Rights> oldRights = aclAttribute->rights();
    if ( oldRights != acl->allRights() ) {
      aclAttribute->setRights( acl->allRights() );
    }
  }

  CollectionModifyJob *modify = new CollectionModifyJob( collection );
  modify->exec();
}

void ImapResource::onRightsReceived( KJob *job )
{
  if ( job->error() ) {
    return; // Well, no metadata for us then...
  }

  KIMAP::MyRightsJob *rightsJob = qobject_cast<KIMAP::MyRightsJob*>( job );
  Collection collection = job->property( "akonadiCollection" ).value<Collection>();

  KIMAP::Acl::Rights imapRights = rightsJob->rights();
  Collection::Rights newRights = Collection::ReadOnly;

  if ( imapRights & KIMAP::Acl::Write ) {
    newRights|= Collection::CanChangeItem;
  }

  if ( imapRights & KIMAP::Acl::Insert ) {
    newRights|= Collection::CanCreateItem;
  }

  if ( imapRights & KIMAP::Acl::DeleteMessage ) {
    newRights|= Collection::CanDeleteItem;
  }

  if ( imapRights & KIMAP::Acl::CreateMailbox ) {
    newRights|= Collection::CanChangeCollection;
    newRights|= Collection::CanCreateCollection;
  }

  if ( imapRights & KIMAP::Acl::DeleteMailbox ) {
    newRights|= Collection::CanDeleteCollection;
  }

  if ( newRights != collection.rights() ) {
    collection.setRights( newRights );

    CollectionModifyJob *modify = new CollectionModifyJob( collection );
    modify->exec();
  }
}

void ImapResource::onQuotasReceived( KJob *job )
{
  if ( job->error() ) {
    return; // Well, no metadata for us then...
  }

  KIMAP::GetQuotaRootJob *quotaJob = qobject_cast<KIMAP::GetQuotaRootJob*>( job );
  Collection collection = job->property( "akonadiCollection" ).value<Collection>();

  QList<QByteArray> newRoots = quotaJob->roots();
  QList< QMap<QByteArray, qint64> > newLimits;
  QList< QMap<QByteArray, qint64> > newUsages;

  foreach ( const QByteArray &root, newRoots ) {
    newLimits << quotaJob->allLimits( root );
    newUsages << quotaJob->allUsages( root );
  }

  // Store the mailbox Quotas
  if ( !collection.hasAttribute( "imapquota" ) ) {
    ImapQuotaAttribute *quotaAttribute  = new ImapQuotaAttribute( newRoots, newLimits, newUsages );
    collection.addAttribute( quotaAttribute );
  } else {
    ImapQuotaAttribute *quotaAttribute =
      static_cast<ImapQuotaAttribute*>( collection.attribute( "imapquota" ) );
    const QList<QByteArray> oldRoots = quotaAttribute->roots();
    const QList< QMap<QByteArray, qint64> > oldLimits = quotaAttribute->limits();
    const QList< QMap<QByteArray, qint64> > oldUsages = quotaAttribute->usages();

    if ( oldRoots != newRoots
      || oldLimits != newLimits
      || oldUsages != newUsages ) {
      quotaAttribute->setQuotas( newRoots, newLimits, newUsages );
    }
  }

  CollectionModifyJob *modify = new CollectionModifyJob( collection );
  modify->exec();
}

void ImapResource::onGetMetaDataDone( KJob *job )
{
  if ( job->error() ) {
    return; // Well, no metadata for us then...
  }

  KIMAP::GetMetaDataJob *meta = qobject_cast<KIMAP::GetMetaDataJob*>( job );
  QMap<QByteArray, QMap<QByteArray, QByteArray> > rawAnnotations = meta->allMetaData( meta->mailBox() );

  QMap<QByteArray, QByteArray> annotations;
  QByteArray attribute = "";
  if ( meta->serverCapability()==KIMAP::MetaDataJobBase::Annotatemore ) {
    attribute = "value.shared";
  }

  foreach ( const QByteArray &entry, rawAnnotations.keys() ) {
    annotations[entry] = rawAnnotations[entry][attribute];
  }

  Collection collection = job->property( "akonadiCollection" ).value<Collection>();

  // Store the mailbox metadata
  if ( !collection.hasAttribute( "collectionannotations" ) ) {
    CollectionAnnotationsAttribute *annotationsAttribute  = new CollectionAnnotationsAttribute( annotations );
    collection.addAttribute( annotationsAttribute );
  } else {
    CollectionAnnotationsAttribute *annotationsAttribute =
      static_cast<CollectionAnnotationsAttribute*>( collection.attribute( "collectionannotations" ) );
    const QMap<QByteArray, QByteArray> oldAnnotations = annotationsAttribute->annotations();
    if ( oldAnnotations != annotations ) {
      annotationsAttribute->setAnnotations( annotations );
    }
  }

  CollectionModifyJob *modify = new CollectionModifyJob( collection );
  modify->exec();
}

void ImapResource::onSelectDone( KJob *job )
{
  if ( job->error() ) {
    itemsRetrievalDone();
    return;
  }

  KIMAP::SelectJob *select = qobject_cast<KIMAP::SelectJob*>( job );

  const QString mailBox = select->mailBox();
  const int messageCount = select->messageCount();
  const qint64 uidValidity = select->uidValidity();
  const qint64 nextUid = select->nextUid();
  const QList<QByteArray> flags = select->flags();

  // uidvalidity can change between sessions, we don't want to refetch
  // folders in that case. Keep track of what is processed and what not.
  static QStringList processed;
  bool firstTime = false;
  if ( processed.indexOf( mailBox ) == -1 ) {
    firstTime = true;
    processed.append( mailBox );
  }

  Collection collection = collectionFromRemoteId( remoteIdForMailBox( mailBox ) );
  Q_ASSERT( collection.isValid() );

  // Get the current uid validity value and store it
  int oldUidValidity = 0;
  if ( !collection.hasAttribute( "uidvalidity" ) ) {
    UidValidityAttribute* currentUidValidity  = new UidValidityAttribute( uidValidity );
    collection.addAttribute( currentUidValidity );
  } else {
    UidValidityAttribute* currentUidValidity =
      static_cast<UidValidityAttribute*>( collection.attribute( "uidvalidity" ) );
    oldUidValidity = currentUidValidity->uidValidity();
    if ( oldUidValidity != uidValidity ) {
      currentUidValidity->setUidValidity( uidValidity );
    }
  }

  // Get the current uid next value and store it
  int oldNextUid = 0;
  if ( !collection.hasAttribute( "uidnext" ) ) {
    UidNextAttribute* currentNextUid  = new UidNextAttribute( nextUid );
    collection.addAttribute( currentNextUid );
  } else {
    UidNextAttribute* currentNextUid =
      static_cast<UidNextAttribute*>( collection.attribute( "uidnext" ) );
    oldNextUid = currentNextUid->uidNext();
    if ( oldNextUid != nextUid ) {
      currentNextUid->setUidNext( nextUid );
    }
  }

  // Store the mailbox flags
  if ( !collection.hasAttribute( "collectionflags" ) ) {
    CollectionFlagsAttribute *flagsAttribute  = new CollectionFlagsAttribute( flags );
    collection.addAttribute( flagsAttribute );
  } else {
    CollectionFlagsAttribute *flagsAttribute =
      static_cast<CollectionFlagsAttribute*>( collection.attribute( "collectionflags" ) );
    const QList<QByteArray> oldFlags = flagsAttribute->flags();
    if ( oldFlags != flags ) {
      flagsAttribute->setFlags( flags );
    }
  }

  CollectionModifyJob *modify = new CollectionModifyJob( collection );
  modify->exec();

  // First check the uidvalidity, if this has changed, it means the folder
  // has been deleted and recreated. So we wipe out the messages and
  // retrieve all.
  if ( oldUidValidity != uidValidity && !firstTime
    && oldUidValidity != 0 ) {
    kDebug() << "UIDVALIDITY check failed (" << oldUidValidity << "|"
             << uidValidity <<") refetching "<< mailBox;

    setItemStreamingEnabled( true );

    KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_account->session() );
    KIMAP::FetchJob::FetchScope scope;
    fetch->setSequenceSet( KIMAP::ImapSet( 1, messageCount ) );
    scope.parts.clear();
    scope.mode = KIMAP::FetchJob::FetchScope::Headers;
    fetch->setScope( scope );
    connect( fetch, SIGNAL( headersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                             QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ),
             this, SLOT( onHeadersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                            QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ) );
    connect( fetch, SIGNAL( result( KJob* ) ),
             this, SLOT( onHeadersFetchDone( KJob* ) ) );
    fetch->start();
    return;
  }

  // See how many messages are in the folder currently
  qint64 realMessageCount = collection.statistics().count();
  if ( realMessageCount == -1 ) {
    Akonadi::CollectionStatisticsJob *job = new Akonadi::CollectionStatisticsJob( collection );
    if ( job->exec() ) {
      Akonadi::CollectionStatistics statistics = job->statistics();
      realMessageCount = statistics.count();
    }
  }

  kDebug() << "integrity: " << mailBox << " should be: " << messageCount << " current: " << realMessageCount;

  if ( messageCount > realMessageCount ) {
    // The amount on the server is bigger than that we have in the cache
    // that probably means that there is new mail. Fetch missing.
    kDebug() << "Fetch missing: " << messageCount << " But: " << realMessageCount;

    setItemStreamingEnabled( true );

    KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_account->session() );
    KIMAP::FetchJob::FetchScope scope;
    fetch->setSequenceSet( KIMAP::ImapSet( realMessageCount+1, messageCount ) );
    scope.parts.clear();
    scope.mode = KIMAP::FetchJob::FetchScope::Headers;
    fetch->setScope( scope );
    connect( fetch, SIGNAL( headersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                             QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ),
             this, SLOT( onHeadersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                            QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ) );
    connect( fetch, SIGNAL( result( KJob* ) ),
             this, SLOT( onHeadersFetchDone( KJob* ) ) );
    fetch->start();
    return;
  } else if ( messageCount != realMessageCount ) {
    // The amount on the server does not match the amount in the cache.
    // that means we need reget the catch completely.
    kDebug() << "O OH: " << messageCount << " But: " << realMessageCount;

    itemsClear( collection );
    setItemStreamingEnabled( true );

    KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_account->session() );
    KIMAP::FetchJob::FetchScope scope;
    fetch->setSequenceSet( KIMAP::ImapSet( 1, messageCount ) );
    scope.parts.clear();
    scope.mode = KIMAP::FetchJob::FetchScope::Headers;
    fetch->setScope( scope );
    connect( fetch, SIGNAL( headersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                             QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ),
             this, SLOT( onHeadersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                            QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ) );
    connect( fetch, SIGNAL( result( KJob* ) ),
             this, SLOT( onHeadersFetchDone( KJob* ) ) );
    fetch->start();
    return;
  } else if ( messageCount == realMessageCount && oldNextUid != nextUid
           && oldNextUid != 0 && !firstTime ) {
    // amount is right but uidnext is different.... something happened
    // behind our back...
    kDebug() << "UIDNEXT check failed, refetching mailbox";

    itemsClear( collection );
    setItemStreamingEnabled( true );

    KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_account->session() );
    KIMAP::FetchJob::FetchScope scope;
    fetch->setSequenceSet( KIMAP::ImapSet( 1, messageCount ) );
    scope.parts.clear();
    scope.mode = KIMAP::FetchJob::FetchScope::Headers;
    fetch->setScope( scope );
    connect( fetch, SIGNAL( headersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                             QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ),
             this, SLOT( onHeadersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                            QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ) );
    connect( fetch, SIGNAL( result( KJob* ) ),
             this, SLOT( onHeadersFetchDone( KJob* ) ) );
    fetch->start();
    return;
  }

  kDebug() << "All fine, nothing to do";
  itemsRetrievalDone();
}


/******************* Private ***********************************************/

bool ImapResource::manualAuth( const QString& username, QString &password )
{
  KPasswordDialog dlg( 0 );
  dlg.setPrompt( i18n( "Could not find a valid password, please enter it here." ) );
  if ( dlg.exec() == QDialog::Accepted ) {
    password = dlg.password();
    return true;
  } else {
    password = QString();
    return false;
  }
}

QString ImapResource::rootRemoteId() const
{
  return "imap://"+m_account->userName()+'@'+m_account->server()+'/';
}

QString ImapResource::remoteIdForMailBox( const QString &path ) const
{
  return rootRemoteId()+path;
}

QString ImapResource::mailBoxForRemoteId( const QString &remoteId ) const
{
  QString path = remoteId;
  path.replace( rootRemoteId(), "" );
  return path;
}

Collection ImapResource::collectionFromRemoteId( const QString &remoteId )
{
  CollectionFetchJob *fetch = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive );
  fetch->setResource( identifier() );
  fetch->exec();

  Collection::List collections = fetch->collections();
  foreach ( const Collection &collection, collections ) {
    if ( collection.remoteId()==remoteId ) {
      return collection;
    }
  }

  return Collection();
}

Item ImapResource::itemFromRemoteId( const Akonadi::Collection &collection, const QString &remoteId )
{
  ItemFetchJob *fetch = new ItemFetchJob( collection );
  fetch->exec();

  Item::List items = fetch->items();
  foreach ( const Item &item, items ) {
    if ( item.remoteId()==remoteId ) {
      return item;
    }
  }

  return Item();
}

void ImapResource::itemsClear( const Collection &collection )
{
  ItemFetchJob *fetch = new ItemFetchJob( collection );
  fetch->exec();

  TransactionSequence *transaction = new TransactionSequence;

  Item::List items = fetch->items();
  foreach ( const Item &item, items ) {
    new ItemDeleteJob( item, transaction );
  }

  transaction->exec();
}

AKONADI_RESOURCE_MAIN( ImapResource )

#include "imapresource.moc"


