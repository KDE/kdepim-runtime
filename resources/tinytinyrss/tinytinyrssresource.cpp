/*
    Copyright (C) 2013    Frank Osterfeld <osterfeld@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "tinytinyrssresource.h"
#include "settings.h"
#include "settingsadaptor.h"
#include "configdialog.h"
#include "jobs.h"

#include <QtDBus/QDBusConnection>

#include <KDateTime>
#include <KRandom>
#include <KWindowSystem>

#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/Item>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/Collection>
#include <Akonadi/CollectionModifyJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/AttributeFactory>

#include <KWallet/Wallet>

#include <KRss/Enclosure>
#include <KRss/FeedCollection>
#include <KRss/Item>

using namespace Akonadi;
using namespace boost;
using namespace KWallet;


TinyTinyRssResource::TinyTinyRssResource( const QString &id )
    : ResourceBase( id )
    , m_state( LoggedOff )
    , m_wallet( Wallet::openWallet( Wallet::NetworkWallet(), 0, Wallet::Asynchronous ) )
    , m_walletOpenedReceived( false )
    , m_loginRequested( false )
{
    KRss::FeedCollection::registerAttributes();

    QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                                                  Settings::self(),
                                                  QDBusConnection::ExportAdaptors );


    changeRecorder()->itemFetchScope().fetchFullPayload( true );
    changeRecorder()->itemFetchScope().setAncestorRetrieval( ItemFetchScope::All );
    changeRecorder()->fetchCollection( true );
    changeRecorder()->collectionFetchScope().setAncestorRetrieval( CollectionFetchScope::All );

    emit status( NotConfigured, i18n( "Waiting for KWallet..." ) );

    if ( m_wallet ) {
        connect( m_wallet, SIGNAL(walletOpened(bool)), this, SLOT(walletOpened(bool)) );
    } else {
        walletOpened( false );
    }
}

TinyTinyRssResource::~TinyTinyRssResource()
{
    delete m_wallet;
}

static const QString walletFolderName = QLatin1String("Akonadi TinyTiny");

static QString getPassword( Wallet* wallet, const QString& identifier ) {
    if ( !wallet )
        return QString();
    if ( !wallet->hasFolder( walletFolderName ) )
        return QString();

    if ( !wallet->setFolder( walletFolderName ) )
        return QString();

    QString password;
    const int ret = wallet->readPassword( identifier, /*out*/ password );
    if ( ret == 0 )
        return password;
    else
        return QString();
}

static void writePassword( Wallet* wallet, const QString& identifier, const QString& password )
{
    //report errors?
    if ( !wallet )
        return;
    if ( !wallet->isOpen() )
        return;
    if ( !wallet->hasFolder( walletFolderName ) ) {
        if ( !wallet->createFolder( walletFolderName ) )
            return;
    }

    if ( !wallet->setFolder( walletFolderName ) )
        return;

    const int ret = wallet->writePassword( identifier, password );
    if ( ret != 0 )
        return;
}

void TinyTinyRssResource::walletOpened( bool success )
{
    m_walletOpenedReceived = true;

    if ( success ) {
        m_password = getPassword( m_wallet, identifier() );
        if ( m_loginRequested )
            startLogin();
    } else {
        setOnline( false );
        emit status( Broken, i18n("KWallet not available") );
        m_password.clear();
    }

    Q_FOREACH( const WId windowId, m_configDialogsWaitingForWallet )
        reallyConfigure( windowId );
    m_configDialogsWaitingForWallet.clear();
}

bool TinyTinyRssResource::isConfigured() const
{
    return KUrl( Settings::serverUrl() ).isValid();
}

void TinyTinyRssResource::retrieveCollections()
{
    if ( !isConfigured() ) {
        emit status( NotConfigured, i18n("No server URL configured") );
        cancelTask( i18n("No server URL configured") );
        return;
    }
    if ( !isLoggedIn() ) {
        ensureReady();
        deferTask();
        return;
    }

    RetrieveCollectionsJob* job = new RetrieveCollectionsJob( this );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(retrieveCollectionsDone(KJob*)) );
    job->setUrl( Settings::serverUrl() );
    job->setSessionId( m_sessionId );
    job->start();
}

void TinyTinyRssResource::retrieveCollectionsDone( KJob* j )
{
    RetrieveCollectionsJob* job = qobject_cast<RetrieveCollectionsJob*>( j );
    Q_ASSERT( job );
    if ( job->error() != KJob::NoError ) {
        cancelTask( job->errorString() );
        return;
    }

    collectionsRetrieved( job->retrievedCollections() );
}

void TinyTinyRssResource::retrieveItems( const Akonadi::Collection& collection )
{
    if ( !isConfigured() ) {
        emit status( NotConfigured, i18n("No server URL configured") );
        cancelTask( i18n("No server URL configured") );
        return;
    }
    if ( !isLoggedIn() ) {
        ensureReady();
        deferTask();
        return;
    }

    const KRss::FeedCollection fc( collection );
    if ( fc.isFolder() ) {
        itemsRetrieved( Akonadi::Item::List() );
        return;
    }

    const QString feedId = collection.remoteId();

    TransferJob* job = new TransferJob( this );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(retrieveItemsDone(KJob*)) );
    setupJob( job );
    job->setOperation( QLatin1String("getHeadlines") );
    job->setCollection( collection );
    job->insertData( QLatin1String("feed_id"), feedId );
    job->insertData( QLatin1String("show_content"), true );
    job->insertData( QLatin1String("include_attachments"), true );
    job->insertData( QLatin1String("view_mode"), QLatin1String("all_articles") );
    job->start();
}

static void itemFromJson( Akonadi::Item& item, const QVariantMap& map, bool hasContent ) {
    KRss::Item rssItem;
    rssItem.setHeadersLoaded( true );
    rssItem.setContentLoaded( hasContent );
    const QString id = map.value( QLatin1String("id") ).toString();
    const QString title = map.value( QLatin1String("title") ).toString();
    const QString link = map.value( QLatin1String("link") ).toString();
    const bool unread = map.value( QLatin1String("unread") ).toBool();
    const bool marked = map.value( QLatin1String("marked") ).toBool();
    const bool is_updated = map.value( QLatin1String("is_updated") ).toBool();
    const int commentsCount = map.value( QLatin1String("comments_count") ).toInt();
    const QString commentsLink = map.value( QLatin1String("comments_link") ).toString();
    const QString content = map.value( QLatin1String("content") ).toString();
    bool updatedOk;
    const qint64 updated = map.value( QLatin1String("updated") ).toLongLong( &updatedOk );
    rssItem.setTitle( title );
    rssItem.setLink( link );
    rssItem.setContent( content );
    rssItem.setCommentsCount( commentsCount );
    rssItem.setCommentsLink( commentsLink );
    const QVariantList attachments = map.value( QLatin1String("attachments") ).toList();
    QList<KRss::Enclosure> enclosures;
    enclosures.reserve( attachments.size() );
    Q_FOREACH( const QVariant& i, attachments ) {
        const QVariantMap encMap = i.toMap();
        const QString url = encMap.value( QLatin1String("content_url") ).toString();
        const QString type = encMap.value( QLatin1String("content_type") ).toString();
        const QString title = encMap.value( QLatin1String("title") ).toString();
        //no file size (length)?
        bool durationOk;
        const int duration = encMap.value( QLatin1String("duration") ).toString().toInt( &durationOk );
        KRss::Enclosure enclosure;
        enclosure.setTitle( title );
        enclosure.setType( type );
        enclosure.setUrl( url );
        if ( durationOk )
            enclosure.setDuration( duration );
        enclosures << enclosure;
    }
    rssItem.setEnclosures( enclosures );
    //TODO author

    if ( updatedOk ) {
        KDateTime dt;
        dt.setTime_t( updated );
        rssItem.setDateUpdated( dt );
    }

    item.setRemoteId( id );

    KRss::ItemStatus statusFlags;
    if ( unread )
        statusFlags |= KRss::Unread;
    if ( marked )
        statusFlags |= KRss::Important;
    if ( is_updated )
        statusFlags |= KRss::Updated;

    KRss::setItemStatus( item, statusFlags );

    item.setPayload<KRss::Item>( rssItem );
}

void TinyTinyRssResource::retrieveItemsDone( KJob* j )
{
    TransferJob* job = qobject_cast<TransferJob*>( j );
    Q_ASSERT( job );
    if ( job->error() != KJob::NoError ) {
        qDebug() << "error retrieving items" << job->collection().displayName() << j->errorString();
        cancelTask( job->errorString() );
        return;
    }

    const QVariantList itemList = job->responseContent().toList();

    Akonadi::Item::List items;
    items.reserve( itemList.size() );

    Q_FOREACH( const QVariant& jsonItem, itemList ) {
        const QVariantMap map = jsonItem.toMap();
        Akonadi::Item item;
        item.setMimeType( KRss::Item::mimeType() );
        itemFromJson( item, map, /*hasContent=*/true );
        items << item;
    }
    qDebug() << "Retrieved" << job->collection().displayName() << items.count();
    itemsRetrieved( items );
}

bool TinyTinyRssResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
    Q_UNUSED( parts )

    if ( !isConfigured() ) {
        emit status( NotConfigured, i18n("No server URL configured") );
        cancelTask( i18n("No server URL configured") );
        return false;
    }
    if ( !isLoggedIn() ) {
        ensureReady();
        deferTask();
        return false;
    }

    TransferJob* job = new TransferJob( this );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(retrieveSingleItemDone(KJob*)) );
    setupJob( job );
    job->setItem( item );
    job->setOperation( QLatin1String("getArticle") );
    job->insertData( QLatin1String("article_id"), item.remoteId() );
    job->start();
    return true;
}

void TinyTinyRssResource::retrieveSingleItemDone( KJob* j )
{
    TransferJob* job = qobject_cast<TransferJob*>( j );
    Q_ASSERT( job );
    if ( job->error() != KJob::NoError ) {
        cancelTask( job->errorString() );
        return;
    }

    const QVariantList itemList = job->responseContent().toList();
    Q_ASSERT( itemList.count() == 1 );
    const QVariantMap map = itemList[0].toMap();
    Akonadi::Item item = job->item();
    itemFromJson( item, map, /*hasContent=*/true );

    itemRetrieved( item );
}

void TinyTinyRssResource::reallyConfigure(WId windowId)
{
    QPointer<ConfigDialog> dlg( new ConfigDialog( identifier() ) );
    dlg->setPassword( m_password );
    if ( windowId )
        KWindowSystem::setMainWindow( dlg, windowId );
    if ( dlg->exec() == KDialog::Accepted ) {
        m_password = dlg->password();
        writePassword( m_wallet, identifier(), m_password );
        emit configurationDialogAccepted();
    } else {
        emit configurationDialogRejected();
    }
    delete dlg;

    Settings::self()->writeConfig();

    if ( isConfigured() )
        synchronizeCollectionTree();
}

void TinyTinyRssResource::configure( WId windowId )
{
    if ( m_walletOpenedReceived )
        reallyConfigure( windowId );
    else
        m_configDialogsWaitingForWallet.append( windowId );
}

void TinyTinyRssResource::collectionChanged(const Akonadi::Collection& collection)
{
    changeCommitted( collection );
}

void TinyTinyRssResource::feedCreated( KJob * j )
{
    TransferJob* job = qobject_cast<TransferJob*>( j );
    if ( job->error() == KJob::NoError ) {
        changeCommitted( job->collection() );
    } else {
        cancelTask( job->errorString() );
    }
}

void TinyTinyRssResource::folderCreated( KJob * j )
{
    TransferJob* job = qobject_cast<TransferJob*>( j );
    if ( job->error() == KJob::NoError ) {
        changeCommitted( job->collection() );
    } else {
        cancelTask( job->errorString() );
    }
}

void TinyTinyRssResource::collectionDeleted( KJob * j )
{
    TransferJob* job = qobject_cast<TransferJob*>( j );
    if ( job->error() == KJob::NoError ) {
        changeCommitted( job->collection() );
    } else {
        cancelTask( job->errorString() );
    }
}

void TinyTinyRssResource::setupJob( TransferJob * job )
{
    job->setUrl( Settings::serverUrl() );
    job->setSessionId( m_sessionId );
}

void TinyTinyRssResource::collectionAdded( const Collection &collection, const Collection &parent )
{
    if ( !m_walletOpenedReceived ) {
        deferTask();
        return;
    }
}

void TinyTinyRssResource::collectionRemoved( const Collection &collection )
{
    if ( !m_walletOpenedReceived ) {
        deferTask();
        return;
    }

    const KRss::FeedCollection fc ( collection );
}

void TinyTinyRssResource::itemChanged( const Akonadi::Item& item, const QSet<QByteArray>& partIdentifiers )
{
    Q_UNUSED( partIdentifiers )
    if ( !isConfigured() ) {
        emit status( NotConfigured, i18n("No server URL configured") );
        cancelTask( i18n("No server URL configured") );
        return;
    }
    if ( !isLoggedIn() ) {
        ensureReady();
        deferTask();
        return;
    }

    TransferJob* job = new TransferJob( this );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(articlesUpdated(KJob*)) );
    setupJob( job );
    job->setPostDataFormat( TransferJob::EncodedQuery );
    job->setItem( item );
    job->setOperation( QLatin1String("updateArticle") );
    const QStringList itemIds = QStringList() << item.remoteId();
    const bool isUnread = KRss::Item::isUnread( item );
    job->insertData( QLatin1String("article_ids"), itemIds.join( QLatin1String(",") ) );
    job->insertData( QLatin1String("field"), QLatin1String("2") );
    job->insertData( QLatin1String("mode"), isUnread ? QLatin1String("1") : QLatin1String("0") );
    job->start();
}

void TinyTinyRssResource::articlesUpdated( KJob* j )
{
    TransferJob* job = qobject_cast<TransferJob*>( j );
    Q_ASSERT( job );
    if ( job->error() != KJob::NoError ) {
        cancelTask( job->errorString() );
        return;
    }

    changeCommitted( job->item() );
}

void TinyTinyRssResource::loginFinished( KJob* j )
{
    TransferJob* job = qobject_cast<TransferJob*>( j );
    Q_ASSERT( job );

    if ( job->error() != KJob::NoError ) { //TODO handle different codes differently? (e.g. network errors)
        m_state = LoggedOff;
        setOnline( false );
        emit status( Broken, i18n("Login failed: %1", job->errorString()) );
        return;
    }
    const QVariantMap map = job->responseContent().toMap();

    m_sessionId = map.value( QLatin1String("session_id") ).toString();

    bool ok;
    const int apiLevel = map.value( QLatin1String("api_level") ).toInt( &ok );
    if ( !ok ) {
        m_state = LoggedOff;
        setOnline( false );
        emit status( Broken, i18n("Could not parse API Level (%1)", map.value( QLatin1String("api_level") ).toString() ) );
        return;
    }
    if ( apiLevel != 4 ) {
        m_state = LoggedOff;
        setOnline( false );
        emit status( Broken, i18n("Unsupported API level %1 (supported: 4)", QString::number( apiLevel ) ) );
        return;
    }

    qDebug() << "Logged in. sessionId: " << m_sessionId;
    m_state = LoggedIn;
}

bool TinyTinyRssResource::isLoggedIn() const
{
    return m_state == LoggedIn;
}

void TinyTinyRssResource::ensureReady()
{
    if ( isLoggedIn() )
        return;
    if ( m_walletOpenedReceived )
        startLogin();
    else
        m_loginRequested = true;
}

void TinyTinyRssResource::startLogin()
{
    if ( m_state != LoggedOff )
        return;
    m_loginRequested = false;
    m_state = LoggingIn;
    TransferJob* job = new TransferJob( this );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(loginFinished(KJob*)) );
    job->setUrl( Settings::serverUrl() );
    job->setOperation( QLatin1String("login") );
    job->insertData( QLatin1String("user"), Settings::username() );
    job->insertData( QLatin1String("password"), m_password );
    job->start();
}

void TinyTinyRssResource::sendLogout()
{
    m_sessionId.clear();
    m_state = LoggingOff;
    TransferJob* job = new TransferJob( this );
    job->setUrl( Settings::serverUrl() );
    job->setOperation( QLatin1String("logout") );
    job->setSessionId( m_sessionId );
    job->start();
    //Ignoring the result
}

AKONADI_RESOURCE_MAIN( TinyTinyRssResource )

#include "tinytinyrssresource.moc"
