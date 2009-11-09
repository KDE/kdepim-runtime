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
#include <QHostInfo>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <KWindowSystem>
#include <KAboutData>

#include <solid/networking.h>

#include <kimap/session.h>
#include <kimap/sessionuiproxy.h>

#include <kimap/appendjob.h>
#include <kimap/capabilitiesjob.h>
#include <kimap/copyjob.h>
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
#include <kimap/rfccodecs.h>
#include <kimap/selectjob.h>
#include <kimap/setacljob.h>
#include <kimap/setmetadatajob.h>
#include <kimap/storejob.h>
#include <kimap/subscribejob.h>

#include <kmime/kmime_message.h>

#include <akonadi/attributefactory.h>
#include <akonadi/cachepolicy.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/collectionquotaattribute.h>
#include <akonadi/collectionstatisticsjob.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/monitor.h>
#include <akonadi/changerecorder.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/session.h>
#include <akonadi/transactionsequence.h>
#include <akonadi/collectionfetchscope.h>

#include <akonadi/kmime/messageparts.h>

#include "collectionannotationsattribute.h"
#include "collectionflagsattribute.h"

#include "imapaclattribute.h"
#include "imapquotaattribute.h"

#include "imapaccount.h"
#include "imapidlemanager.h"

#include "resourceadaptor.h"

using namespace Akonadi;

static const char AKONADI_COLLECTION[] = "akonadiCollection";
static const char AKONADI_ITEM[] = "akonadiItem";
static const char AKONADI_PARTS[] = "akonadiParts";
static const char REPORTED_COLLECTIONS[] = "reportedCollections";
static const char PREVIOUS_REMOTEID[] = "previousRemoteId";
static const char SOURCE_COLLECTION[] = "sourceCollection";
static const char DESTINATION_COLLECTION[] = "destinationCollection";

ImapResource::ImapResource( const QString &id )
        :ResourceBase( id ), m_account( 0 ), m_idle( 0 )
{
  Akonadi::AttributeFactory::registerAttribute<UidValidityAttribute>();
  Akonadi::AttributeFactory::registerAttribute<UidNextAttribute>();
  Akonadi::AttributeFactory::registerAttribute<NoSelectAttribute>();

  Akonadi::AttributeFactory::registerAttribute<CollectionAnnotationsAttribute>();
  Akonadi::AttributeFactory::registerAttribute<CollectionFlagsAttribute>();

  Akonadi::AttributeFactory::registerAttribute<ImapAclAttribute>();
  Akonadi::AttributeFactory::registerAttribute<ImapQuotaAttribute>();

  changeRecorder()->fetchCollection( true );
  changeRecorder()->collectionFetchScope().setAncestorRetrieval( CollectionFetchScope::All );
  changeRecorder()->itemFetchScope().fetchFullPayload( true );
  changeRecorder()->itemFetchScope().setAncestorRetrieval( ItemFetchScope::All );

  setHierarchicalRemoteIdentifiersEnabled( true );

  connect( this, SIGNAL(reloadConfiguration()), SLOT(reconnect()) );

  new ResourceAdaptor( this );
}

ImapResource::~ImapResource()
{
}

bool ImapResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
    Q_UNUSED( parts );

  if ( !isSessionAvailable() ) {
    kDebug() << "Ignoring this request. Probably there is no connection.";
    cancelTask( i18n( "There is currently no connection to the IMAP server." ) );
    return false;
  }

    const QString mailBox = mailBoxForCollection( item.parentCollection() );
    const qint64 uid = item.remoteId().toLongLong();

    KIMAP::SelectJob *select = new KIMAP::SelectJob( m_account->mainSession() );
    select->setMailBox( mailBox );
    select->start();
    KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_account->mainSession() );
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
  Q_UNUSED( mailBox );

  KIMAP::FetchJob *fetch = qobject_cast<KIMAP::FetchJob*>( sender() );
  Q_ASSERT( fetch!=0 );
  Q_ASSERT( uids.size()==1 );
  Q_ASSERT( messages.size()==1 );

  Item i = fetch->property( "akonadiItem" ).value<Item>();

  kDebug() << "MESSAGE from Imap server" << i.remoteId();
  Q_ASSERT( i.isValid() );

  KIMAP::MessagePtr message = messages[messages.keys().first()];

  i.setMimeType( "message/rfc822" );
  i.setPayload( KMime::Message::Ptr( message ) );

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

  if ( dlg.result() == QDialog::Accepted ) {
    Settings::self()->writeConfig();
    reconnect();

    emit configurationDialogAccepted();
  } else {
    emit configurationDialogRejected();
  }
}

void ImapResource::startConnect( bool forceManualAuth )
{
  if ( Settings::self()->imapServer().isEmpty() ) {
    emit status( Broken, i18n( "No server configured yet." ) );
    return;
  }

  connect( Settings::self(), SIGNAL(passwordRequestCompleted(QString, bool)),
           this, SLOT(onPasswordRequestCompleted(QString, bool)) );
  if ( forceManualAuth ) {
    Settings::self()->requestManualAuth();
  } else {
    Settings::self()->requestPassword();
  }
}

void ImapResource::onPasswordRequestCompleted( const QString &password, bool userRejected )
{
  disconnect( Settings::self(), SIGNAL(passwordRequestCompleted(QString, bool)),
              this, SLOT(onPasswordRequestCompleted(QString, bool)) );

  if ( userRejected ) {
    emit status( Broken, i18n( "Could not read the password: user rejected wallet access." ) );
    return;
  } else if ( password.isEmpty() ) {
    emit status( Broken, i18n( "Authentication failed." ) );
    return;
  } else {
    Settings::self()->setPassword( password );
  }

  if ( m_account!=0 ) {
    m_account->deleteLater();
    disconnect( m_account, 0, this, 0 );
  }

  m_account = new ImapAccount( Settings::self(), this );

  connect( m_account, SIGNAL( success( KIMAP::Session* ) ),
           this, SLOT( onConnectSuccess( KIMAP::Session* ) ) );
  connect( m_account, SIGNAL( error( KIMAP::Session*, int, const QString& ) ),
           this, SLOT( onConnectError( KIMAP::Session*, int, const QString& ) ) );

  m_account->connect( password );
}

void ImapResource::itemAdded( const Item &item, const Collection &collection )
{
  if ( !isSessionAvailable() ) {
    kDebug() << "Defering this request. Probably there is no connection.";
    deferTask();
    return;
  }

  if ( !item.hasPayload<KMime::Message::Ptr>() ) {
    changeProcessed();
    return;
  }

  const QString mailBox = mailBoxForCollection( collection );

  kDebug() << "Got notification about item added for local id " << item.id() << " and remote id " << item.remoteId();

  // save message to the server.
  KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();

  KIMAP::AppendJob *job = new KIMAP::AppendJob( m_account->mainSession() );
  job->setProperty( AKONADI_COLLECTION, QVariant::fromValue( collection ) );
  job->setProperty( "akonadiItem", QVariant::fromValue( item ) );
  job->setMailBox( mailBox );
  job->setContent( msg->encodedContent( true ) );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onAppendMessageDone( KJob* ) ) );
  job->start();
}

void ImapResource::onAppendMessageDone( KJob *job )
{
  KIMAP::AppendJob *append = qobject_cast<KIMAP::AppendJob*>( job );

  Item item = job->property( "akonadiItem" ).value<Item>();

  if ( append->error() ) {
    emit error( append->errorString() );
    deferTask();
    return;
  }

  qint64 uid = append->uid();
  Q_ASSERT( uid > 0 );

  const QString remoteId =  QString::number( uid );
  kDebug() << "Setting remote ID to " << remoteId << " for item with local id " << item.id();
  item.setRemoteId( remoteId );

  changeCommitted( item );

  // Check if it we got here because an itemChanged() call
  // (since in IMAP you're forced to append+remove in this case)
  qint64 oldUid = job->property( "oldUid" ).toLongLong();
  if ( oldUid ) {
    // OK it's indeed a content change, so we've to mark the old version as deleted
    KIMAP::StoreJob *store = new KIMAP::StoreJob( m_account->mainSession() );
    store->setUidBased( true );
    store->setSequenceSet( KIMAP::ImapSet( oldUid ) );
    store->setFlags( QList<QByteArray>() << "\\Deleted" );
    store->setMode( KIMAP::StoreJob::AppendFlags );
    store->start();
  }

  Collection collection = job->property( AKONADI_COLLECTION ).value<Collection>();

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

    new CollectionModifyJob( collection );
  }
}

void ImapResource::itemChanged( const Item &item, const QSet<QByteArray> &parts )
{
  kDebug() << item.remoteId() << parts;

  if ( !isSessionAvailable() ) {
    kDebug() << "Defering this request. Probably there is no connection.";
    deferTask();
    return;
  }

  const QString mailBox = mailBoxForCollection( item.parentCollection() );
  const qint64 uid = item.remoteId().toLongLong();

  if ( parts.contains( "PLD:RFC822" ) ) {
    if ( !item.hasPayload<KMime::Message::Ptr>() ) {
      changeProcessed();
      return;
    }
    // save message to the server.
    KMime::Message::Ptr msg = item.payload<KMime::Message::Ptr>();

    KIMAP::AppendJob *job = new KIMAP::AppendJob( m_account->mainSession() );
    job->setProperty( "akonadiItem", QVariant::fromValue( item ) );
    job->setProperty( "oldUid", uid ); // Will be used in onAppendMessageDone
    job->setMailBox( mailBox );
    job->setContent( msg->encodedContent( true ) );
    job->setFlags( item.flags().toList() );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( onAppendMessageDone( KJob* ) ) );
    job->start();

  } else if ( parts.contains( "FLAGS" ) ) {
    KIMAP::SelectJob *select = new KIMAP::SelectJob( m_account->mainSession() );
    select->setMailBox( mailBox );
    select->start();
    KIMAP::StoreJob *store = new KIMAP::StoreJob( m_account->mainSession() );
    store->setProperty( "akonadiItem", QVariant::fromValue( item ) );
    store->setProperty( "itemUid", uid );
    store->setUidBased( true );
    store->setSequenceSet( KIMAP::ImapSet( uid ) );
    store->setFlags( item.flags().toList() );
    store->setMode( KIMAP::StoreJob::SetFlags );
    connect( store, SIGNAL( result( KJob* ) ), SLOT( onStoreFlagsDone( KJob* ) ) );
    store->start();
  } else {
    changeProcessed();
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
  if ( !isSessionAvailable() ) {
    kDebug() << "Defering this request. Probably there is no connection.";
    deferTask();
    return;
  }

  // The imap specs do not allow for a single message to be deleted. We can only
  // set the \Deleted flag. The message will actually be deleted when EXPUNGE will
  // be issued on the next retrieveItems().

  const QString mailBox = mailBoxForCollection( item.parentCollection() );
  const qint64 uid = item.remoteId().toLongLong();

  KIMAP::SelectJob *select = new KIMAP::SelectJob( m_account->mainSession() );
  select->setMailBox( mailBox );
  select->start();
  KIMAP::StoreJob *store = new KIMAP::StoreJob( m_account->mainSession() );
  store->setProperty( "akonadiItem", QVariant::fromValue( item ) );
  store->setProperty( "itemRemoval", true );
  store->setUidBased( true );
  store->setSequenceSet( KIMAP::ImapSet( uid ) );
  store->setFlags( QList<QByteArray>() << "\\Deleted" );
  store->setMode( KIMAP::StoreJob::AppendFlags );
  connect( store, SIGNAL( result( KJob* ) ), SLOT( onStoreFlagsDone( KJob* ) ) );
  store->start();
}

void ImapResource::itemMoved( const Akonadi::Item &item, const Akonadi::Collection &source,
                              const Akonadi::Collection &destination )
{
  if ( !isSessionAvailable() ) {
    kDebug() << "Defering this request. Probably there is no connection.";
    deferTask();
    return;
  }

  if ( item.remoteId().isEmpty() ) {
    emit error( i18n( "Cannot move message, it does not exist on the server." ) );
    changeProcessed();
    return;
  }

  if ( source.remoteId().isEmpty() ) {
    emit error( i18n( "Cannot move message out of '%1', '%1' does not exist on the server.",
                      source.name() ) );
    changeProcessed();
    return;
  }

  if ( destination.remoteId().isEmpty() ) {
    emit error( i18n( "Cannot move message to '%1', '%1' does not exist on the server.",
                      source.name() ) );
    changeProcessed();
    return;
  }

  const QString oldMailBox = mailBoxForCollection( source );
  const QString newMailBox = mailBoxForCollection( destination );

  if ( oldMailBox != newMailBox ) {
    KIMAP::SelectJob *select = new KIMAP::SelectJob( m_account->mainSession() );
    select->setMailBox( oldMailBox );
    select->setProperty( AKONADI_ITEM, QVariant::fromValue( item ) );
    select->setProperty( SOURCE_COLLECTION, QVariant::fromValue( source ) );
    select->setProperty( DESTINATION_COLLECTION, QVariant::fromValue( destination ) );
    connect( select, SIGNAL( result( KJob* ) ), SLOT( onPreItemMoveSelectDone( KJob* ) ) );
    select->start();
  } else {
    changeProcessed();
  }
}

void ImapResource::onPreItemMoveSelectDone( KJob *job )
{
  if ( !job->error() ) {
    Item item = job->property( AKONADI_ITEM ).value<Item>();
    const qint64 uid = item.remoteId().toLongLong();

    Collection destination = job->property( DESTINATION_COLLECTION ).value<Collection>();
    const QString newMailBox = mailBoxForCollection( destination );

    KIMAP::CopyJob *copy = new KIMAP::CopyJob( m_account->mainSession() );
    copy->setProperty( AKONADI_ITEM, job->property( AKONADI_ITEM ) );
    copy->setProperty( SOURCE_COLLECTION, job->property( SOURCE_COLLECTION ) );
    copy->setProperty( DESTINATION_COLLECTION, job->property( DESTINATION_COLLECTION ) );
    copy->setUidBased( true );
    copy->setSequenceSet( KIMAP::ImapSet( uid ) );
    copy->setMailBox( newMailBox );
    connect( copy, SIGNAL( result( KJob* ) ), SLOT( onCopyMessageDone( KJob* ) ) );
    copy->start();

  } else {
    const Collection source = job->property( SOURCE_COLLECTION ).value<Collection>();
    Q_ASSERT( source.isValid() );
    emit error( i18n( "Failed to move message out of '%1' on the IMAP server. Could not select '%1'.",
                      source.name() ) );
    changeProcessed();
  }
}

void ImapResource::onCopyMessageDone( KJob *job )
{
  if ( !job->error() ) {
    Item item = job->property( AKONADI_ITEM ).value<Item>();
    Collection destination = job->property( DESTINATION_COLLECTION ).value<Collection>();
    const qint64 oldUid = item.remoteId().toLongLong();

    // Go ahead, UIDPLUS is supposed to be supported and we copied a single message
    KIMAP::CopyJob *copy = static_cast<KIMAP::CopyJob*>( job );
    const qint64 newUid = copy->resultingUids().intervals().first().begin();

    // Update the item content with the new UID from the copy
    item.setRemoteId( QString::number( newUid ) );
    item.setParentCollection( destination );

    // Mark the old one ready for deletion
    KIMAP::StoreJob *store = new KIMAP::StoreJob( m_account->mainSession() );
    store->setProperty( AKONADI_ITEM, QVariant::fromValue( item ) );
    store->setProperty( SOURCE_COLLECTION, job->property( SOURCE_COLLECTION ) );
    store->setProperty( DESTINATION_COLLECTION, job->property( DESTINATION_COLLECTION ) );
    store->setUidBased( true );
    store->setSequenceSet( KIMAP::ImapSet( oldUid ) );
    store->setFlags( QList<QByteArray>() << "\\Deleted" );
    store->setMode( KIMAP::StoreJob::AppendFlags );
    connect( store, SIGNAL( result( KJob* ) ), SLOT( onPostItemMoveStoreFlagsDone( KJob* ) ) );
    store->start();

  } else {
    const Collection destination = job->property( DESTINATION_COLLECTION ).value<Collection>();
    Q_ASSERT( destination.isValid() );
    emit error( i18n( "Failed to move message to '%1' on the IMAP server. Could not copy into '%1'.",
                      destination.name() ) );
    changeProcessed();
  }
}

void ImapResource::onPostItemMoveStoreFlagsDone( KJob *job )
{
  Item item = job->property( AKONADI_ITEM ).value<Item>();

  if ( job->error() ) {
    const Collection source = job->property( SOURCE_COLLECTION ).value<Collection>();
    Q_ASSERT( source.isValid() );
    emit warning( i18n( "Failed to mark the message from '%1' for deletion on the IMAP server. "
                        "It will reappear on next sync.",
                        source.name() ) );
  }

  changeCommitted( item );
}

typedef QHash<QString, Collection> StringCollectionMap;
Q_DECLARE_METATYPE( StringCollectionMap )

void ImapResource::retrieveCollections()
{
  if ( !isSessionAvailable() ) {
    kDebug() << "Ignoring this request. Probably there is no connection.";
    cancelTask( i18n( "There is currently no connection to the IMAP server." ) );
    reconnect();
    return;
  }

  Collection root;
  root.setName( m_account->server() + '/' + m_account->userName() );
  root.setRemoteId( rootRemoteId() );
  root.setContentMimeTypes( QStringList( Collection::mimeType() ) );
  root.setRights( Collection::ReadOnly );
  root.setParentCollection( Collection::root() );
  root.addAttribute( new NoSelectAttribute( true ) );

  CachePolicy policy;
  policy.setInheritFromParent( false );
  policy.setSyncOnDemand( true );

  QStringList localParts;
  localParts << Akonadi::MessagePart::Envelope
             << Akonadi::MessagePart::Header;
  int cacheTimeout = 60;

  if ( Settings::self()->disconnectedModeEnabled() ) {
    // For disconnected mode we also cache the body
    // and we keep all data indifinitely
    localParts << Akonadi::MessagePart::Body;
    cacheTimeout = -1;
  }

  policy.setLocalParts( localParts );
  policy.setCacheTimeout( cacheTimeout );

  policy.setIntervalCheckTime( Settings::self()->intervalCheckTime() );

  root.setCachePolicy( policy );

  setCollectionStreamingEnabled( true );
  collectionsRetrieved( Collection::List() << root );

  QHash<QString, Collection> reportedCollections;
  reportedCollections.insert( QString(), root );

  KIMAP::ListJob *listJob = new KIMAP::ListJob( m_account->mainSession() );
  listJob->setIncludeUnsubscribed( !m_account->isSubscriptionEnabled() );
  listJob->setQueriedNamespaces( m_account->namespaces() );
  connect( listJob, SIGNAL( mailBoxesReceived(QList<KIMAP::MailBoxDescriptor>, QList< QList<QByteArray> >) ),
           this, SLOT( onMailBoxesReceived(QList<KIMAP::MailBoxDescriptor>, QList< QList<QByteArray> >) ) );
  connect( listJob, SIGNAL(result(KJob*)), SLOT(onMailBoxesReceiveDone(KJob*)) );
  listJob->setProperty( REPORTED_COLLECTIONS, QVariant::fromValue<StringCollectionMap>( reportedCollections ) );
  listJob->start();
}

void ImapResource::onMailBoxesReceived( const QList< KIMAP::MailBoxDescriptor > &descriptors,
                                        const QList< QList<QByteArray> > &flags )
{
  QHash<QString, Collection> reportedCollections = sender()->property( REPORTED_COLLECTIONS ).value< QHash<QString, Collection> >();

  Collection::List collections;
  QStringList contentTypes;
  contentTypes << "message/rfc822" << Collection::mimeType();

  for ( int i=0; i<descriptors.size(); ++i ) {
    KIMAP::MailBoxDescriptor descriptor = descriptors[i];

    const QStringList pathParts = descriptor.name.split(descriptor.separator);
    const QString separator = descriptor.separator;
    Q_ASSERT( separator.size() == 1 ); // that's what the spec says

    QString parentPath;
    QString currentPath;

    for ( int j = 0; j < pathParts.size(); ++j ) {
      const bool isDummy = j != pathParts.size() - 1;
      const QString pathPart = pathParts.at( j );
      currentPath += separator + pathPart;

      if ( reportedCollections.contains( currentPath ) ) {
        if ( !isDummy )
          kWarning() << "Something is wrong here, we already have created a collection for" << currentPath;
        parentPath = currentPath;
        continue;
      }

      const QList<QByteArray> currentFlags  = isDummy ? (QList<QByteArray>() << "\\NoSelect") : flags[i];

      Collection c;
      c.setName( pathPart );
      c.setRemoteId( separator + pathPart );
      const Collection parentCollection = reportedCollections.value( parentPath );
      c.setParentCollection( parentCollection );
      c.setContentMimeTypes( contentTypes );

      // If the folder is the Inbox, make some special settings.
      if ( currentPath.compare( separator + QLatin1String("INBOX") , Qt::CaseInsensitive ) == 0 ) {
        EntityDisplayAttribute *attr = c.attribute<EntityDisplayAttribute>( Collection::AddIfMissing );
        attr->setDisplayName( i18n( "Inbox" ) );
        attr->setIconName( "mail-folder-inbox" );
        QStringList ridPath;
        Collection *curCol = &c;
        while ( (*curCol) != Collection::root() && !curCol->remoteId().isEmpty() ) {
          ridPath.append( curCol->remoteId() );
          curCol = &curCol->parentCollection();
        }
        Settings::self()->setIdleRidPath( ridPath );
        Settings::self()->writeConfig();
        if ( !m_idle )
          startIdle();
      }

      // If the folder is the user top-level folder, mark it as well, even although it is not officially noted in the RFC
      if ( currentPath == (separator + QLatin1String( "user" )) && currentFlags.contains( "\\NoSelect" ) ) {
        EntityDisplayAttribute *attr = c.attribute<EntityDisplayAttribute>( Collection::AddIfMissing );
        attr->setDisplayName( i18n( "Shared Folders" ) );
        attr->setIconName( "x-mail-distribution-list" );
      }

      // If this folder is a noselect folder, make some special settings.
      if ( currentFlags.contains( "\\NoSelect" ) ) {
        c.addAttribute( new NoSelectAttribute( true ) );
        c.setContentMimeTypes( QStringList() << Collection::mimeType() );
        c.setRights( Collection::ReadOnly );
      }

      collections << c;

      reportedCollections.insert( currentPath, c );
      parentPath = currentPath;
    }
  }

  sender()->setProperty( REPORTED_COLLECTIONS, QVariant::fromValue<StringCollectionMap>( reportedCollections ) );
  collectionsRetrieved( collections );

  if ( Settings::self()->retrieveMetadataOnFolderListing() ) {
    foreach ( const Collection &c, collections ) {
      triggerCollectionExtraInfoJobs( c );
    }
  }
}

void ImapResource::onMailBoxesReceiveDone(KJob* job)
{
  // TODO error handling
  Q_UNUSED( job );
  collectionsRetrievalDone();
}

// ----------------------------------------------------------------------------------

void ImapResource::triggerCollectionExtraInfoJobs( const Collection &collection )
{
  const QString mailBox = mailBoxForCollection( collection );
  const QStringList capabilities = m_account->capabilities();

  // First get the annotations from the mailbox if it's supported
  if ( capabilities.contains( "METADATA" ) || capabilities.contains( "ANNOTATEMORE" ) ) {
    KIMAP::GetMetaDataJob *meta = new KIMAP::GetMetaDataJob( m_account->mainSession() );
    meta->setProperty( AKONADI_COLLECTION, QVariant::fromValue( collection ) );
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
    KIMAP::GetAclJob *acl = new KIMAP::GetAclJob( m_account->mainSession() );
    acl->setProperty( AKONADI_COLLECTION, QVariant::fromValue( collection ) );
    acl->setMailBox( mailBox );
    connect( acl, SIGNAL( result( KJob* ) ), SLOT( onGetAclDone( KJob* ) ) );
    acl->start();

    KIMAP::MyRightsJob *rights = new KIMAP::MyRightsJob( m_account->mainSession() );
    rights->setProperty( AKONADI_COLLECTION, QVariant::fromValue( collection ) );
    rights->setMailBox( mailBox );
    connect( rights, SIGNAL( result( KJob* ) ), SLOT( onRightsReceived( KJob* ) ) );
    rights->start();
  }

  // Get the QUOTA info from the mailbox if it's supported
  if ( capabilities.contains( "QUOTA" ) ) {
    KIMAP::GetQuotaRootJob *quota = new KIMAP::GetQuotaRootJob( m_account->mainSession() );
    quota->setProperty( AKONADI_COLLECTION, QVariant::fromValue( collection ) );
    quota->setMailBox( mailBox );
    connect( quota, SIGNAL( result( KJob* ) ), SLOT( onQuotasReceived( KJob* ) ) );
    quota->start();
  }
}

void ImapResource::retrieveItems( const Collection &col )
{
  if ( !isSessionAvailable() ) {
    kDebug() << "Ignoring this request. Probably there is no connection.";
    cancelTask( i18n( "There is currently no connection to the IMAP server." ) );
    return;
  }

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

  triggerCollectionExtraInfoJobs( col );

  const QString mailBox = mailBoxForCollection( col );

  // Now is the right time to expunge the messages marked \\Deleted from this mailbox.
  if ( Settings::self()->automaticExpungeEnabled() ) {
    triggerExpunge( mailBox );
  }

  // Issue another select to get the updated info from the mailbox
  KIMAP::SelectJob *select = new KIMAP::SelectJob( m_account->mainSession() );
  select->setProperty( AKONADI_COLLECTION, QVariant::fromValue( col ) );
  select->setMailBox( mailBox );
  connect( select, SIGNAL( result( KJob* ) ),
           this, SLOT( onSelectDone( KJob* ) ) );
  select->start();
}

void ImapResource::triggerExpunge( const QString &mailBox )
{
  kDebug() << mailBox;

  KIMAP::SelectJob *select = new KIMAP::SelectJob( m_account->mainSession() );
  select->setMailBox( mailBox );
  select->start();

  KIMAP::ExpungeJob *expunge = new KIMAP::ExpungeJob( m_account->mainSession() );
  expunge->start();
}

void ImapResource::onHeadersReceived( const QString &mailBox, const QMap<qint64, qint64> &uids,
                                      const QMap<qint64, qint64> &sizes,
                                      const QMap<qint64, KIMAP::MessageFlags> &flags,
                                      const QMap<qint64, KIMAP::MessagePtr> &messages )
{
  Q_UNUSED( mailBox );

  Item::List addedItems;

  foreach ( qint64 number, uids.keys() ) {
    Akonadi::Item i;
    i.setRemoteId( QString::number( uids[number] ) );
    i.setMimeType( "message/rfc822" );
    i.setPayload( KMime::Message::Ptr( messages[number] ) );
    i.setSize( sizes[number] );

    foreach( const QByteArray &flag, flags[number] ) {
      i.setFlag( flag );
    }
    kDebug() << "Flags: " << i.flags();
    addedItems << i;
  }

  if ( sender()->property( "nonIncremental" ).toBool() ) {
    itemsRetrieved( addedItems );
  } else {
    itemsRetrievedIncremental( addedItems, Item::List() );
  }
}

void ImapResource::onHeadersFetchDone( KJob * /*job*/ )
{
  itemsRetrievalDone();
}


// ----------------------------------------------------------------------------------

void ImapResource::collectionAdded( const Collection & collection, const Collection &parent )
{
  if ( !isSessionAvailable() ) {
    kDebug() << "Defering this request. Probably there is no connection.";
    deferTask();
    return;
  }

  if ( parent.remoteId().isEmpty() ) {
    emit error( i18n("Cannot add IMAP folder '%1' for a non-existing parent folder '%2'.", collection.name(), parent.name() ) );
    changeProcessed();
    return;
  }

  QString newMailBox = mailBoxForCollection( parent );
  if ( !newMailBox.isEmpty() )
    newMailBox += parent.remoteId().at( 0 ); // separator for non-toplevel mailboxes
  newMailBox += collection.name();

  kDebug( ) << "New folder: " << newMailBox;

  Collection c = collection;
  c.setRemoteId( parent.remoteId().at( 0 ) + collection.name() );

  KIMAP::CreateJob *job = new KIMAP::CreateJob( m_account->mainSession() );
  job->setProperty( AKONADI_COLLECTION, QVariant::fromValue( c ) );
  job->setMailBox( newMailBox );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onCreateMailBoxDone( KJob* ) ) );
  job->start();
}

void ImapResource::onCreateMailBoxDone( KJob *job )
{
  const Collection collection = job->property( AKONADI_COLLECTION ).value<Collection>();

  if ( !job->error() ) {
    changeCommitted( collection );
  } else {
    emit error( i18n( "Failed to create folder '%1' on the IMAP server.", collection.name() ) );
    changeProcessed();
  }
}

void ImapResource::collectionChanged( const Collection &collection, const QSet<QByteArray> &parts )
{
  if ( !isSessionAvailable() ) {
    kDebug() << "Defering this request. Probably there is no connection.";
    deferTask();
    return;
  }

  if ( collection.remoteId().isEmpty() ) {
    emit error( i18n("Cannot modify IMAP folder '%1', it does not exist on the server.", collection.name() ) );
    changeProcessed();
    return;
  }

  QStringList encodedParts;
  foreach ( const QByteArray &part, parts ) {
    encodedParts << QString::fromUtf8( part );
  }

  kDebug() << "parts:" << encodedParts;

  triggerNextCollectionChangeJob( collection, encodedParts );
}

void ImapResource::triggerNextCollectionChangeJob( const Akonadi::Collection &collection,
                                                   const QStringList &remainingParts )
{
  if ( remainingParts.isEmpty() ) { // We processed all parts, we're done here
    changeCommitted( collection );
    return;
  }

  QStringList parts = remainingParts;
  QString currentPart = parts.takeFirst();

  if ( currentPart == "NAME" ) {
    Collection c = collection;
    c.setRemoteId( collection.remoteId().at( 0 ) + collection.name() );

    const QString oldMailBox = mailBoxForCollection( collection );
    const QString newMailBox = mailBoxForCollection( c );

    if ( oldMailBox != newMailBox ) {
      KIMAP::RenameJob *job = new KIMAP::RenameJob( m_account->mainSession() );
      job->setProperty( AKONADI_COLLECTION, QVariant::fromValue( c ) );
      job->setProperty( AKONADI_PARTS, parts );
      job->setProperty( PREVIOUS_REMOTEID, collection.remoteId() );
      job->setSourceMailBox( oldMailBox );
      job->setDestinationMailBox( newMailBox );
      connect( job, SIGNAL( result( KJob* ) ), SLOT( onRenameMailBoxDone( KJob* ) ) );
      job->start();
    } else {
      triggerNextCollectionChangeJob( collection, parts );
    }

  } else if ( currentPart == "AccessRights" ) {
    ImapAclAttribute *aclAttribute =
      collection.attribute<ImapAclAttribute>();

    if ( aclAttribute==0 ) {
      emit error( i18n( "ACLs for '%1' need to be retrieved from the IMAP server first. Skipping ACL change",
                        collection.name() ) );
      triggerNextCollectionChangeJob( collection, parts );
      return;
    }

    KIMAP::Acl::Rights imapRights = aclAttribute->rights()[m_account->userName().toUtf8()];
    Collection::Rights newRights = collection.rights();

    if ( newRights & Collection::CanChangeItem ) {
      imapRights|= KIMAP::Acl::Write;
    } else {
      imapRights&= ~KIMAP::Acl::Write;
    }

    if ( newRights & Collection::CanCreateItem ) {
      imapRights|= KIMAP::Acl::Insert;
    } else {
      imapRights&= ~KIMAP::Acl::Insert;
    }

    if ( newRights & Collection::CanDeleteItem ) {
      imapRights|= KIMAP::Acl::DeleteMessage;
    } else {
      imapRights&= ~KIMAP::Acl::DeleteMessage;
    }

    if ( newRights & ( Collection::CanChangeCollection | Collection::CanCreateCollection ) ) {
      imapRights|= KIMAP::Acl::CreateMailbox;
      imapRights|= KIMAP::Acl::Create;
    } else {
      imapRights&= ~KIMAP::Acl::CreateMailbox;
      imapRights&= ~KIMAP::Acl::Create;
    }

    if ( newRights & Collection::CanDeleteCollection ) {
      imapRights|= KIMAP::Acl::DeleteMailbox;
    } else {
      imapRights&= ~KIMAP::Acl::DeleteMailbox;
    }

    if ( ( newRights & Collection::CanDeleteItem )
      && ( newRights & Collection::CanDeleteCollection ) ) {
      imapRights|= KIMAP::Acl::Delete;
    } else {
      imapRights&= ~KIMAP::Acl::Delete;
    }

    kDebug() << "imapRights:" << imapRights
             << "newRights:" << newRights;

    KIMAP::SetAclJob *job = new KIMAP::SetAclJob( m_account->mainSession() );
    job->setProperty( AKONADI_COLLECTION, QVariant::fromValue( collection ) );
    job->setProperty( AKONADI_PARTS, parts );
    job->setMailBox( mailBoxForCollection( collection ) );
    job->setRights( KIMAP::SetAclJob::Change, imapRights );
    job->setIdentifier( m_account->userName().toUtf8() );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( onSetAclDone( KJob* ) ) );
    job->start();

  } else if ( currentPart == "collectionannotations" ) {
    CollectionAnnotationsAttribute *annotationsAttribute =
      collection.attribute<CollectionAnnotationsAttribute>();

    if ( annotationsAttribute==0 ) { // No annotations it seems... server is lieing to us?
      triggerNextCollectionChangeJob( collection, parts );
    }

    KIMAP::SetMetaDataJob *job = 0;

    QMap<QByteArray, QByteArray> annotations = annotationsAttribute->annotations();
    kDebug() << "All annotations: " << annotations;
    foreach ( const QByteArray &entry, annotations.keys() ) {
      job = new KIMAP::SetMetaDataJob( m_account->mainSession() );
      if ( m_account->capabilities().contains( "METADATA" ) ) {
        job->setServerCapability( KIMAP::MetaDataJobBase::Metadata );
      } else {
        job->setServerCapability( KIMAP::MetaDataJobBase::Annotatemore );
      }

      QByteArray attribute = entry;
      if ( job->serverCapability()==KIMAP::MetaDataJobBase::Annotatemore ) {
        attribute = "value.shared";
      }

      job->setMailBox( mailBoxForCollection( collection ) );
      job->setEntry( entry );
      job->addMetaData( attribute, annotations[entry] );
      kDebug() << "Job got entry:" << entry << " attribute:" << attribute << "value:" << annotations[entry];

      job->start();
    }

    // We'll get info out of the last job only to trigger the next phase
    // of the collection change. The other ones we fire and forget.
    // Obviously we assume here that they will all succeed or all fail.
    job->setProperty( AKONADI_COLLECTION, QVariant::fromValue( collection ) );
    job->setProperty( AKONADI_PARTS, parts );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( onSetMetaDataDone( KJob* ) ) );

  } else {
    // unknown part
    triggerNextCollectionChangeJob( collection, parts );
  }
}


void ImapResource::onRenameMailBoxDone( KJob *job )
{
  Collection collection = job->property( AKONADI_COLLECTION ).value<Collection>();
  QStringList parts = job->property( AKONADI_PARTS ).toStringList();

  if ( !job->error() ) {
    triggerNextCollectionChangeJob( collection, parts );
  } else {
    kDebug() << "Failed to rename the folder, resetting it in akonadi again";
    const QString prevRid = job->property( PREVIOUS_REMOTEID ).toString();
    Q_ASSERT( !prevRid.isEmpty() );
    collection.setName( prevRid.mid( 1 ) );
    collection.setRemoteId( prevRid );
    emit warning( i18n( "Failed to rename the folder, restoring folder list." ) );
    changeCommitted( collection );
  }
}

void ImapResource::onSetAclDone( KJob *job )
{
  Collection collection = job->property( AKONADI_COLLECTION ).value<Collection>();
  QStringList parts = job->property( AKONADI_PARTS ).toStringList();

  if ( job->error() ) {
    emit error( i18n( "Failed to write the new ACLs for '%1' on the IMAP server. %2",
                      collection.name(), job->errorText() ) );
  }

  triggerNextCollectionChangeJob( collection, parts );
}

void ImapResource::onSetMetaDataDone( KJob *job )
{
  Collection collection = job->property( AKONADI_COLLECTION ).value<Collection>();
  QStringList parts = job->property( AKONADI_PARTS ).toStringList();

  if ( job->error() ) {
    emit error( i18n( "Failed to write the new annotations for '%1' on the IMAP server. %2",
                      collection.name(), job->errorText() ) );
  }

  triggerNextCollectionChangeJob( collection, parts );
}

void ImapResource::collectionRemoved( const Collection &collection )
{
  if ( !isSessionAvailable() ) {
    kDebug() << "Defering this request. Probably there is no connection.";
    deferTask();
    return;
  }

  const QString mailBox = mailBoxForCollection( collection );

  KIMAP::DeleteJob *job = new KIMAP::DeleteJob( m_account->mainSession() );
  job->setProperty( AKONADI_COLLECTION, QVariant::fromValue( collection ) );
  job->setMailBox( mailBox );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onDeleteMailBoxDone( KJob* ) ) );
  job->start();
}

void ImapResource::onDeleteMailBoxDone( KJob *job )
{
  // finish the task.
  changeProcessed();

    if ( job->error() ) {
        kDebug() << "Failed to delete the folder, resync the folder tree";
        emit warning( i18n( "Failed to delete the folder, restoring folder list." ) );
        synchronizeCollectionTree();
    }
}


void ImapResource::collectionMoved( const Akonadi::Collection &collection, const Akonadi::Collection &source,
                                    const Akonadi::Collection &destination )
{
  if ( !isSessionAvailable() ) {
    kDebug() << "Defering this request. Probably there is no connection.";
    deferTask();
    return;
  }

  if ( collection.remoteId().isEmpty() ) {
    emit error( i18n( "Cannot move IMAP folder '%1', it does not exist on the server.",
                      collection.name() ) );
    changeProcessed();
    return;
  }

  if ( source.remoteId().isEmpty() ) {
    emit error( i18n( "Cannot move IMAP folder '%1' out of '%2', '%2' does not exist on the server.",
                      collection.name(),
                      source.name() ) );
    changeProcessed();
    return;
  }

  if ( destination.remoteId().isEmpty() ) {
    emit error( i18n( "Cannot move IMAP folder '%1' to '%2', '%2' does not exist on the server.",
                      collection.name(),
                      source.name() ) );
    changeProcessed();
    return;
  }

  // collection.remoteId() already includes the separator
  const QString oldMailBox = mailBoxForCollection( source )+collection.remoteId();
  const QString newMailBox = mailBoxForCollection( destination )+collection.remoteId();

  if ( oldMailBox != newMailBox ) {
    KIMAP::RenameJob *job = new KIMAP::RenameJob( m_account->mainSession() );
    job->setProperty( AKONADI_COLLECTION, QVariant::fromValue( collection ) );
    job->setProperty( SOURCE_COLLECTION, QVariant::fromValue( source ) );
    job->setSourceMailBox( oldMailBox );
    job->setDestinationMailBox( newMailBox );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( onMailBoxMoveDone( KJob* ) ) );
    job->start();
  } else {
    changeProcessed();
  }
}

void ImapResource::onMailBoxMoveDone( KJob *job )
{
  Collection collection = job->property( AKONADI_COLLECTION ).value<Collection>();

  if ( !job->error() ) {
    KIMAP::SubscribeJob *subscribe = new KIMAP::SubscribeJob( m_account->mainSession() );
    subscribe->setMailBox( static_cast<KIMAP::RenameJob*>( job )->destinationMailBox() );
    subscribe->setProperty( AKONADI_COLLECTION, QVariant::fromValue( collection ) );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( onSubscribeDone( KJob* ) ) );
    subscribe->start();
  } else {
    const Collection parent = job->property( SOURCE_COLLECTION ).value<Collection>();
    Q_ASSERT(  parent.isValid() );
    emit error( i18n( "Failed to move folder '%1' out of '%2' on the IMAP server.", collection.name(), parent.name() ) );
    changeProcessed();
  }
}

void ImapResource::onSubscribeDone( KJob *job )
{
  Collection collection = job->property( AKONADI_COLLECTION ).value<Collection>();

  if ( job->error() ) { // Just warn about the failed subscription
    emit warning( i18n( "Failed to subcribe to the newly moved folder '%1' on the IMAP server.",
                      collection.name() ) );
  }

  changeCommitted( collection );
}

/******************* Slots  ***********************************************/

void ImapResource::onConnectError( KIMAP::Session *session, int code, const QString &message )
{
  if ( m_account->mainSession()!=session ) {
    return;
  }

  if ( code==ImapAccount::LoginFailError ) {
    // the credentials where not ok....
    int i = KMessageBox::questionYesNoCancelWId( winIdForDialogs(),
                                                 i18n( "The server refused the supplied username and password. "
                                                       "Do you want to go to the settings, have another attempt "
                                                       "at logging in, or do nothing?" ),
                                                 i18n( "Could Not Authenticate" ),
                                                 KGuiItem( i18n( "Settings" ) ),
                                                 KGuiItem( i18nc( "Input username/password manually and not store them", "Single Input" ) ) );
    if ( i == KMessageBox::Yes ) {
      configure( winIdForDialogs() );
      return;
    } else if ( i == KMessageBox::No ) {
      startConnect( true );
      return;
    } else {
      KIMAP::LogoutJob *logout = new KIMAP::LogoutJob( m_account->mainSession() );
      logout->start();
      emit status( Broken, i18n( "Could not connect to the IMAP-server %1.", m_account->server() ) );
    }
  }

  m_account->disconnect();
  emit error( message );
}

void ImapResource::onConnectSuccess( KIMAP::Session *session )
{
  if ( m_account->mainSession()!=session ) {
    return;
  }

  startIdle();
  emit status( Idle, i18n( "Connection established." ) );
  synchronizeCollectionTree();
}

void ImapResource::onGetAclDone( KJob *job )
{
  if ( job->error() ) {
    return; // Well, no metadata for us then...
  }

  KIMAP::GetAclJob *acl = qobject_cast<KIMAP::GetAclJob*>( job );
  Collection collection = job->property( AKONADI_COLLECTION ).value<Collection>();

  // Store the mailbox ACLs
  ImapAclAttribute *aclAttribute = collection.attribute<ImapAclAttribute>( Collection::AddIfMissing );
  const QMap<QByteArray, KIMAP::Acl::Rights> oldRights = aclAttribute->rights();
  if ( oldRights != acl->allRights() ) {
    aclAttribute->setRights( acl->allRights() );
    new CollectionModifyJob( collection );
  }
}

void ImapResource::onRightsReceived( KJob *job )
{
  if ( job->error() ) {
    return; // Well, no metadata for us then...
  }

  KIMAP::MyRightsJob *rightsJob = qobject_cast<KIMAP::MyRightsJob*>( job );
  Collection collection = job->property( AKONADI_COLLECTION ).value<Collection>();

  KIMAP::Acl::Rights imapRights = rightsJob->rights();
  Collection::Rights newRights = Collection::ReadOnly;

  if ( imapRights & KIMAP::Acl::Write ) {
    newRights|= Collection::CanChangeItem;
  }

  if ( imapRights & KIMAP::Acl::Insert ) {
    newRights|= Collection::CanCreateItem;
  }

  if ( imapRights & ( KIMAP::Acl::DeleteMessage | KIMAP::Acl::Delete ) ) {
    newRights|= Collection::CanDeleteItem;
  }

  if ( imapRights & ( KIMAP::Acl::CreateMailbox | KIMAP::Acl::Create ) ) {
    newRights|= Collection::CanChangeCollection;
    newRights|= Collection::CanCreateCollection;
  }

  if ( imapRights & ( KIMAP::Acl::DeleteMailbox | KIMAP::Acl::Delete ) ) {
    newRights|= Collection::CanDeleteCollection;
  }

  kDebug() << "imapRights:" << imapRights
           << "newRights:" << newRights
           << "oldRights:" << collection.rights();

  if ( newRights != collection.rights() ) {
    collection.setRights( newRights );

    new CollectionModifyJob( collection );
  }
}

void ImapResource::onQuotasReceived( KJob *job )
{
  if ( job->error() ) {
    return; // Well, no metadata for us then...
  }

  KIMAP::GetQuotaRootJob *quotaJob = qobject_cast<KIMAP::GetQuotaRootJob*>( job );
  Collection collection = job->property( AKONADI_COLLECTION ).value<Collection>();
  const QString &mailBox = mailBoxForCollection( collection );

  QList<QByteArray> newRoots = quotaJob->roots();
  QList< QMap<QByteArray, qint64> > newLimits;
  QList< QMap<QByteArray, qint64> > newUsages;
  qint64 newCurrent = -1;
  qint64 newMax = -1;

  foreach ( const QByteArray &root, newRoots ) {
    newLimits << quotaJob->allLimits( root );
    newUsages << quotaJob->allUsages( root );

    const QString &decodedRoot = QString::fromUtf8( KIMAP::decodeImapFolderName( root ) );

    if ( newRoots.size() == 1 || decodedRoot == mailBox ) {
      newCurrent = newUsages.last()["STORAGE"] * 1024;
      newMax = newLimits.last()["STORAGE"] * 1024;
    }
  }

  bool updateNeeded = false;

  // Store the mailbox IMAP Quotas
  ImapQuotaAttribute *imapQuotaAttribute = collection.attribute<ImapQuotaAttribute>( Collection::AddIfMissing );
  const QList<QByteArray> oldRoots = imapQuotaAttribute->roots();
  const QList< QMap<QByteArray, qint64> > oldLimits = imapQuotaAttribute->limits();
  const QList< QMap<QByteArray, qint64> > oldUsages = imapQuotaAttribute->usages();

  if ( oldRoots != newRoots
    || oldLimits != newLimits
    || oldUsages != newUsages )
  {
    imapQuotaAttribute->setQuotas( newRoots, newLimits, newUsages );
    updateNeeded = true;
  }

  // Store the collection Quota
  CollectionQuotaAttribute *quotaAttribute
    = collection.attribute<CollectionQuotaAttribute>( Collection::AddIfMissing );
  qint64 oldCurrent = quotaAttribute->currentValue();
  qint64 oldMax = quotaAttribute->maximumValue();

  if ( oldCurrent != newCurrent
    || oldMax != newMax ) {
    quotaAttribute->setCurrentValue( newCurrent );
    quotaAttribute->setMaximumValue( newMax );
    updateNeeded = true;
  }

  if ( updateNeeded ) {
    new CollectionModifyJob( collection );
  }
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

  // filter out unused and annoying Cyrus annotation /vendor/cmu/cyrus-imapd/lastupdate
  // which contains the current date and time and thus constantly changes for no good
  // reason which triggers a change notification and thus a bunch of Akonadi operations
  annotations.remove( "/vendor/cmu/cyrus-imapd/lastupdate" );

  Collection collection = job->property( AKONADI_COLLECTION ).value<Collection>();

  // Store the mailbox metadata
  CollectionAnnotationsAttribute *annotationsAttribute =
    collection.attribute<CollectionAnnotationsAttribute>( Collection::AddIfMissing );
  const QMap<QByteArray, QByteArray> oldAnnotations = annotationsAttribute->annotations();
  if ( oldAnnotations != annotations ) {
    annotationsAttribute->setAnnotations( annotations );
    new CollectionModifyJob( collection );
  }
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

  Collection collection = job->property( AKONADI_COLLECTION ).value<Collection>();

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

  new CollectionModifyJob( collection );

  KIMAP::FetchJob::FetchScope scope;
  scope.parts.clear();
  scope.mode = KIMAP::FetchJob::FetchScope::Headers;

  if ( collection.cachePolicy()
       .localParts().contains( Akonadi::MessagePart::Body ) ) {
    scope.mode = KIMAP::FetchJob::FetchScope::Full;
  }

  // First check the uidvalidity, if this has changed, it means the folder
  // has been deleted and recreated. So we wipe out the messages and
  // retrieve all.
  if ( oldUidValidity != uidValidity && !firstTime
    && oldUidValidity != 0 ) {
    kDebug() << "UIDVALIDITY check failed (" << oldUidValidity << "|"
             << uidValidity <<") refetching "<< mailBox;

    setItemStreamingEnabled( true );

    KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_account->mainSession() );
    fetch->setSequenceSet( KIMAP::ImapSet( 1, messageCount ) );
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

    KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_account->mainSession() );
    fetch->setSequenceSet( KIMAP::ImapSet( realMessageCount+1, messageCount ) );
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
    // that means we need reget the cache completely.
    kDebug() << "O OH: " << messageCount << " But: " << realMessageCount;

    setItemStreamingEnabled( true );

    KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_account->mainSession() );
    fetch->setSequenceSet( KIMAP::ImapSet( 1, messageCount ) );
    fetch->setScope( scope );
    connect( fetch, SIGNAL( headersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                             QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ),
             this, SLOT( onHeadersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                            QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ) );
    connect( fetch, SIGNAL( result( KJob* ) ),
             this, SLOT( onHeadersFetchDone( KJob* ) ) );
    fetch->setProperty( "nonIncremental", true );
    fetch->start();
    return;
  } else if ( messageCount == realMessageCount && oldNextUid != nextUid
           && oldNextUid != 0 && !firstTime ) {
    // amount is right but uidnext is different.... something happened
    // behind our back...
    kDebug() << "UIDNEXT check failed, refetching mailbox";

    setItemStreamingEnabled( true );

    KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_account->mainSession() );
    fetch->setSequenceSet( KIMAP::ImapSet( 1, messageCount ) );
    fetch->setScope( scope );
    connect( fetch, SIGNAL( headersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                             QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ),
             this, SLOT( onHeadersReceived( QString, QMap<qint64, qint64>, QMap<qint64, qint64>,
                                            QMap<qint64, KIMAP::MessageFlags>, QMap<qint64, KIMAP::MessagePtr> ) ) );
    connect( fetch, SIGNAL( result( KJob* ) ),
             this, SLOT( onHeadersFetchDone( KJob* ) ) );
    fetch->setProperty( "nonIncremental", true );
    fetch->start();
    return;
  }

  kDebug() << "All fine, nothing to do";
  itemsRetrievalDone();
}


/******************* Private ***********************************************/

QString ImapResource::rootRemoteId() const
{
  return "imap://"+m_account->userName()+'@'+m_account->server()+'/';
}

QString ImapResource::mailBoxForCollection( const Collection& col ) const
{
  if ( col.remoteId().isEmpty() ) {
    kWarning() << "Got incomplete ancestor chain:" << col;
    return QString();
  }

  if ( col.parentCollection() == Collection::root() ) {
    kWarning( col.remoteId() != rootRemoteId() ) << "RID mismatch, is " << col.remoteId() << " expected " << rootRemoteId();
    return QString( "" );
  }
  const QString parentMailbox = mailBoxForCollection( col.parentCollection() );
  if ( parentMailbox.isNull() ) // invalid, != isEmpty() here!
    return QString();

  const QString mailbox =  parentMailbox + col.remoteId();
  if ( parentMailbox.isEmpty() )
    return mailbox.mid( 1 ); // strip of the separator on top-level mailboxes
  return mailbox;
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

void ImapResource::doSetOnline(bool online)
{
  if ( !online && isSessionAvailable() ) {
    m_account->disconnect();
  } else if ( online ) {
    startConnect();
  }
  ResourceBase::doSetOnline( online );
}

bool ImapResource::needsNetwork() const
{
  const QString hostName = Settings::self()->imapServer().section( ':', 0, 0 );
  // ### is there a better way to do this?
  if ( hostName == QLatin1String( "127.0.0.1" ) ||
       hostName == QLatin1String( "localhost" ) ||
       hostName == QHostInfo::localHostName() ) {
    return false;
  }
  return true;
}

bool ImapResource::isSessionAvailable() const
{
  return m_account && m_account->mainSession()
      && m_account->mainSession()->state() != KIMAP::Session::Disconnected;
}

void ImapResource::reconnect()
{
  setNeedsNetwork( needsNetwork() );
  setOnline( false ); // we are not connected initially
  setOnline( !needsNetwork() ||
             Solid::Networking::status() == Solid::Networking::Unknown ||
             Solid::Networking::status() == Solid::Networking::Connected );
}

void ImapResource::startIdle()
{
  delete m_idle;
  m_idle = 0;

  if ( !m_account || !m_account->capabilities().contains( "IDLE" ) )
    return;

  const QStringList ridPath = Settings::self()->idleRidPath();
  if ( ridPath.size() < 2 )
    return;

  Collection c, p;
  p.setParentCollection( Collection::root() );
  for ( int i = ridPath.size() - 1; i > 0; --i ) {
    p.setRemoteId( ridPath.at( i ) );
    c.setParentCollection( p );
    p = c;
  }
  c.setRemoteId( ridPath.first() );

  Akonadi::CollectionFetchScope scope;
  scope.setResource( identifier() );

  Akonadi::CollectionFetchJob *fetch
    = new Akonadi::CollectionFetchJob( c, Akonadi::CollectionFetchJob::Base, this );
  fetch->setFetchScope( scope );
  fetch->setProperty( "mailBox", mailBoxForCollection( c ) );

  connect( fetch, SIGNAL(result(KJob*)),
           this, SLOT(onIdleCollectionFetchDone(KJob*)) );
}

void ImapResource::onIdleCollectionFetchDone( KJob *job )
{
  const QString mailBox = job->property( "mailBox" ).toString();

  if ( job->error() == 0 ) {
    Akonadi::CollectionFetchJob *fetch = static_cast<Akonadi::CollectionFetchJob*>( job );
    Akonadi::Collection c = fetch->collections().first();

    const QString password = Settings::self()->password();
    if ( password.isEmpty() )
      return;

    m_idle = new ImapIdleManager( c, mailBox,
                                  m_account->extraSession( "idle", password ),
                                  this );

  } else {
    kWarning() << "CollectionFetch for mail box "
               << mailBox << "failed. error="
               << job->error() << ", errorString=" << job->errorString();
  }
}

void ImapResource::requestManualExpunge( qint64 collectionId )
{
  if ( !Settings::self()->automaticExpungeEnabled() ) {
    scheduleCustomTask( this, "expungeRequested",
                        QVariant::fromValue( Collection( collectionId ) ) );
  }
}

void ImapResource::expungeRequested( const QVariant &collectionArgument )
{
  const Collection collection = collectionArgument.value<Collection>();

  if ( collection.isValid() ) {
    Akonadi::CollectionFetchScope scope;
    scope.setResource( identifier() );
    scope.setAncestorRetrieval( Akonadi::CollectionFetchScope::All );

    Akonadi::CollectionFetchJob *fetch
      = new Akonadi::CollectionFetchJob( collection,
                                         Akonadi::CollectionFetchJob::Base,
                                         this );
    fetch->setFetchScope( scope );
    fetch->setProperty( AKONADI_COLLECTION, collection.id() );

    connect( fetch, SIGNAL(result(KJob*)),
             this, SLOT(onExpungeCollectionFetchDone(KJob*)) );
  } else {
    changeProcessed();
  }
}

void ImapResource::onExpungeCollectionFetchDone( KJob *job )
{
  const Collection::Id collectionId = job->property( AKONADI_COLLECTION ).toLongLong();

  if ( job->error() == 0 ) {
    Akonadi::CollectionFetchJob *fetch = static_cast<Akonadi::CollectionFetchJob*>( job );

    foreach ( const Akonadi::Collection &c, fetch->collections() ) {
      if ( c.id() == collectionId ) {
        const QString mailBox = mailBoxForCollection( c );

        if ( !mailBox.isEmpty() ) {
          triggerExpunge( mailBox );
        }
        break;
      }
    }
  } else {
    kWarning() << "CollectionFetch for collection "
               << collectionId << "failed. error="
               << job->error() << ", errorString=" << job->errorString();
  }

  changeProcessed();
}

AKONADI_RESOURCE_MAIN( ImapResource )

#include "imapresource.moc"


