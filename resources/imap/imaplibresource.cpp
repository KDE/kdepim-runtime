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

#include <libakonadi/collectionlistjob.h>
#include <libakonadi/collectionmodifyjob.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/session.h>

#include <kmime/kmime_message.h>

using namespace Akonadi;

ImaplibResource::ImaplibResource( const QString &id )
        :ResourceBase( id ), m_retrieveItemsRequested( false )
{
    new SettingsAdaptor( Settings::self() );
    QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
            Settings::self(), QDBusConnection::ExportAdaptors );
    startConnect();
}

ImaplibResource::~ImaplibResource()
{
    delete m_imap;
}

bool ImaplibResource::retrieveItem( const Akonadi::Item &item, const QStringList &parts )
{
    const QString reference = item.reference().remoteId();
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
    SetupServer dlg;
    KWindowSystem::setMainWindow( &dlg, windowId );
    dlg.exec();
    if ( !Settings::self()->imapServer().isEmpty() && !Settings::self()->username().isEmpty() )
        setName(  Settings::self()->imapServer() + "/" + Settings::self()->username() );
    else 
        setName( KGlobal::mainComponent().aboutData()->appName() );
    startConnect();
    /*
    if ( !Settings::self()->name().isEmpty() ) {
      setName( Settings::self()->name() );
    }
    */
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

    if ((safe == 1 || safe == 2) && !QSslSocket::supportsSsl())
    {
        kWarning() << "Crypto not supported!";
        // TODO: find out how to communicate a critical failure.

        //slotError(i18n("You requested TLS/SSL, but your "
        //               "system does not seem to be set up for that."));
        return;
    }

    connections(); // todo: double connections on reconnect?
    m_imap->startConnection( server, port, safe );
}

void ImaplibResource::itemAdded( const Akonadi::Item & item, const Akonadi::Collection& collection )
{
    kDebug( ) << "Implement me!";
    changesCommitted( item );
}

void ImaplibResource::itemChanged( const Akonadi::Item& item, const QStringList& parts )
{
    kDebug( ) << "Implement me!" << item.reference().remoteId() << parts;
    changesCommitted( item );
}

void ImaplibResource::itemRemoved( const Akonadi::DataReference & ref )
{
    kDebug( ) << "Implement me!";
}

void ImaplibResource::retrieveCollections()
{
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
    c.setContentTypes( QStringList( Collection::collectionMimeType() ) );
    collections.insert( id, c );
    return c.remoteId();
}

void ImaplibResource::slotFolderListReceived( const QStringList& list )
{
    QHash<QString, Collection> collections;
    QStringList contentTypes;
    contentTypes << "message/rfc822" << Collection::collectionMimeType();

    Collection root;
    root.setName( m_server + "/" + m_username );
    root.setRemoteId( "temporary random unique identifier" ); // ### should be the server url or similar
    root.setContentTypes( QStringList( Collection::collectionMimeType() ) );

    CachePolicy policy;
    policy.setInheritFromParent( false );
    policy.enableSyncOnDemand( true );
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
        c.setContentTypes( contentTypes );
        c.setParentRemoteId( findParent( collections, root, path ) );

        kDebug( ) << "ADDING: " << ( *it );
        collections[ *it ] = c;
        ++it;
    }

    collectionsRetrieved( collections.values() );
}

// ----------------------------------------------------------------------------------

void ImaplibResource::retrieveItems( const Akonadi::Collection & col, const QStringList &parts )
{
    kDebug( ) << col.remoteId();
    m_retrieveItemsRequested = true;
    m_imap->getMailBox( col.remoteId() );
}

void ImaplibResource::slotMessagesInFolder( Imaplib*, const QString& mb, int amount )
{
    kDebug( ) << mb << amount << "Cache:" << m_amountMessagesCache.value( mb );
    if ( !m_retrieveItemsRequested ) {
        return;
    }

    // We need to remember the amount of messages in a mailbox, so we can emit
    // itemsRetrieved() at the right time when all the messages are received.

    if ( amount == 0 ) {
        m_retrieveItemsRequested = false;
        itemsRetrieved();
    } else {
        m_amountMessagesCache[ mb ] = amount;
        m_imap->getHeaderList( mb, 1, amount );
    }
}

void ImaplibResource::slotUidsAndFlagsReceived( Imaplib*,const QString& mb,const QStringList& values )
{
    kDebug( ) << mb << values.count();
    if ( !m_retrieveItemsRequested ) {
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

    m_imap->getHeaders( mb, fetchlist );
}

void ImaplibResource::slotHeadersReceived( Imaplib*, const QString& mb, const QStringList& list )
{
    kDebug( ) << mb << list.count();
    if ( !m_retrieveItemsRequested ) {
        return;
    }

    // this should hold the headers of the messages.

    static Item::List s_messages;
    static QHash<QString, int> s_amountCache;
    s_amountCache[mb] += ( list.count() / 3 );


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

        Akonadi::Item i( DataReference( -1, mbox + "-+-" + uid ) );
        i.setMimeType( "message/rfc822" );
        i.setPayload( MessagePtr( mail ) );

        foreach( QString flag, m_flagsCache.value( mbox + "-+-" + uid ).split( " " ) )
        i.setFlag( flag.toLatin1() /* ok? */ );

        s_messages.append( i );
    }

    // we should only emit this when we have received all messages, remember the messages arrive in
    // blocks of 250.
    kDebug( ) << mb << "Total received:" << s_amountCache[mb] << "Total should be:" << m_amountMessagesCache[mb];
    if ( s_amountCache[mb] >= m_amountMessagesCache[mb] ) {
        itemsRetrieved( s_messages );
        s_amountCache[mb] = 0;
        s_messages.clear();
        kDebug() << "Flushed all messages to akonadi";
        m_retrieveItemsRequested = false;
    } else
        kDebug() << "Messages not yet complete... waiting for more...";
}

// ----------------------------------------------------------------------------------

void ImaplibResource::collectionAdded( const Collection & collection, const Collection &parent )
{
    kDebug( ) << "Implement me!";
}

void ImaplibResource::collectionChanged( const Collection & collection )
{
    kDebug( ) << "Implement me!";
}

void ImaplibResource::collectionRemoved( int id, const QString & remoteId )
{
    kDebug( ) << "Implement me!";
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
    int i = KMessageBox::questionYesNoCancel( 0,
            i18n( "The server refused the supplied username and password. "
                  "Do you want to go to the settings, re-enter it for one "
                  "time or do nothing?" ),
            i18n( "Could Not Log In" ),
            KGuiItem( i18n( "Settings" ) ), KGuiItem( i18n( "Single Input" ) ) );
    if ( i == KMessageBox::Yes )
        configure( 0 );
    else if ( i == KMessageBox::No ) {
        manualAuth( connection, m_username );
    } else
        connection->logout();
}

void ImaplibResource::slotAlert( Imaplib*, const QString& message )
{
    KMessageBox::information( 0, i18n( "Server reported: %1",message ) );
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
             SIGNAL( currentFolders( const QStringList& ) ),
             SLOT( slotFolderListReceived( const QStringList& ) ) );

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
             SIGNAL( saveDone() ),
             SIGNAL( saveDone() ) );
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
             SIGNAL( unseenCount( Imaplib*, const QString&, int ) ),
             SLOT( slotUnseenMessagesInMailbox( Imaplib*, const QString& , int ) ) );
    connect( m_imap,
             SIGNAL( mailBoxAdded( const QString& ) ),
             SLOT( slotMailBoxAdded( const QString& ) ) );
    connect( m_imap,
             SIGNAL( mailBoxDeleted( const QString& ) ),
             SLOT( slotMailBoxRemoved( const QString& ) ) );
    connect( m_imap,
             SIGNAL( mailBoxRenamed( const QString&, const QString& ) ),
             SLOT( slotMailBoxRenamed( const QString&, const QString& ) ) );
    connect( m_imap,
             SIGNAL( expungeCompleted( Imaplib*, const QString& ) ),
             SLOT( slotMailBoxExpunged( Imaplib*, const QString& ) ) );
    connect( m_imap,
             SIGNAL( integrity( const QString&, int, const QString&,
                                const QString& ) ),
             SLOT( slotIntegrity( const QString&, int, const QString&,
                                  const QString& ) ) );
    */
}

void ImaplibResource::manualAuth( Imaplib* connection, const QString& username )
{
    KPasswordDialog dlg( 0 /* todo: sane? */ );
    dlg.setPrompt( i18n( "Could not find a valid password, please enter it here" ) );
    if ( dlg.exec() == QDialog::Accepted && !dlg.password().isEmpty() )
        connection->login( username, QString( dlg.password() ) );
    else
        connection->logout();
}

#include "imaplibresource.moc"
