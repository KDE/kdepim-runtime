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
#include "settingsadaptor.h"
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

#include <kio/ksslcertificatemanager.h>

#include <kimap/session.h>
#include <kimap/sessionuiproxy.h>
#include <kimap/fetchjob.h>
#include <kimap/listjob.h>
#include <kimap/loginjob.h>
#include <kimap/logoutjob.h>
#include <kimap/selectjob.h>
#include <kimap/statusjob.h>

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
#include <akonadi/session.h>
#include <akonadi/transactionsequence.h>

using namespace Akonadi;

class SessionUiProxy : public KIMAP::SessionUiProxy {
  public:
    bool ignoreSslError(const KSslErrorUiData& errorData) {
      if (KSslCertificateManager::askIgnoreSslErrors(errorData, KSslCertificateManager::StoreRules)) {
        return true;
      } else {
        return false;
      }
    }
};

ImapResource::ImapResource( const QString &id )
        :ResourceBase( id )
{
  Akonadi::AttributeFactory::registerAttribute<UidValidityAttribute>();
  Akonadi::AttributeFactory::registerAttribute<UidNextAttribute>();
  Akonadi::AttributeFactory::registerAttribute<NoSelectAttribute>();

  changeRecorder()->fetchCollection( true );
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                                                Settings::self(), QDBusConnection::ExportAdaptors );

  startConnect();
}

ImapResource::~ImapResource()
{
}

bool ImapResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
    const QString remoteId = item.remoteId();
    const QStringList temp = remoteId.split( "-+-" );
    QByteArray mailBox = temp[0].toUtf8();
    mailBox.replace( rootRemoteId().toUtf8(), "" );
    const QByteArray uid = temp[1].toUtf8();

    m_itemCache[remoteId] = item;

    KIMAP::SelectJob *select = new KIMAP::SelectJob( m_session );
    select->setMailBox( mailBox );
    select->start();
    KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_session );
    KIMAP::FetchJob::FetchScope scope;
    fetch->setUidBased( true );
    fetch->setSequenceSet( uid+':'+uid );
    scope.parts.clear();// = parts.toList();
    scope.mode = KIMAP::FetchJob::FetchScope::Content;
    fetch->setScope( scope );
    connect( fetch, SIGNAL( messageReceived( QByteArray, qint64, int, boost::shared_ptr<KMime::Message> ) ),
             this, SLOT( onMessageReceived( QByteArray, qint64, int, boost::shared_ptr<KMime::Message> ) ) );
    connect( fetch, SIGNAL( partReceived( QByteArray, qint64, int, QByteArray, boost::shared_ptr<KMime::Content> ) ),
             this, SLOT( onPartReceived( QByteArray, qint64, int, QByteArray, boost::shared_ptr<KMime::Content> ) ) );
    connect( fetch, SIGNAL( result( KJob* ) ),
             this, SLOT( onContentFetchDone( KJob* ) ) );
    fetch->start();
    return true;
}

void ImapResource::onMessageReceived( const QByteArray &mailBox, qint64 uid, int /*messageNumber*/, boost::shared_ptr<KMime::Message> message )
{
  const QString remoteId =  mailBoxRemoteId( mailBox ) + "-+-" + QString::number( uid );

  Item i = m_itemCache.take(remoteId);

  kDebug() << "MESSAGE from Imap server" << remoteId;
  Q_ASSERT( i.isValid() );

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

  if ( !Settings::self()->imapServer().isEmpty() && !Settings::self()->userName().isEmpty() ) {
    setName( Settings::self()->imapServer() + '/' + Settings::self()->userName() );
  } else {
    setName( KGlobal::mainComponent().aboutData()->appName() );
  }

  startConnect();
}

void ImapResource::startConnect( bool forceManualAuth )
{
  m_server = Settings::self()->imapServer();
  int safe = Settings::self()->safety();
  int auth = Settings::self()->authentication();

  if ( m_server.isEmpty() ) {
    return;
  }

  QString server = m_server.section( ":", 0, 0 );
  int port = m_server.section( ":", 1, 1 ).toInt();

  if (( safe == 1 || safe == 2 ) && !QSslSocket::supportsSsl() ) {
    kWarning() << "Crypto not supported!";
    emit error( i18n( "You requested TLS/SSL to connect to %1, but your "
                      "system does not seem to be set up for that.", m_server ) );
    return;
  }

  if ( port == 0 ) {
    switch ( safe ) {
    case 1:
    case 3:
      port = 143;
      break;
    case 2:
      port = 993;
      break;
    }
  }

  m_session = new KIMAP::Session( server, port, this );
  m_session->setUiProxy( new SessionUiProxy );

  m_userName =  Settings::self()->userName();
  QString password = Settings::self()->password();

  if ( password.isEmpty() || forceManualAuth ) {
    if ( !manualAuth( m_userName, password ) ) {
      KIMAP::LogoutJob *job = new KIMAP::LogoutJob( m_session );
      job->start();
      return;
    }
  }

  KIMAP::LoginJob *loginJob = new KIMAP::LoginJob( m_session );
  loginJob->setUserName( m_userName );
  loginJob->setPassword( password );

  switch ( safe ) {
  case 1:
    break;
  case 2:
    kFatal("Sorry, not implemented yet: ImapResource::startConnect with SSL support");
    break;
  case 3:
    loginJob->setEncryptionMode( KIMAP::LoginJob::TlsV1 );
    break;
  default:
    kFatal("Shouldn't happen...");
  }

  switch ( auth ) {
  case 1:
    loginJob->setAuthenticationMode( KIMAP::LoginJob::ClearText );
    break;
  case 2:
    loginJob->setAuthenticationMode( KIMAP::LoginJob::Login );
    break;
  case 3:
    loginJob->setAuthenticationMode( KIMAP::LoginJob::Plain );
    break;
  case 4:
    loginJob->setAuthenticationMode( KIMAP::LoginJob::CramMD5 );
    break;
  case 5:
    loginJob->setAuthenticationMode( KIMAP::LoginJob::DigestMD5 );
    break;
  case 6:
    loginJob->setAuthenticationMode( KIMAP::LoginJob::NTLM );
    break;
  case 7:
    loginJob->setAuthenticationMode( KIMAP::LoginJob::GSSAPI );
    break;
  case 8:
    loginJob->setAuthenticationMode( KIMAP::LoginJob::Anonymous );
    break;
  default:
    kFatal("Shouldn't happen...");
  }

  connect( loginJob, SIGNAL( result(KJob*) ),
           this, SLOT( onLoginDone(KJob*) ) );
  loginJob->start();
}

void ImapResource::itemAdded( const Akonadi::Item & item, const Akonadi::Collection& collection )
{
#ifdef KIMAP_PORT_TEMPORARILY_REMOVED
    m_itemAdded = item;
    m_collection = collection;
    const QString mailbox = collection.remoteId();
    kDebug() << "mailbox:" << mailbox;

    // save message to the server.
    MessagePtr msg = item.payload<MessagePtr>();
    m_imap->saveMessage( mailbox, msg->encodedContent( true ) + "\r\n" );
#else // KIMAP_PORT_TEMPORARILY_REMOVED
    kFatal("Sorry, not implemented: ImapResource::itemAdded");
    return ;
#endif // KIMAP_PORT_TEMPORARILY_REMOVED
}

void ImapResource::slotSaveDone( int uid )
{
#ifdef KIMAP_PORT_TEMPORARILY_REMOVED
    if ( uid > -1 ) {
        const QString reference =  m_collection.remoteId() + "-+-" + QString::number( uid );
        kDebug() << "Setting reference to " << reference;
        m_itemAdded.setRemoteId( reference );
    }
    changeCommitted( m_itemAdded );
#else // KIMAP_PORT_TEMPORARILY_REMOVED
    kFatal("Sorry, not implemented: ImapResource::slotSaveDone");
    return ;
#endif // KIMAP_PORT_TEMPORARILY_REMOVED
}

void ImapResource::itemChanged( const Akonadi::Item& item, const QSet<QByteArray>& parts )
{
#ifdef KIMAP_PORT_TEMPORARILY_REMOVED
    // We can just assume that we only get here if the flags have changed. So get them and
    // store those on the server.
    QSet<QByteArray> flags = item.flags();
    QByteArray newFlags;
    foreach( const QByteArray &flag, flags )
    newFlags += flag + ' ';

    // kDebug( ) << "Flags going to be set" << newFlags;
    const QString reference = item.remoteId();
    const QStringList temp = reference.split( "-+-" );
    m_imap->setFlags( temp[0], temp[1].toInt(), temp[1].toInt(), newFlags );

    changeCommitted( item );
#else // KIMAP_PORT_TEMPORARILY_REMOVED
    kFatal("Sorry, not implemented: ImapResource::itemChanged");
    return ;
#endif // KIMAP_PORT_TEMPORARILY_REMOVED
}

void ImapResource::itemRemoved( const Akonadi::Item &item )
{
#ifdef KIMAP_PORT_TEMPORARILY_REMOVED
    //itemRemoved actually can not be implemented. The imap specs do not allow for a single
    //message to be deleted. Users of this resource should make sure they set the flag
    //to Deleted and call the DBus purge method when you want to get rid of the items
    //from the server.
    changeProcessed();
#else // KIMAP_PORT_TEMPORARILY_REMOVED
    kFatal("Sorry, not implemented: ImapResource::itemRemoved");
    return ;
#endif // KIMAP_PORT_TEMPORARILY_REMOVED
}

void ImapResource::retrieveCollections()
{
  if ( !m_session ) {
    kDebug() << "Ignoring this request. Probably there is no connection.";
    return;
  }

  Collection root;
  root.setName( m_server + '/' + m_userName );
  root.setRemoteId( rootRemoteId() );
  root.setContentMimeTypes( QStringList( Collection::mimeType() ) );

  CachePolicy policy;
  policy.setInheritFromParent( false );
  policy.setSyncOnDemand( true );
  root.setCachePolicy( policy );

  collectionsRetrievedIncremental( Collection::List() << root, Collection::List() );

  KIMAP::ListJob *listJob = new KIMAP::ListJob( m_session );
  listJob->setIncludeUnsubscribed( true );
  connect( listJob, SIGNAL( mailBoxReceived(QList<QByteArray>, QList<QByteArray>) ),
           this, SLOT( onMailBoxReceived(QList<QByteArray>, QList<QByteArray>) ) );
  listJob->start();
}

void ImapResource::onMailBoxReceived( const QList<QByteArray> &descriptor,
                                      const QList<QByteArray> &flags )
{
  QList<QByteArray> pathParts = descriptor;
  QByteArray separator = pathParts.takeFirst();

  QString parentPath;
  QString currentPath;
  Collection::List collections;

  QStringList contentTypes;
  contentTypes << "message/rfc822" << Collection::mimeType();

  foreach ( const QByteArray &pathPart, pathParts ) {
    currentPath+='/'+pathPart;
    if ( currentPath.startsWith( '/' ) ) {
      currentPath.remove( 0, 1 );
    }

    Collection c;
    c.setName( pathPart );
    c.setRemoteId( mailBoxRemoteId( currentPath ) );
    c.setParentRemoteId( mailBoxRemoteId( parentPath ) );
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
    if ( flags.contains( "\\NoSelect" ) ) {
      cachePolicy.setSyncOnDemand( false );
      c.addAttribute( new NoSelectAttribute( true ) );
    }

    c.setCachePolicy( cachePolicy );

    collections << c;
    parentPath = currentPath;
  }

  collectionsRetrievedIncremental( collections, Collection::List() );
}

// ----------------------------------------------------------------------------------

void ImapResource::retrieveItems( const Akonadi::Collection & col )
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

  KIMAP::StatusJob *statusJob = new KIMAP::StatusJob( m_session );
  statusJob->setMailBox( col.remoteId().replace( rootRemoteId(), "" ).toUtf8() );
  connect( statusJob, SIGNAL( status( QByteArray, int, qint64, int ) ),
           this, SLOT( onStatusReceived( QByteArray, int, qint64, int ) ) );
  connect( statusJob, SIGNAL( result( KJob* ) ),
           this, SLOT( onStatusDone( KJob* ) ) );
  statusJob->start();
}

void ImapResource::onHeadersReceived( const QByteArray &mailBox, qint64 uid, int /*messageNumber*/,
                                      qint64 size, QList<QByteArray> flags,
                                      boost::shared_ptr<KMime::Message> message )
{
  Akonadi::Item i;
  i.setRemoteId( mailBoxRemoteId( mailBox ) + "-+-" + QString::number( uid ) );
  i.setMimeType( "message/rfc822" );
  i.setPayload( MessagePtr( message ) );
  i.setSize( size );

  foreach( const QByteArray &flag, flags ) {
    i.setFlag( flag );
  }
  kDebug() << "Flags: " << i.flags();

  itemsRetrievedIncremental( Item::List() << i, Item::List() );
}

void ImapResource::onHeadersFetchDone( KJob */*job*/ )
{
  itemsRetrievalDone();
}


// ----------------------------------------------------------------------------------

void ImapResource::collectionAdded( const Collection & collection, const Collection &parent )
{
#ifdef KIMAP_PORT_TEMPORARILY_REMOVED
    const QString remoteName = parent.remoteId() + '.' + collection.name();
    kDebug( ) << "New folder: " << remoteName;

    m_collection = collection;
    m_imap->createMailBox( remoteName );
    m_collection.setRemoteId( remoteName );
#else // KIMAP_PORT_TEMPORARILY_REMOVED
    kFatal("Sorry, not implemented: ImapResource::collectionAdded");
    return ;
#endif // KIMAP_PORT_TEMPORARILY_REMOVED
}

void ImapResource::slotCollectionAdded( bool success )
{
#ifdef KIMAP_PORT_TEMPORARILY_REMOVED
    // finish the task.
    changeCommitted( m_collection );

    if ( !success ) {
        // remove the collection again.
        // TODO: this will trigger collectionRemoved again ;-)
        kDebug() << "Failed to create the folder, deleting it in akonadi again";
        emit warning( i18n( "Failed to create the folder, restoring folder list." ) );
        new CollectionDeleteJob( m_collection, this );
    }
#else // KIMAP_PORT_TEMPORARILY_REMOVED
    kFatal("Sorry, not implemented: ImapResource::slotCollectionAdded");
    return ;
#endif // KIMAP_PORT_TEMPORARILY_REMOVED
}

void ImapResource::collectionChanged( const Collection & collection )
{
#ifdef KIMAP_PORT_TEMPORARILY_REMOVED
    kDebug( ) << "Implement me!";
    changeProcessed();
#else // KIMAP_PORT_TEMPORARILY_REMOVED
    kFatal("Sorry, not implemented: ImapResource::collectionChanged");
    return ;
#endif // KIMAP_PORT_TEMPORARILY_REMOVED
}

void ImapResource::collectionRemoved( const Akonadi::Collection &collection )
{
#ifdef KIMAP_PORT_TEMPORARILY_REMOVED
    kDebug( ) << "Del folder: " << collection.id() << collection.remoteId();
    m_imap->deleteMailBox( collection.remoteId() );
#else // KIMAP_PORT_TEMPORARILY_REMOVED
    kFatal("Sorry, not implemented: ImapResource::collectionRemoved");
    return ;
#endif // KIMAP_PORT_TEMPORARILY_REMOVED
}

void ImapResource::slotCollectionRemoved( bool success )
{
#ifdef KIMAP_PORT_TEMPORARILY_REMOVED
    // finish the task.
    changeProcessed();

    if ( !success ) {
        kDebug() << "Failed to delete the folder, resync the folder tree";
        emit warning( i18n( "Failed to delete the folder, restoring folder list." ) );
        synchronizeCollectionTree();
    }
#else // KIMAP_PORT_TEMPORARILY_REMOVED
    kFatal("Sorry, not implemented: ImapResource::slotCollectionRemoved");
    return ;
#endif // KIMAP_PORT_TEMPORARILY_REMOVED
}

/******************* Slots  ***********************************************/

void ImapResource::onLoginDone( KJob *job )
{
  if ( job->error()==0 ) {
    synchronizeCollectionTree();
    return;
  }

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
  } else if ( i == KMessageBox::No ) {
    startConnect( true );
  } else {
    KIMAP::LogoutJob *logout = new KIMAP::LogoutJob( m_session );
    logout->start();
    emit warning( i18n( "Could not connect to the IMAP-server %1.", m_server ) );
  }
}

void ImapResource::slotAlert( Imaplib*, const QString& message )
{
#ifdef KIMAP_PORT_TEMPORARILY_REMOVED
    emit error( i18n( "Server reported: %1.",message ) );
#else // KIMAP_PORT_TEMPORARILY_REMOVED
    kFatal("Sorry, not implemented: ImapResource::slotAlert");
    return ;
#endif // KIMAP_PORT_TEMPORARILY_REMOVED
}

void ImapResource::slotPurge( const QString &folder )
{
#ifdef KIMAP_PORT_TEMPORARILY_REMOVED
    m_imap->expungeMailBox( folder );
#else // KIMAP_PORT_TEMPORARILY_REMOVED
    kFatal("Sorry, not implemented: ImapResource::slotPurge");
    return ;
#endif // KIMAP_PORT_TEMPORARILY_REMOVED
}

void ImapResource::onStatusReceived( const QByteArray &mailBox, int messageCount,
                                     qint64 uidValidity, int nextUid )
{
  // uidvalidity can change between sessions, we don't want to refetch
  // folders in that case. Keep track of what is processed and what not.
  static QList<QByteArray> processed;
  bool firstTime = false;
  if ( processed.indexOf( mailBox ) == -1 ) {
    firstTime = true;
    processed.append( mailBox );
  }

  Collection collection = collectionFromRemoteId( mailBoxRemoteId( mailBox ) );
  Q_ASSERT( collection.isValid() );

  // Get the current uid next value and store it
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

    KIMAP::SelectJob *select = new KIMAP::SelectJob( m_session );
    select->setMailBox( mailBox );
    select->start();
    KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_session );
    KIMAP::FetchJob::FetchScope scope;
    fetch->setSequenceSet( "1:"+QByteArray::number( messageCount ) );
    scope.parts.clear();
    scope.mode = KIMAP::FetchJob::FetchScope::Headers;
    fetch->setScope( scope );
    connect( fetch, SIGNAL( headersReceived( QByteArray, qint64, int, qint64, QList<QByteArray>, boost::shared_ptr<KMime::Message> ) ),
             this, SLOT( onHeadersReceived( QByteArray, qint64, int, qint64, QList<QByteArray>, boost::shared_ptr<KMime::Message> ) ) );
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

    KIMAP::SelectJob *select = new KIMAP::SelectJob( m_session );
    select->setMailBox( mailBox );
    select->start();
    KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_session );
    KIMAP::FetchJob::FetchScope scope;
    fetch->setSequenceSet( QByteArray::number( realMessageCount+1 )+':'+QByteArray::number( messageCount ) );
    scope.parts.clear();
    scope.mode = KIMAP::FetchJob::FetchScope::Headers;
    fetch->setScope( scope );
    connect( fetch, SIGNAL( headersReceived( QByteArray, qint64, int, qint64, QList<QByteArray>, boost::shared_ptr<KMime::Message> ) ),
             this, SLOT( onHeadersReceived( QByteArray, qint64, int, qint64, QList<QByteArray>, boost::shared_ptr<KMime::Message> ) ) );
    connect( fetch, SIGNAL( result( KJob* ) ),
             this, SLOT( onHeadersFetchDone( KJob* ) ) );
    fetch->start();
    return;
  } else if ( messageCount != realMessageCount ) {
    // The amount on the server does not match the amount in the cache.
    // that means we need reget the catch completely.
    kDebug() << "O OH: " << messageCount << " But: " << realMessageCount;

    itemsClear();
    setItemStreamingEnabled( true );

    KIMAP::SelectJob *select = new KIMAP::SelectJob( m_session );
    select->setMailBox( mailBox );
    select->start();
    KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_session );
    KIMAP::FetchJob::FetchScope scope;
    fetch->setSequenceSet( "1:"+QByteArray::number( messageCount ) );
    scope.parts.clear();
    scope.mode = KIMAP::FetchJob::FetchScope::Headers;
    fetch->setScope( scope );
    connect( fetch, SIGNAL( headersReceived( QByteArray, qint64, int, qint64, QList<QByteArray>, boost::shared_ptr<KMime::Message> ) ),
             this, SLOT( onHeadersReceived( QByteArray, qint64, int, qint64, QList<QByteArray>, boost::shared_ptr<KMime::Message> ) ) );
    connect( fetch, SIGNAL( result( KJob* ) ),
             this, SLOT( onHeadersFetchDone( KJob* ) ) );
    fetch->start();
    return;
  } else if ( messageCount == realMessageCount && oldNextUid != nextUid
           && oldNextUid != 0 && !firstTime ) {
    // amount is right but uidnext is different.... something happened
    // behind our back...
    kDebug() << "UIDNEXT check failed, refetching mailbox";

    itemsClear();
    setItemStreamingEnabled( true );

    KIMAP::SelectJob *select = new KIMAP::SelectJob( m_session );
    select->setMailBox( mailBox );
    select->start();
    KIMAP::FetchJob *fetch = new KIMAP::FetchJob( m_session );
    KIMAP::FetchJob::FetchScope scope;
    fetch->setSequenceSet( "1:"+QByteArray::number( messageCount ) );
    scope.parts.clear();
    scope.mode = KIMAP::FetchJob::FetchScope::Headers;
    fetch->setScope( scope );
    connect( fetch, SIGNAL( headersReceived( QByteArray, qint64, int, qint64, QList<QByteArray>, boost::shared_ptr<KMime::Message> ) ),
             this, SLOT( onHeadersReceived( QByteArray, qint64, int, qint64, QList<QByteArray>, boost::shared_ptr<KMime::Message> ) ) );
    connect( fetch, SIGNAL( result( KJob* ) ),
             this, SLOT( onHeadersFetchDone( KJob* ) ) );
    fetch->start();
    return;
  }

  kDebug() << "All fine, nothing to do";
  itemsRetrievalDone();
}

void ImapResource::onStatusDone( KJob *job )
{
    if ( job->error() ) {
      itemsRetrievalDone();
    }
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
  return "imap://"+m_userName+'@'+m_server+'/';
}

QString ImapResource::mailBoxRemoteId( const QString &path ) const
{
  return rootRemoteId()+path;
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

void ImapResource::itemsClear()
{
  ItemFetchJob *fetch = new ItemFetchJob( currentCollection() );
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


