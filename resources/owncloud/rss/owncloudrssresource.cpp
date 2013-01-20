/*
    Copyright (C) 2012    Frank Osterfeld <osterfeld@kde.org>

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

#include "owncloudrssresource.h"
#include "settings.h"
#include "settingsadaptor.h"
#include "configdialog.h"
#include "jobs.h"

#include <QtDBus/QDBusConnection>

#include <KRandom>
#include <KWindowSystem>

#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/Collection>
#include <Akonadi/CollectionModifyJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/AttributeFactory>

#include <KWallet/Wallet>

#include <KRss/FeedCollection>
#include <KRss/Item>

using namespace Akonadi;
using namespace boost;
using namespace KWallet;

static QString mimeType() {
    return QLatin1String("application/rss+xml");
}

OwncloudRssResource::OwncloudRssResource( const QString &id )
    : ResourceBase( id )
    , m_wallet( Wallet::openWallet( Wallet::NetworkWallet(), 0, Wallet::Asynchronous ) )
    , m_walletOpenedReceived( false )
{
    Q_ASSERT( m_wallet );
    connect( m_wallet, SIGNAL(walletOpened(bool)), this, SLOT(walletOpened(bool)) );
    QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

#if 0
    m_policy.setInheritFromParent( false );
    m_policy.setSyncOnDemand( false );
    m_policy.setLocalParts( QStringList() << KRss::Item::HeadersPart << KRss::Item::ContentPart << Akonadi::Item::FullPayload );

    // Change recording makes the resource unusable for hours here
    // after migrating 130000 items, so disable it, as we don't write back item changes anyway.
    changeRecorder()->setChangeRecordingEnabled( false );
    changeRecorder()->fetchCollection( true );
    changeRecorder()->fetchChangedOnly( true );
    changeRecorder()->setItemFetchScope( ItemFetchScope() );
#endif
}

OwncloudRssResource::~OwncloudRssResource()
{
    delete m_wallet;
}

static const QString walletFolderName = QLatin1String("Akonadi Owncloud");

static QString getPassword( Wallet* wallet, const QString& identifier ) {
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

void OwncloudRssResource::walletOpened( bool success )
{
    m_walletOpenedReceived = true;

    if ( success )
        m_password = getPassword( m_wallet, identifier() );
    else
        m_password.clear();

    Q_FOREACH( const WId windowId, m_configDialogsWaitingForWallet )
        reallyConfigure( windowId );
    m_configDialogsWaitingForWallet.clear();
}

void OwncloudRssResource::foldersListed( KJob* j )
{
    Q_ASSERT( m_listJob == j );
    m_listJob = 0;
    ListNodeJob* job = qobject_cast<ListNodeJob*>( j );
    Q_ASSERT( job );

    if ( job->error() != KJob::NoError ) {
        cancelTask( job->errorString() );
        return;
    }

    m_folders = job->children();

    ListNodeJob* feedsJob = new ListNodeJob( ListNodeJob::Feeds, this );
    feedsJob->setUrl( Settings::owncloudServerUrl() );
    feedsJob->setUsername( Settings::username() );
    feedsJob->setPassword( m_password );
    connect( feedsJob, SIGNAL(result(KJob*)), this, SLOT(feedsListed(KJob*)) );
    m_listJob = feedsJob;
    feedsJob->start();
}

struct IdLessThan {
    bool operator()( const ListNodeJob::Node& lhs, const ListNodeJob::Node& rhs ) const {
        if ( lhs.id != rhs.id )
            return lhs.id < rhs.id;
        if ( lhs.title != rhs.title )
            return lhs.title < rhs.title;
        if ( lhs.parentId != rhs.parentId )
            return lhs.parentId < rhs.parentId;
        if ( lhs.icon != rhs.icon )
            return lhs.icon < rhs.icon;
        return lhs.link < rhs.link;
    }
};

static Collection::List buildCollections( const Collection& top,
                                          QVector<ListNodeJob::Node> folders,
                                          QVector<ListNodeJob::Node> feeds ) {
    qSort( folders.begin(), folders.end(), IdLessThan() );
    qSort( feeds.begin(), feeds.end(), IdLessThan() );

    Collection::List folderCollections, feedCollections;
    folderCollections.reserve( folders.size() );
    feedCollections.reserve( feeds.size() );

    for ( int i = 0; i < folders.size(); ++i ) {
        const ListNodeJob::Node node = folders[i];
        KRss::FeedCollection folder;
        folder.setParentCollection( top );
        folder.setRemoteId( node.id );
        folder.setIsFolder( true );
        folder.setParentRemoteId( node.parentId );
        folder.setContentMimeTypes( QStringList() << Collection::mimeType() << mimeType() );
        folder.setName( node.title + KRandom::randomString( 8 ) );
        folder.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( node.title );
        folderCollections.append( folder );
    }

    for ( int i = 0; i < feeds.size(); ++i ) {
        const ListNodeJob::Node node = feeds[i];
        KRss::FeedCollection feed;
        feed.setParentCollection( top );
        feed.setRemoteId( node.id );
        feed.setParentRemoteId( node.parentId );
        feed.setContentMimeTypes( QStringList() << mimeType() );
        feed.setName( node.title + KRandom::randomString( 8 ) );
        feed.setXmlUrl( node.link );
        feed.setImageUrl( node.icon );
        feed.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( node.title );
        feed.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setIconName( QString("application-rss+xml") );
        feedCollections.append( feed );
    }

    return Collection::List() << top << folderCollections << feedCollections;
}

void OwncloudRssResource::feedsListed( KJob * j ) {
    Q_ASSERT( m_listJob == j );
    m_listJob = 0;
    ListNodeJob* job = qobject_cast<ListNodeJob*>( j );
    Q_ASSERT( job );

    if ( job->error() != KJob::NoError ) {
        cancelTask( job->errorString() );
        return;
    }

    const QVector<ListNodeJob::Node> feeds = job->children();

    Collection::List collections;

    // create a top-level collection
    Collection top;
    top.setParent( Collection::root() );
    top.setRemoteId( QLatin1String("0") );
    top.setContentMimeTypes( QStringList() << Collection::mimeType() << mimeType() );
    top.setName( i18n("Owncloud Feeds") );
    top.setRights( Collection::CanCreateCollection );
    top.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( i18n("Owncloud News") );
    collections << buildCollections( top, m_folders, feeds );
    collectionsRetrieved( collections );
}

void OwncloudRssResource::retrieveCollections()
{
    Q_ASSERT( !m_listJob );
    m_folders.clear();

    if ( !m_walletOpenedReceived ) {
        deferTask();
        return;
    }

    ListNodeJob* job = new ListNodeJob( ListNodeJob::Folders, this );
    job->setUrl( Settings::owncloudServerUrl() );
    job->setUsername( Settings::username() );
    job->setPassword( m_password );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(foldersListed(KJob*)) );
    m_listJob = job;
    job->start();
}


void OwncloudRssResource::retrieveItems( const Akonadi::Collection &collection )
{
    itemsRetrieved( Akonadi::Item::List() );
}


bool OwncloudRssResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
    Q_UNUSED( parts );

    itemRetrieved( item );
    return true;
}

void OwncloudRssResource::reallyConfigure(WId windowId)
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

    synchronizeCollectionTree();
}

void OwncloudRssResource::configure( WId windowId )
{
    if ( m_walletOpenedReceived )
        reallyConfigure( windowId );
    else
        m_configDialogsWaitingForWallet.append( windowId );

}

void OwncloudRssResource::collectionChanged(const Akonadi::Collection& collection)
{
    changeCommitted( collection );
}

void OwncloudRssResource::collectionAdded( const Collection &collection, const Collection &parent )
{
    Q_UNUSED( parent )
    changeCommitted( collection );
}

void OwncloudRssResource::collectionRemoved( const Collection &collection )
{
    changeCommitted( collection );
}


AKONADI_RESOURCE_MAIN( OwncloudRssResource )

#include "owncloudrssresource.moc"
