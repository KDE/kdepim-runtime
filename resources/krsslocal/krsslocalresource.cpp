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

#include <krss/rssitem.h>
#include <krss/feedcollection.h>
#include <krss/feedpropertiescollectionattribute.h>

using namespace Akonadi;

static bool writeFeedsToOpml(const QString &path,
                             const QList<boost::shared_ptr< const ParsedNode> >& nodes,
                             const QString& titleOpml,
                             bool withCustomProperties,
                             QString* errorString)
{
    Q_UNUSED(withCustomProperties)
    Q_ASSERT(errorString);
    KSaveFile file( path );
    if ( !file.open( QIODevice::WriteOnly ) ) {
        *errorString = i18n("Could not open %1: %2", path, file.errorString());
        return false;
    }

    QXmlStreamWriter writer( &file );
    writer.setAutoFormatting( true );
    writer.writeStartDocument();
    OpmlWriter::writeOpml( writer, nodes, titleOpml );
    writer.writeEndDocument();

    if ( writer.hasError() || !file.finalize() ) { //hasError() refers to the underlying device, so file.errorString() is our best bet in both cases
        *errorString = i18n("Could not save %1: %2", path, file.errorString() );
        return false;
    }

    return true;
}

class ExportToOpmlJob::Private {
    ExportToOpmlJob* const q;
public:
    explicit Private( ExportToOpmlJob* qq ) : q( qq ), includeCustomProperties( false ) {}

    void doStart();
    void fetchFinished( KJob* job );

    QString resource;
    QString outputFile;
    bool includeCustomProperties;
};

ExportToOpmlJob::ExportToOpmlJob( QObject* parent )
    : d( new Private( this ) )
{}

ExportToOpmlJob::~ExportToOpmlJob() {
    delete d;
}

void ExportToOpmlJob::start()
{
    QMetaObject::invokeMethod( this, "doStart", Qt::QueuedConnection );
}


QString ExportToOpmlJob::resource() const {
    return d->resource;
}

void ExportToOpmlJob::setResource( const QString& identifier ) {
    d->resource = identifier;
}


QString ExportToOpmlJob::outputFile() const {
    return d->outputFile;
}

void ExportToOpmlJob::setOutputFile( const QString& path ) {
    d->outputFile = path;
}

bool ExportToOpmlJob::includeCustomProperties() const {
    return d->includeCustomProperties;
}

void ExportToOpmlJob::setIncludeCustomProperties( bool includeCustomProperties ) {
    d->includeCustomProperties = includeCustomProperties;
}

void ExportToOpmlJob::Private::doStart() {
    CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive, q );
    job->setResource( resource );
    job->fetchScope().setContentMimeTypes( QStringList() << QLatin1String("application/rss+xml") );
    connect( job, SIGNAL( result( KJob* ) ), q, SLOT( fetchFinished( KJob* ) ) );
}

void ExportToOpmlJob::Private::fetchFinished( KJob* j ) {
    CollectionFetchJob* job = qobject_cast<CollectionFetchJob*>( j );
    Q_ASSERT(job);
    if ( job->error() ) {
        q->setErrorText( job->errorText() );
        q->setError( KJob::UserDefinedError );
        q->emitResult();
        return;
    }

    const Collection::List collections = job->collections();
    QString errorString;
    const bool written = writeFeedsToOpml( outputFile,
                                           Util::parsedDescendants( collections, Collection::root() ),
                                           QString(),
                                           includeCustomProperties,
                                           &errorString );
    if ( !written ) {
        q->setErrorText( errorString );
        q->setError( KJob::UserDefinedError );
    }

    q->emitResult();
}

using namespace Akonadi;
using namespace boost;

static const int CacheTimeout = -1, IntervalCheckTime = 5;
static const int WriteBackTimeout = 30000; // in milliseconds

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

QString KRssLocalResource::mimeType()
{
    return QLatin1String("application/rss+xml");
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
    const QString path = Settings::self()->path();

    QString errorString;
    if ( !ensureOpmlCreated( path, &errorString ) ) {
        error( errorString );
        return;
    }

    /* We'll parse the opml file */
    QFile file( path );
    /* If we can't open it, let's show an error message. */
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error(i18n("Could not open %1: %2", path, file.errorString()) );
        return;
    }
    
    QXmlStreamReader reader( &file );
    
    OpmlReader parser;
        
    while ( !reader.atEnd() ) {
        reader.readNext();

        if ( reader.isStartElement() ) {
	    //check if the file is formatted opml, before parsing it
	    //TODO: move this checking to inside the parser.readOpml implementation
            if ( reader.name().toString().toLower() == QLatin1String("opml") ) {
                kDebug() << "OPML version" << reader.attributes().value( QLatin1String("version") ).toString();
                parser.readOpml( reader );
            }
            else {
                reader.raiseError( i18n ( "The file is not a valid OPML document." ) );
            }
        }
    }

    if (reader.hasError()) {
        error(i18n("Could not parse OPML from %1: %2").arg( path, reader.errorString() ) );
        return;

    }
    
    m_titleOpml = parser.titleOpml();
    QList<shared_ptr<const ParsedNode> > parsedNodes = parser.topLevelNodes();
    
    // create a top-level collection
    KRss::FeedCollection top;
    top.setParent( Collection::root() );
    top.setRemoteId( path );
    top.setIsFolder( true );
    top.setName( i18n("Local Feeds") );
    top.setTitle( i18n("Local Feeds") );
    top.setContentMimeTypes( QStringList() << Collection::mimeType() << mimeType() );

    top.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( m_titleOpml );
    //it customizes the root collection with an opml icon
    top.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setIconName( QString("application-opml+xml") );
    //TODO: modify CMakeLists.txt so that it installs the icon
    
    const Collection::List list = buildCollectionTree(parsedNodes, top);
      
    collectionsRetrieved( list );
}

Collection::List KRssLocalResource::buildCollectionTree( const QList<shared_ptr<const ParsedNode> >& listOfNodes, const Collection &parent)
{
    Collection::List list;
    list << parent;
  
    foreach(const shared_ptr<const ParsedNode> parsedNode, listOfNodes) {
        if (!parsedNode->isFolder()) {
            Collection c = (static_pointer_cast<const ParsedFeed>(parsedNode))->toAkonadiCollection();
            c.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( parsedNode->title() );
            c.setParent( parent );
            c.setCachePolicy( m_policy );

            //it customizes the collection with an rss icon
            c.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setIconName( QString("application-rss+xml") );

            list << c;
        } else {
            shared_ptr<const ParsedFolder> parsedFolder = static_pointer_cast<const ParsedFolder>(parsedNode);
            KRss::FeedCollection folder;
            folder.setParent( parent );
            folder.setName( parsedFolder->title() + KRandom::randomString( 8 ) );
            folder.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( parsedFolder->title() );
            folder.setRemoteId( Settings::self()->path() + parsedFolder->title() );
            folder.setIsFolder( true );
            folder.setContentMimeTypes( QStringList() << Collection::mimeType() << mimeType() );
            list << buildCollectionTree( parsedFolder->children(), folder );
        }
    }
  
    return list;
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
    ExportToOpmlJob* job = new ExportToOpmlJob( this );
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
