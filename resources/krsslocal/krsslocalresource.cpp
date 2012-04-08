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
#include "settings.h"
#include "settingsadaptor.h"
#include "util.h"
#include "configdialog.h"


#include <QtDBus/QDBusConnection>
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
#include <KRss/RssItem>

using namespace Akonadi;
using namespace boost;

static const int CacheTimeout = -1, IntervalCheckTime = 5;
static const int WriteBackTimeout = 30000; // in milliseconds

static QString mimeType() {
    return QLatin1String("application/rss+xml");
}

KRssLocalResource::KRssLocalResource( const QString &id )
    : ResourceBase( id )
    , m_writeBackTimer( new QTimer( this ) )
    , m_syncer( 0 )
    , m_quitLoop( 0 )
{
    qsrand(QDateTime::currentDateTime().toTime_t());
    new SettingsAdaptor( Settings::self() );
    if ( Settings::self()->path().isEmpty() )
        Settings::self()->setPath( KGlobal::dirs()->localxdgdatadir() + "/feeds/" + identifier() + QLatin1String("/feeds.opml") );
    QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

    //policy.setCacheTimeout( CACHE_TIMEOUT );
    //policy.setIntervalCheckTime( INTERVAL_CHECK_TIME );

    AttributeFactory::registerAttribute<KRss::FeedPropertiesCollectionAttribute>();

    m_policy.setInheritFromParent( false );
    m_policy.setSyncOnDemand( false );
    m_policy.setLocalParts( QStringList() << KRss::Item::HeadersPart << KRss::Item::ContentPart << Akonadi::Item::FullPayload );

    // Change recording makes the resource unusable for hours here
    // after migrating 130000 items, so disable it, as we don't write back item changes anyway.
    changeRecorder()->setChangeRecordingEnabled( false );
    changeRecorder()->fetchCollection( true );
    changeRecorder()->fetchChangedOnly( true );
    changeRecorder()->setItemFetchScope( ItemFetchScope() );

    //This timer handles the situation in which at least one collection is changed
    //and the modifications must be written back on the opml file.
    m_writeBackTimer->setSingleShot( true );
    m_writeBackTimer->setInterval( WriteBackTimeout );
    connect(m_writeBackTimer, SIGNAL(timeout()), this, SLOT(startOpmlExport()));
}

KRssLocalResource::~KRssLocalResource()
{
}

static bool ensureOpmlCreated( const QString& path, QString* errorString ) {
    Q_ASSERT( errorString );
    errorString->clear();
    if ( QFile::exists( path ) )
        return true;

    KSaveFile out( path );
    if ( !out.open( QIODevice::WriteOnly ) ) {
        *errorString = i18n("Could not create OPML file %1: %2").arg( path, out.errorString() );
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
        *errorString = i18n("Could not finish writing to OPML file %1: %2").arg( path, out.errorString() );
        return false;
    }

    return true;
}

void KRssLocalResource::retrieveCollections()
{
    const QString opmlPath = Settings::self()->path();

    // create a top-level collection
    KRss::FeedCollection top;
    top.setParent( Collection::root() );
    top.setRemoteId( opmlPath );
    top.setIsFolder( true );
    top.setName( i18n("Local Feeds") );
    top.setTitle( i18n("Local Feeds") );
    top.setContentMimeTypes( QStringList() << Collection::mimeType() << mimeType() );
    top.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( m_titleOpml );
    top.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setIconName( QString("application-opml+xml") );
    //TODO: modify CMakeLists.txt so that it installs the icon


    QString errorString;
    if ( !ensureOpmlCreated( opmlPath, &errorString ) ) {
        error( errorString );
        return;
    }

    KRss::ImportFromOpmlJob* job = new KRss::ImportFromOpmlJob( this );
    job->setParentFolder( top );
    job->setInputFile( opmlPath );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(opmlImportFinished(KJob*)) );
    job->start();

}

void KRssLocalResource::opmlImportFinished( KJob* j ) {
    KRss::ImportFromOpmlJob* job = qobject_cast<KRss::ImportFromOpmlJob*>( j );
    Q_ASSERT( job );

    if ( job->error() ) {
        error( job->errorString() );
        return;
    }

    m_titleOpml = job->opmlTitle();

    Collection::List collections = job->collections();
    for ( int i = 0; i < collections.size(); ++i )
        collections[i].setCachePolicy( m_policy );

    collectionsRetrieved( collections );
}

void KRssLocalResource::retrieveItems( const Akonadi::Collection &collection )
{   
    Syndication::Loader * const loader = Syndication::Loader::create();
    connect( loader, SIGNAL( loadingComplete( Syndication::Loader*, Syndication::FeedPtr, Syndication::ErrorCode ) ),
             this, SLOT( slotLoadingComplete( Syndication::Loader*, Syndication::FeedPtr, Syndication::ErrorCode ) ) );
    const KRss::FeedCollection fc( collection );
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

    bool fcChanged = false;

    // if no HTML URL is set, we assume that the feed was previously added by the user (URL only) and we now add the metadata from the fetched feed
    if ( fc.htmlUrl().isEmpty() ) {
        fc.setName( feed->title() );
        fc.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( feed->title() );
        fc.setDescription( feed->description() );
        fc.setHtmlUrl( feed->link() );
        fcChanged = true;
    }

    // clear previous fetch error
    if ( fc.fetchError() ) {
        fc.setFetchError( false );
        fc.setFetchErrorString( QString() );
        fcChanged = true;
    }

    if ( fcChanged ) {
        Akonadi::CollectionModifyJob* job = new Akonadi::CollectionModifyJob( fc );
        job->start();
    }

    QList<Syndication::ItemPtr> syndItems = feed->items();
    Akonadi::Item::List items;
    KDateTime now = KDateTime::currentLocalDateTime();
    foreach ( const Syndication::ItemPtr& syndItem, syndItems ) {
        Akonadi::Item item( mimeType() );
        item.setRemoteId( syndItem->id() );
        item.setPayload<KRss::RssItem>( Util::fromSyndicationItem( syndItem, &now ) );
        items << item;
    }

    //--- a replacement of itemsRetrieved that uses a custom ItemSync---
    if (!m_syncer) {
        m_syncer = new RssItemSync( fc );
        connect( m_syncer, SIGNAL(result(KJob*)), this, SLOT(slotItemSyncDone(KJob*)) );
    }
    m_syncer->setIncrementalSyncItems( items, Item::List() );
}

void KRssLocalResource::slotItemSyncDone( KJob *job )
{
    m_syncer = 0;
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

void KRssLocalResource::aboutToQuit()
{
    // any cleanup you need to do while there is still an active
    // event loop. The resource will terminate after this method returns

    if ( !m_writeBackTimer->isActive() )
        return;
    m_writeBackTimer->stop();
    startOpmlExport();
    QEventLoop loop;
    m_quitLoop = &loop;
    loop.exec();
    m_quitLoop = 0;
}

void KRssLocalResource::configure( WId windowId )
{
    
    QPointer<ConfigDialog> dlg( new ConfigDialog );
    if ( windowId )
      KWindowSystem::setMainWindow( dlg, windowId );
    if ( dlg->exec() == KDialog::Accepted ) {
      emit configurationDialogAccepted();
    } else {
      emit configurationDialogRejected();
    }
    delete dlg;
    Settings::self()->writeConfig();
    synchronizeCollectionTree();
}

void KRssLocalResource::collectionChanged(const Akonadi::Collection& collection)
{  
    changeCommitted( collection );

    if ( !m_writeBackTimer->isActive() )
        m_writeBackTimer->start();
}

void KRssLocalResource::collectionAdded( const Collection &collection, const Collection &parent )
{
    Q_UNUSED( parent )
    changeCommitted( collection );

    if ( !m_writeBackTimer->isActive() )
        m_writeBackTimer->start();
}

void KRssLocalResource::collectionRemoved( const Collection &collection )
{
    changeCommitted( collection );
    if ( !m_writeBackTimer->isActive() )
        m_writeBackTimer->start();
}

void KRssLocalResource::startOpmlExport()
{
    KRss::ExportToOpmlJob* job = new KRss::ExportToOpmlJob( this );
    job->setResource( identifier() );
    job->setOutputFile( Settings::self()->path() );
    job->setIncludeCustomProperties( true );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(opmlExportFinished(KJob*)) );
}

void KRssLocalResource::opmlExportFinished( KJob *job )
{
    if ( job->error() )
        kDebug() << "Error occurred" << job->errorString();

    if ( m_quitLoop )
        m_quitLoop->quit();
}



AKONADI_RESOURCE_MAIN( KRssLocalResource )

#include "krsslocalresource.moc"
