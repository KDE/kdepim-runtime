/*
    Copyright (c) 2007 Till Adam <adam@kde.org>
    Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>

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

#include "imaplibresource.h"
#include "setupserver.h"
#include "settingsadaptor.h"
#include "uidvalidityattribute.h"
#include "uidnextattribute.h"
#include "noselectattribute.h"
#include "imaplib.h"

#include <QtCore/QDebug>
#include <QtDBus/QDBusConnection>

#include <kdebug.h>
#include <klocale.h>
#include <kpassworddialog.h>
#include <kmessagebox.h>
#include <KWindowSystem>
#include <KAboutData>

#include <kmime/kmime_message.h>

#include <boost/shared_ptr.hpp>
typedef boost::shared_ptr<KMime::Message> MessagePtr;

#include <akonadi/cachepolicy.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/collectionstatisticsjob.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/monitor.h>
#include <akonadi/changerecorder.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/session.h>

using namespace Akonadi;

ImaplibResource::ImaplibResource( const QString &id )
        :ResourceBase( id ), m_imap( 0 ), m_incrementalFetch( false )
{
    changeRecorder()->fetchCollection( true );
    new SettingsAdaptor( Settings::self() );
    QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
            Settings::self(), QDBusConnection::ExportAdaptors );

    startConnect();
}

ImaplibResource::~ImaplibResource()
{
    delete m_imap;
}

bool ImaplibResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
    Q_UNUSED( parts );

    const QString reference = item.remoteId();
    kDebug( ) << "Fetch request for" << reference;
    const QStringList temp = reference.split( "-+-" );
    m_imap->getMessage( temp[0], temp[1].toInt() );
    m_itemCache[reference] = item;
    return true;
}

void ImaplibResource::slotMessageReceived( Imaplib*, const QString& mb, int uid,
        const QString& body )
{
    const QString reference =  mb + "-+-" + QString::number( uid );

    kDebug() << "MESSAGE from Imap server" << reference;
    Q_ASSERT( m_itemCache.value( reference ).isValid() );

    KMime::Message *mail = new KMime::Message();
    mail->setContent( KMime::CRLFtoLF( body.toLatin1() ) );
    mail->parse();

    Item i( m_itemCache.value( reference ) );
    i.setMimeType( "message/rfc822" );
    i.setPayload( MessagePtr( mail ) );

    kDebug() << "Has Payload: " << i.hasPayload();

    itemRetrieved( i );
}

void ImaplibResource::configure( WId windowId )
{
    SetupServer dlg( windowId );
    KWindowSystem::setMainWindow( &dlg, windowId );
    dlg.exec();
    if ( !Settings::self()->imapServer().isEmpty() && !Settings::self()->username().isEmpty() )
        setName( Settings::self()->imapServer() + '/' + Settings::self()->username() );
    else
        setName( KGlobal::mainComponent().aboutData()->appName() );
    startConnect();
}

void ImaplibResource::startConnect()
{
    m_server = Settings::self()->imapServer();
    int safe = Settings::self()->safety();

    if ( m_server.isEmpty() ) {
        return;
    }

    QString server = m_server.section( ":",0,0 );
    int port = m_server.section( ":",1,1 ).toInt();

    m_imap = new Imaplib( 0,"serverconnection" );

    if (( safe == 1 || safe == 2 ) && !QSslSocket::supportsSsl() ) {
        kWarning() << "Crypto not supported!";
        emit error( i18n( "You requested TLS/SSL to connect to %1, but your "
                          "system does not seem to be set up for that.", m_server ) );
        return;
    }

    connections(); // todo: double connections on reconnect?
    m_imap->startConnection( server, port, safe );
}

void ImaplibResource::itemAdded( const Akonadi::Item & item, const Akonadi::Collection& collection )
{
    m_itemAdded = item;
    m_collection = collection;
    const QString mailbox = collection.remoteId();
    kDebug() << "mailbox:" << mailbox;

    // save message to the server.
    MessagePtr msg = item.payload<MessagePtr>();
    m_imap->saveMessage( mailbox, msg->encodedContent( true ) + "\r\n" );
}

void ImaplibResource::slotSaveDone( int uid )
{
    if ( uid > -1 ) {
        const QString reference =  m_collection.remoteId() + "-+-" + QString::number( uid );
        kDebug() << "Setting reference to " << reference;
        m_itemAdded.setRemoteId( reference );
    }
    changeCommitted( m_itemAdded );
}

void ImaplibResource::itemChanged( const Akonadi::Item& item, const QSet<QByteArray>& parts )
{
    // We can just assume that we only get here if the flags have changed. So get them and
    // store those on the server.
    QSet<QByteArray> flags = item.flags();
    QByteArray newFlags;
    foreach( const QByteArray &flag, flags )
    newFlags += flag + ' ';

    kDebug( ) << "Flags going to be set" << newFlags;
    const QString reference = item.remoteId();
    const QStringList temp = reference.split( "-+-" );
    m_imap->setFlags( temp[0], temp[1].toInt(), temp[1].toInt(), newFlags );

    changeCommitted( item );
}

void ImaplibResource::itemRemoved( const Akonadi::Item &item )
{
    //itemRemoved actually can not be implemented. The imap specs do not allow for a single
    //message to be deleted. Users of this resource should make sure they set the the flag
    //to Deleted and call the DBus purge method when you want to get rid of the items
    //from the server.
    changeProcessed();
}

void ImaplibResource::retrieveCollections()
{
    if ( !m_imap ) {
        kDebug() << "Ignoring this request. Probably there is no connection.";
        return;
    }
    kDebug();
    m_imap->getMailBoxList();
}

static QString findParent( QHash<QString, Collection> &collections, const Collection &root, const QStringList &_path )
{
    QStringList path = _path;
    path.removeLast();
    if ( path.isEmpty() )
        return root.remoteId();
    const QString id = path.join( "." ); // ### is this always the correct path separator?
    if ( collections.contains( id ) )
        return collections.value( id ).remoteId();
    Collection c;
    c.setName( path.last() );
    c.setRemoteId( id );
    c.setParentRemoteId( findParent( collections, root, path ) );
    c.setContentMimeTypes( QStringList( Collection::mimeType() ) );
    collections.insert( id, c );
    return c.remoteId();
}

void ImaplibResource::slotFolderListReceived( const QStringList& list, const QStringList& noselectfolders )
{
    QHash<QString, Collection> collections;
    QStringList contentTypes;
    contentTypes << "message/rfc822" << Collection::mimeType();

    Collection root;
    root.setName( m_server + '/' + m_username );
    root.setRemoteId( "temporary random unique identifier" ); // ### should be the server url or similar
    root.setContentMimeTypes( QStringList( Collection::mimeType() ) );

    CachePolicy policy;
    policy.setInheritFromParent( false );
    policy.setSyncOnDemand( true );
    root.setCachePolicy( policy );

    collections.insert( root.remoteId(), root );

    QStringList::ConstIterator it = list.begin();
    while ( it != list.end() ) {
        QStringList path = ( *it ).split( '.' ); // ### is . always the path separator?
        Q_ASSERT( !path.isEmpty() );
        Collection c;
        if ( collections.contains( *it ) ) {
            c = collections.value( *it );
        } else {
            c.setName( path.last() );
        }
        c.setRemoteId( *it );
        c.setRights( Collection::AllRights );
        c.setContentMimeTypes( contentTypes );
        c.setParentRemoteId( findParent( collections, root, path ) );

        CachePolicy itemPolicy;
        itemPolicy.setInheritFromParent( false );
        itemPolicy.setIntervalCheckTime( -1 );
        itemPolicy.setSyncOnDemand( true );

        // If the folder is the Inbox, make some special settings.
        if ( (*it).compare( QLatin1String("INBOX") , Qt::CaseInsensitive ) == 0 ) {
            itemPolicy.setIntervalCheckTime( 1 );
            c.setName( "Inbox" );
        }

        // If this folder is a noselect folder, make some special settings.
        if ( noselectfolders.contains( *it ) ) {
            itemPolicy.setSyncOnDemand( false );
            c.addAttribute( new NoSelectAttribute( true ) );
        }
        c.setCachePolicy( itemPolicy );

        collections[ *it ] = c;
        ++it;
    }

    collectionsRetrieved( collections.values() );
}

// ----------------------------------------------------------------------------------

void ImaplibResource::retrieveItems( const Akonadi::Collection & col )
{
    kDebug( ) << col.remoteId();
    m_collection = col;

    // If there is new mail, we get it without giubg through integrity, so the default is true!
    m_incrementalFetch = true;

    // Prevent fetching items from noselect folders.
    if ( m_collection.hasAttribute( "noselect" ) ) {
        NoSelectAttribute* noselect = static_cast<NoSelectAttribute*>( m_collection.attribute( "noselect" ) );
        if ( noselect->noSelect() ) {
            kDebug() << "No Select folder";
            itemsRetrievalDone();
            return;
        }
    }

    m_imap->checkMail( col.remoteId() );
}

void ImaplibResource::slotMessagesInFolder( Imaplib*, const QString& mb, int amount )
{
    kDebug( ) << mb << amount;

}

void ImaplibResource::slotUidsAndFlagsReceived( Imaplib*,const QString& mb,const QStringList& values )
{
    kDebug( ) << mb << values.count();

    if ( values.count() == 0 ) {
        itemsRetrievalDone();
        return;
    }

    // results contain the uid and the flags for each item in this folder.
    // we will ignore the fact that we already have items.
    QStringList fetchlist;
    QStringList::ConstIterator it = values.begin();
    while ( it != values.end() ) {
        const QString uid = ( *it );
        ++it;

        m_flagsCache[mb + "-+-" + uid] = *it;
        ++it;

        //  if (all.indexOf(uid) == -1)

        fetchlist.append( uid );
    }

    // if we are not fetching the whole folder, we can not do it in batches, so don't set
    // the totalItems, as that would put it in batch mode.
    setTotalItems( fetchlist.count() );

    m_imap->getHeaders( mb, fetchlist );
}

void ImaplibResource::slotHeadersReceived( Imaplib*, const QString& mb, const QStringList& list )
{
    kDebug( ) << mb << list.count();

    Item::List messages;

    QStringList::ConstIterator it = list.begin();
    while ( it != list.end() ) {
        const QString uid = ( *it );
        ++it;

        const QString mbox = ( *it );
        ++it;

        const QString headers = ( *it );
        ++it;

        KMime::Message* mail = new KMime::Message();
        mail->setContent( KMime::CRLFtoLF( headers.trimmed().toLatin1() ) );
        mail->parse();

        Akonadi::Item i( -1 );
        i.setRemoteId( mbox + "-+-" + uid );
        i.setMimeType( "message/rfc822" );
        i.setPayload( MessagePtr( mail ) );

        foreach( const QString &flag, m_flagsCache.value( mbox + "-+-" + uid ).split( " " ) ) {
            i.setFlag( flag.toLatin1() );
        }
        kDebug() << "Flags: " << i.flags();

        messages.append( i );
    }

    kDebug() << "calling partlyretrieved with amount: " << messages.count() << "Incremental?" << m_incrementalFetch;

    m_incrementalFetch ? itemsRetrievedIncremental( messages, Item::List() ) :  itemsRetrieved( messages );
}

// ----------------------------------------------------------------------------------

void ImaplibResource::collectionAdded( const Collection & collection, const Collection &parent )
{
    const QString remoteName = parent.remoteId() + '.' + collection.name();
    kDebug( ) << "New folder: " << remoteName;

    m_collection = collection;
    m_imap->createMailBox( remoteName );
    m_collection.setRemoteId( remoteName );
}

void ImaplibResource::slotCollectionAdded( bool success )
{
    // finish the task.
    changeCommitted( m_collection );

    if ( !success ) {
        // remove the collection again.
        // TODO: this will trigger collectionRemoved again ;-)
        kDebug() << "Failed to create the folder, deleting it in akonadi again";
        emit warning( i18n( "Failed to create the folder, restoring folder list." ) );
        new CollectionDeleteJob( m_collection, this );
    }
}

void ImaplibResource::collectionChanged( const Collection & collection )
{
    kDebug( ) << "Implement me!";
    changeProcessed();
}

void ImaplibResource::collectionRemoved( const Akonadi::Collection &collection )
{
    kDebug( ) << "Del folder: " << collection.id() << collection.remoteId();
    m_imap->deleteMailBox( collection.remoteId() );
}

void ImaplibResource::slotCollectionRemoved( bool success )
{
    // finish the task.
    changeProcessed();

    if ( !success ) {
        kDebug() << "Failed to delete the folder, resync the folder tree";
        emit warning( i18n( "Failed to delete the folder, restoring folder list." ) );
        synchronizeCollectionTree();
    }
}

/******************* Slots  ***********************************************/

void ImaplibResource::slotLogin( Imaplib* connection )
{
    kDebug();

    m_username =  Settings::self()->username();
    QString pass = Settings::self()->password();

    pass.isEmpty() ? manualAuth( connection, m_username )
    : connection->login( m_username, pass );
}

void ImaplibResource::slotLoginFailed( Imaplib* connection )
{
    // the credentials where not ok....
    int i = KMessageBox::questionYesNoCancelWId( winIdForDialogs(),
            i18n( "The server refused the supplied username and password. "
                  "Do you want to go to the settings, re-enter it for one "
                  "time or do nothing?" ),
            i18n( "Could Not Log In" ),
            KGuiItem( i18n( "Settings" ) ), KGuiItem( i18n( "Single Input" ) ) );
    if ( i == KMessageBox::Yes )
        configure( winIdForDialogs() );
    else if ( i == KMessageBox::No ) {
        manualAuth( connection, m_username );
    } else {
        connection->logout();
        emit warning( i18n( "Could not connect to the IMAP-server %1", m_server ) );
    }
}

void ImaplibResource::slotAlert( Imaplib*, const QString& message )
{
    emit error( i18n( "Server reported: %1",message ) );
}

void ImaplibResource::slotPurge( const QString &folder )
{
    m_imap->expungeMailBox( folder );
}

void ImaplibResource::slotIntegrity( const QString& mb, int totalShouldBe,
                                     const QString& uidvalidity, const QString& uidnext )
{
    // uidvalidity can change between sessions, we don't want to refetch
    // folders in that case. Keep track of what is processed and what not.
    static QStringList processed;
    bool firsttime = false;
    if ( processed.indexOf( mb ) == -1 ) {
        firsttime = true;
        processed.append( mb );
    }

    // Get the current uid next value and store it
    int oldUidValidity = 0;
    if ( !m_collection.hasAttribute( "uidvalidity" ) ) {
        UidValidityAttribute* currentuidvalidity  = new UidValidityAttribute( uidvalidity.toInt() );
        m_collection.addAttribute( currentuidvalidity );
    } else {
        UidValidityAttribute* currentuidvalidity =
            static_cast<UidValidityAttribute*>( m_collection.attribute( "uidvalidity" ) );
        oldUidValidity = currentuidvalidity->uidValidity();
        if ( currentuidvalidity->uidValidity() != uidvalidity.toInt() ) {
            currentuidvalidity->setUidValidity( uidvalidity.toInt() );
        }
    }

    // Get the current uid next value and store it
    int oldUidNext = 0;
    if ( !m_collection.hasAttribute( "uidnext" ) ) {
        UidNextAttribute* currentuidnext  = new UidNextAttribute( uidnext.toInt() );
        m_collection.addAttribute( currentuidnext );
    } else {
        UidNextAttribute* currentuidnext =
            static_cast<UidNextAttribute*>( m_collection.attribute( "uidnext" ) );
        oldUidNext = currentuidnext->uidNext();
        if ( currentuidnext->uidNext() != uidnext.toInt() ) {
            currentuidnext->setUidNext( uidnext.toInt() );
        }
    }

    // First check the uidvalidity, if this has changed, it means the folder
    // has been deleted and recreated. So we wipe out the messages and
    // retrieve all.
    if ( oldUidValidity != uidvalidity.toInt() && !firsttime
            && oldUidValidity != 0 && !uidvalidity.isEmpty() ) {
        kDebug() << "UIDVALIDITY check failed (" << oldUidValidity << "|"
        << uidvalidity <<") refetching "<< mb << endl;

        m_imap->getHeaderList( mb, 1, totalShouldBe );
        return;
    }

    // See how many messages are in the folder currently
    qint64 mailsReal = m_collection.statistics().count();
    if ( mailsReal == -1 ) {
        Akonadi::CollectionStatisticsJob *job = new Akonadi::CollectionStatisticsJob( m_collection );
        if ( job->exec() ) {
            Akonadi::CollectionStatistics statistics = job->statistics();
            mailsReal = statistics.count();
        }
    }

    kDebug() << "integrity: " << mb << " should be: " << totalShouldBe << " current: " << mailsReal;

    if ( totalShouldBe > mailsReal ) {
        // The amount on the server is bigger than that we have in the cache
        // that probably means that there is new mail. Fetch missing.
        kDebug() << "Fetch missing: " << totalShouldBe << " But: " << mailsReal;
        m_incrementalFetch = true;
        m_imap->getHeaderList( mb, mailsReal+1, totalShouldBe );
        return;
    } else if ( totalShouldBe != mailsReal ) {
        // The amount on the server does not match the amount in the cache.
        // that means we need reget the catch completely.
        kDebug() << "O OH: " << totalShouldBe << " But: " << mailsReal;
        m_incrementalFetch = false;
        m_imap->getHeaderList( mb, 1, totalShouldBe );
        return;
    } else if ( totalShouldBe == mailsReal && oldUidNext != uidnext.toInt()
                && oldUidNext != 0 && !uidnext.isEmpty() && !firsttime ) {
        //buggy
        itemsRetrievalDone();
        return;

        // amount is right but uidnext is different.... something happened
        // behind our back...
        m_imap->getHeaderList( mb, 1, totalShouldBe );
        kDebug() << "UIDNEXT check failed, refetching mailbox" << endl;
    }

    kDebug() << "All fine, nothing to do";
    itemsRetrievalDone();
}

/******************* Private ***********************************************/

void ImaplibResource::connections()
{
    connect( m_imap,
             SIGNAL( login( Imaplib* ) ),
             SLOT( slotLogin( Imaplib* ) ) );
    connect( m_imap,
             SIGNAL( loginFailed( Imaplib* ) ),
             SLOT( slotLoginFailed( Imaplib* ) ) );

    connect( m_imap,
             SIGNAL( alert( Imaplib*, const QString& ) ),
             SLOT( slotAlert( Imaplib*, const QString& ) ) );

    connect( m_imap,
             SIGNAL( currentFolders( const QStringList&, const QStringList& ) ),
             SLOT( slotFolderListReceived( const QStringList&, const QStringList& ) ) );

    connect( m_imap,
             SIGNAL( messageCount( Imaplib*, const QString&, int ) ),
             SLOT( slotMessagesInFolder( Imaplib*, const QString&, int ) ) );

    connect( m_imap,
             SIGNAL( uidsAndFlagsInFolder( Imaplib*,const QString&,const QStringList& ) ),
             SLOT( slotUidsAndFlagsReceived( Imaplib*,const QString&,const QStringList& ) ) );
    connect( m_imap,
             SIGNAL( headersInFolder( Imaplib*, const QString&, const QStringList& ) ),
             SLOT( slotHeadersReceived( Imaplib*, const QString&, const QStringList& ) ) );

    connect( m_imap,
             SIGNAL( message( Imaplib*, const QString&, int, const QString& ) ),
             SLOT( slotMessageReceived( Imaplib*, const QString&, int, const QString& ) ) );

    connect( m_imap,
             SIGNAL( saveDone( int ) ),
             SLOT( slotSaveDone( int ) ) );

    connect( m_imap,
             SIGNAL( mailBoxAdded( bool ) ),
             SLOT( slotCollectionAdded( bool ) ) );
    connect( m_imap,
             SIGNAL( mailBoxDeleted( bool, const QString& ) ),
             SLOT( slotCollectionRemoved( bool ) ) );

    connect( m_imap,
             SIGNAL( integrity( const QString&, int, const QString&,
                                const QString& ) ),
             SLOT( slotIntegrity( const QString&, int, const QString&,
                                  const QString& ) ) );
    /*
    connect( m_imap,
             SIGNAL( loginOk( Imaplib* ) ),
             SIGNAL( loginOk() ) );
    connect( m_imap,
             SIGNAL( status( const QString& ) ),
             SIGNAL( status( const QString& ) ) );
    connect( m_imap,
             SIGNAL( statusReady() ),
             SIGNAL( statusReady() ) );
    connect( m_imap,
             SIGNAL( statusError( const QString& ) ),
             SIGNAL( statusError( const QString& ) ) );
    connect( m_imap,
             SIGNAL( error( const QString& ) ),
             SLOT( slotError( const QString& ) ) );
    connect( m_imap,
             SIGNAL( unexpectedDisconnect() ),
             SLOT( slotDisconnected() ) );
    connect( m_imap,
             SIGNAL( disconnected() ),
             SIGNAL( disconnected() ) );
    connect( m_imap,
             SIGNAL( mailBoxRenamed( const QString&, const QString& ) ),
             SLOT( slotMailBoxRenamed( const QString&, const QString& ) ) );
    connect( m_imap,
             SIGNAL( expungeCompleted( Imaplib*, const QString& ) ),
             SLOT( slotMailBoxExpunged( Imaplib*, const QString& ) ) );
    */
}

void ImaplibResource::manualAuth( Imaplib* connection, const QString& username )
{
    KPasswordDialog dlg( 0 /* no winId constructor available */ );
    dlg.setPrompt( i18n( "Could not find a valid password, please enter it here" ) );
    if ( dlg.exec() == QDialog::Accepted && !dlg.password().isEmpty() )
        connection->login( username, QString( dlg.password() ) );
    else
        connection->logout();
}

AKONADI_RESOURCE_MAIN( ImaplibResource )

#include "imaplibresource.moc"


