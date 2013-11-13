/*
    KRssLocalResource - an Akonadi resource to store subscriptions in an opml file
    Copyright (C) 2011    Alessandro Cosentino <cosenal@gmail.com>

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

#include "krsslocalresource.h"
#include "rsscollectionattribute.h"
#include "settings.h"
#include "settingsadaptor.h"
#include "util.h"
#include "configdialog.h"


#include <QtDBus/QDBusConnection>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <KWindowSystem>
#include <KDebug>
#include <KLocale>
#include <KDateTime>
#include <KDebug>
#include <KGlobal>
#include <KRandom>
#include <KSaveFile>
#include <KStandardDirs>

#include <QtXml/QXmlStreamReader>
#include <QtXml/QXmlStreamWriter>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/Collection>
#include <Akonadi/CollectionModifyJob>
#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/AttributeFactory>

#include <KRss/ExportToOpmlJob>
#include <KRss/FeedCollection>
#include <KRss/FeedPropertiesCollectionAttribute>
#include <KRss/ImportFromOpmlJob>
#include <KRss/Item>

#include <cerrno>

using namespace Akonadi;
using namespace boost;


KRssLocalResource::KRssLocalResource( const QString &id )
    : ResourceBase( id )
{
    qsrand(QDateTime::currentDateTime().toTime_t());
    new SettingsAdaptor( Settings::self() );
    if ( Settings::self()->path().isEmpty() )
        Settings::self()->setPath( KGlobal::dirs()->localxdgdatadir() + "feeds/" + identifier() + QLatin1String("/feeds.opml") );
    QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

    connect( Settings::self(), SIGNAL(configChanged()), this, SLOT(configChanged()) );

    AttributeFactory::registerAttribute<KRss::FeedPropertiesCollectionAttribute>();
    AttributeFactory::registerAttribute<RssCollectionAttribute>();

    const QStringList localParts = QStringList() << KRss::Item::HeadersPart << KRss::Item::ContentPart << Akonadi::Item::FullPayload;

    m_defaultPolicy.setInheritFromParent( false );
    m_defaultPolicy.setSyncOnDemand( false );
    m_defaultPolicy.setLocalParts( localParts );
    m_defaultPolicy.setIntervalCheckTime( Settings::useIntervalFetch() ? Settings::autoFetchInterval() : -1 );

    // Change recording makes the resource unusable for hours here
    // after migrating 130000 items, so disable it, as we don't write back item changes anyway.
    changeRecorder()->setChangeRecordingEnabled( false );
    changeRecorder()->fetchCollection( true );
    changeRecorder()->fetchChangedOnly( false );
    changeRecorder()->setItemFetchScope( ItemFetchScope() );
}

KRssLocalResource::~KRssLocalResource()
{
}

void KRssLocalResource::configChanged()
{
    const int checkTime = Settings::useIntervalFetch() ? Settings::autoFetchInterval() : -1;
    if ( m_defaultPolicy.intervalCheckTime() == checkTime )
        return;
    m_defaultPolicy.setIntervalCheckTime( checkTime );
    synchronizeCollectionTree();
}

static bool ensureOpmlCreated( const QString& filePath, QString* errorString ) {
    Q_ASSERT( errorString );
    errorString->clear();

    QFileInfo fi( filePath );
    if ( fi.exists() )
        return true;

    QDir dir;
    if ( !dir.exists( fi.absolutePath() ) ) {
        errno = 0;
        if ( !dir.mkpath( fi.absolutePath() ) ) {
            *errorString = i18n("Could not create OPML file %1: Can't create parent folder: %2", filePath, QLatin1String(strerror(errno)));
            return false;
        }
    }

    KSaveFile out( filePath );
    if ( !out.open( QIODevice::WriteOnly ) ) {
        *errorString = i18n("Could not create OPML file %1: %2", filePath, out.errorString() );
        return false;
    }

    QXmlStreamWriter writer( &out );
    writer.writeStartDocument();
    writer.writeStartElement( QLatin1String("opml") );
    writer.writeAttribute( QLatin1String("version"), QLatin1String("1.0") );
    writer.writeStartElement( QLatin1String("head") );
    writer.writeTextElement( QLatin1String("text"), i18n("Feeds") );
    writer.writeEndElement(); // head
    writer.writeStartElement( QLatin1String("body") );
    writer.writeEndElement(); // body
    writer.writeEndElement(); // opml
    writer.writeEndDocument();

    if ( writer.hasError() || !out.finalize() ) {
        *errorString = i18n("Could not finish writing to OPML file %1: %2", filePath, out.errorString() );
        return false;
    }

    return true;
}

void KRssLocalResource::retrieveCollections()
{
    const QString opmlPath = Settings::self()->path();

    QString errorString;
    if ( !ensureOpmlCreated( opmlPath, &errorString ) ) {
        kDebug() << errorString;
        status( Broken, errorString );
        cancelTask( errorString );
        return;
    }

    // create a top-level collection
    KRss::FeedCollection top;
    top.setCachePolicy( m_defaultPolicy );
    top.setParent( Collection::root() );
    top.setRemoteId( opmlPath );
    top.setIsFolder( true );
    top.setName( i18n("Local Feeds") );
    top.setTitle( i18n("Local Feeds") );
    top.setContentMimeTypes( QStringList() << Collection::mimeType() << KRss::Item::mimeType() );
    top.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( m_titleOpml );
    top.setRights( Collection::CanCreateCollection );
    //TODO: modify CMakeLists.txt so that it installs the icon

    KRss::ImportFromOpmlJob* job = new KRss::ImportFromOpmlJob( this );
    job->setCreateCollections( false );
    job->setParentFolder( top );
    job->setInputFile( opmlPath );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(opmlImportFinished(KJob*)) );
    job->start();
}

static CachePolicy applyAutoFetch( const Collection& c, CachePolicy p ) {
    const KRss::FeedCollection fc = c;
    const int customInterval = fc.fetchInterval();
    switch ( customInterval ) {
    case -1: //use default
        break;
    case 0: //fetch never
        p.setIntervalCheckTime( -1 );
        break;
    default:
        p.setIntervalCheckTime( customInterval );
        break;
    }

    return p;
}

void KRssLocalResource::opmlImportFinished( KJob* j ) {
    KRss::ImportFromOpmlJob* job = qobject_cast<KRss::ImportFromOpmlJob*>( j );
    Q_ASSERT( job );

    if ( job->error() ) {
        const QString errorString = job->errorString();
        kDebug() << errorString;
        status( Broken, errorString );
        cancelTask( errorString );
        return;
    }

    m_titleOpml = job->opmlTitle();

    Collection::List collections = job->collections();
    Q_ASSERT( !collections.isEmpty() );
    collections[0].setCachePolicy( m_defaultPolicy );
    for ( int i = 0; i < collections.size(); ++i ) {
        CachePolicy policy = m_defaultPolicy;
        policy = applyAutoFetch( collections[i], policy );
        collections[i].setCachePolicy( policy );
    }
    collectionsRetrieved( collections );
}

void KRssLocalResource::retrieveItems( const Akonadi::Collection &collection )
{
    const KRss::FeedCollection fc( collection );
    if ( fc.isFolder() ) {
        itemsRetrievalDone();
        return;
    }

    Syndication::Loader * const loader = Syndication::Loader::create();
    connect( loader, SIGNAL(loadingComplete(Syndication::Loader*,Syndication::FeedPtr,Syndication::ErrorCode)),
             this, SLOT(slotLoadingComplete(Syndication::Loader*,Syndication::FeedPtr,Syndication::ErrorCode)) );
    const KUrl xmlUrl = fc.xmlUrl();
    m_collectionByLoader.insert( loader, collection );
    loader->loadFrom( xmlUrl );
}

static QString errorCodeToString( Syndication::ErrorCode err )
{
    using namespace Syndication;
    switch ( err )
    {
        case Timeout:
            return i18n( "Timeout on remote server" );
        case UnknownHost:
            return i18n( "Unknown host" );
        case FileNotFound:
            return i18n( "Feed file not found on remote server" );
        case InvalidXml:
            return i18n( "Could not read feed (invalid XML)" );
        case XmlNotAccepted:
            return i18n( "Could not read feed (unknown format)" );
        case InvalidFormat:
            return i18n( "Could not read feed (invalid feed)" );
        case Success:
        case Aborted:
        default:
            break;
    }
    return QString();
}

void KRssLocalResource::slotLoadingComplete(Syndication::Loader* loader, Syndication::FeedPtr feed,
					    Syndication::ErrorCode status)
{
    const Collection c = m_collectionByLoader.take( loader );

    if ( status == Syndication::Aborted ) {
        itemsRetrievalDone();
        return;
    }

    if ( status != Syndication::Success ) {
        KRss::FeedCollection fc( c );
        const QString prevString = fc.fetchErrorString();
        const QString newString = errorCodeToString( status );
        if ( prevString != newString ) {
            fc.setFetchError( true );
            fc.setFetchErrorString( newString );
            Akonadi::CollectionModifyJob* job = new Akonadi::CollectionModifyJob( fc );
            job->start();
        }
        itemsRetrievalDone();
        return;
    }

    KRss::FeedCollection fc( c );

    const Syndication::ImagePtr image = feed->image();
    const QString imageUrl = image ? image->url() : QString();
    const QString imageTitle = image ? image->title() : QString();
    const QString imageLink = image ? image->link() : QString();

    fc.setImageUrl( imageUrl );
    fc.setImageTitle( imageTitle );
    fc.setImageLink( imageLink );

    // if no HTML URL is set, we assume that the feed was previously added by the user (URL only) and we now add the metadata from the fetched feed
    if ( fc.htmlUrl().isEmpty() ) {
        fc.setName( feed->title() );
        fc.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( feed->title() );
        fc.setDescription( feed->description() );
        fc.setHtmlUrl( feed->link() );
    }

    const int fetchCounter = fc.attribute<RssCollectionAttribute>( Entity::AddIfMissing )->fetchCounter() + 1;
    fc.attribute<RssCollectionAttribute>( Entity::AddIfMissing )->setFetchCounter( fetchCounter );
    const QString fetchCounterString = QString::number( fetchCounter );

    // clear previous fetch error
    if ( fc.fetchError() ) {
        fc.setFetchError( false );
        fc.setFetchErrorString( QString() );
    }

    Akonadi::CollectionModifyJob* job = new Akonadi::CollectionModifyJob( fc );
    job->start();

    QList<Syndication::ItemPtr> syndItems = feed->items();
    Akonadi::Item::List items;
    KDateTime now = KDateTime::currentLocalDateTime();
    foreach ( const Syndication::ItemPtr& syndItem, syndItems ) {
        Akonadi::Item item( KRss::Item::mimeType() );
        item.setRemoteId( syndItem->id() );
        item.setPayload<KRss::Item>( Util::fromSyndicationItem( syndItem, &now ) );
        item.setRemoteRevision( fetchCounterString );
        items << item;
    }

    //--- a replacement of itemsRetrieved that uses a custom ItemSync---
    RssItemSync* sync = new RssItemSync( fc );
    connect( sync, SIGNAL(result(KJob*)), this, SLOT(slotItemSyncDone(KJob*)) );
    sync->setIncrementalSyncItems( items, Item::List() );
}

void KRssLocalResource::slotItemSyncDone( KJob *job )
{
    if ( job->error() && job->error() != Job::UserCanceled )
        emit error( job->errorString() );
    itemsRetrievalDone();
}

bool KRssLocalResource::retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
    Q_UNUSED( parts );

    itemRetrieved( item );
    return true;
}


void KRssLocalResource::configure( WId windowId )
{
    Settings::self()->disconnect( this );
    QPointer<ConfigDialog> dlg( new ConfigDialog );
    if ( windowId )
        KWindowSystem::setMainWindow( dlg, windowId );
    if ( dlg->exec() == KDialog::Accepted ) {
        Settings::self()->writeConfig();
        emit configurationDialogAccepted();
        synchronizeCollectionTree();
    } else {
        emit configurationDialogRejected();
    }
    delete dlg;
    connect( Settings::self(), SIGNAL(configChanged()), this, SLOT(configChanged()) );
}

void KRssLocalResource::collectionChanged(const Akonadi::Collection& collection_)
{
    Akonadi::Collection collection = collection_;

    CachePolicy policy = m_defaultPolicy;
    policy = applyAutoFetch( collection, policy );
    collection.setCachePolicy( policy );

    handleChange( collection );
}

void KRssLocalResource::collectionAdded( const Collection &collection_, const Collection &parent )
{
    Q_UNUSED( parent )
    Akonadi::Collection collection = collection_;

    CachePolicy policy = m_defaultPolicy;
    policy = applyAutoFetch( collection, policy );
    collection.setCachePolicy( policy );

    handleChange( collection );
}

void KRssLocalResource::collectionRemoved( const Collection &collection )
{
    handleChange( collection );
}

void KRssLocalResource::handleChange( const Collection& collection ) {
    QString errorString;
    if ( writeOpml( &errorString ) )
        changeCommitted( collection );
    else
        cancelTask( errorString );
}

bool KRssLocalResource::writeOpml( QString* errorString )
{
    KRss::ExportToOpmlJob* job = new KRss::ExportToOpmlJob( this );
    job->setResource( identifier() );
    job->setOutputFile( Settings::self()->path() );
    job->setIncludeCustomProperties( true );
    job->exec(); //TODO is this safe (reentrancy)?
    *errorString = job->errorString();
    return job->error() == KJob::NoError;
}


AKONADI_RESOURCE_MAIN( KRssLocalResource )
