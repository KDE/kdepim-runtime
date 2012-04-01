/*
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

#include <QtDBus/QDBusConnection>
#include <KFileDialog>
#include <KDebug>
#include <KStandardDirs>
#include <KLocale>
#include <KSaveFile>
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
using namespace boost;

KRssLocalResource::KRssLocalResource( const QString &id )
  : ResourceBase( id ), m_syncer( 0 )
{
    new SettingsAdaptor( Settings::self() );
    QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );

    //policy.setCacheTimeout( CACHE_TIMEOUT );
    //policy.setIntervalCheckTime( INTERVAL_CHECK_TIME );

    AttributeFactory::registerAttribute<KRss::FeedPropertiesCollectionAttribute>();

    policy.setInheritFromParent( false );
    policy.setSyncOnDemand( false );
    policy.setLocalParts( QStringList() << KRss::Item::HeadersPart << KRss::Item::ContentPart << Akonadi::Item::FullPayload );


    //changeRecorder()->fetchCollection( true );

    changeRecorder()->itemFetchScope().fetchFullPayload( false );
    //changeRecorder()->itemFetchScope().fetchAllAttributes( true );

    //This timer handles the situation in which at least one collection is changed
    //and the modifications must be written back on the opml file.
    //
    writeBackTimer = new QTimer(this);
    writeBackTimer->setSingleShot( true );
    connect(writeBackTimer, SIGNAL(timeout()), this, SIGNAL(quitOrTimeout()));
    connect(this, SIGNAL(quitOrTimeout()), this, SLOT(fetchCollections()));
}

KRssLocalResource::~KRssLocalResource()
{
}

QString KRssLocalResource::mimeType()
{
    return QLatin1String("application/rss+xml");
}

void KRssLocalResource::retrieveCollections()
{
    const QString path = Settings::self()->path();
    
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
    
    titleOpml = parser.titleOpml();
    QList<shared_ptr<const ParsedNode> > parsedNodes = parser.topLevelNodes();
    
    // create a top-level collection
    Collection top;
    top.setParent( Collection::root() );
    top.setRemoteId( path );
    top.setName( i18n("Local Feeds") );
    top.setContentMimeTypes( QStringList() << Collection::mimeType() << mimeType() );

    top.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( titleOpml );
    //it customizes the root collection with an opml icon
    top.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setIconName( QString("application-opml+xml") );
    //TODO: modify CMakeLists.txt so that it installs the icon
    
    Collection::List list;
    list = buildCollectionTree(parser.topLevelNodes(), list, top); 
      
    collectionsRetrieved( list );
}

Collection::List KRssLocalResource::buildCollectionTree( QList<shared_ptr<const ParsedNode> > listOfNodes, 
				   Collection::List &list, Collection &parent)
{
    list << parent;
  
    foreach(const shared_ptr<const ParsedNode> parsedNode, listOfNodes) {
        if (!parsedNode->isFolder()) {
            Collection c = (static_pointer_cast<const ParsedFeed>(parsedNode))->toAkonadiCollection();
            c.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( parsedNode->title() );
            c.setParent( parent );
            c.setCachePolicy( policy );

            //it customizes the collection with an rss icon
            c.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setIconName( QString("application-rss+xml") );

            list << c;
        } else {
            shared_ptr<const ParsedFolder> parsedFolder = static_pointer_cast<const ParsedFolder>(parsedNode);
            Collection folder;
            folder.setParent( parent );
            folder.setName( i18n("T_%1", parsedFolder->title()) );
                folder.attribute<Akonadi::EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( parsedFolder->title() );
            folder.setRemoteId( Settings::self()->path() + parsedFolder->title() );
            folder.setContentMimeTypes( QStringList() << Collection::mimeType() << mimeType() );
            list = buildCollectionTree( parsedFolder->children(), list, folder );
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
    foreach ( const Syndication::ItemPtr& syndItem, syndItems ) {
        Akonadi::Item item( mimeType() );
        item.setRemoteId( syndItem->id() );
        item.setPayload<KRss::RssItem>( Util::fromSyndicationItem( syndItem ) );
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
    // TODO: any cleanup you need to do while there is still an active
    // event loop. The resource will terminate after this method returns

    if (writeBackTimer->isActive()) {
        writeBackTimer->stop();
        emit quitOrTimeout();
    }
}

void KRssLocalResource::configure( WId windowId )
{
    Q_UNUSED( windowId );

    const QString oldPath = Settings::self()->path();

    KUrl startUrl;
    if ( oldPath.isEmpty() )
        startUrl = KUrl( QDir::homePath() );
    else
        startUrl = KUrl( oldPath );

    const QString title = i18nc("@title:window", "Select an OPML Document");
    QString newPath = KFileDialog::getOpenFileName( startUrl, QLatin1String("*.opml|") + i18n("OPML Document (*.opml)"),
                                              0, title );

    if ( newPath.isEmpty() )
        newPath = KStandardDirs::locateLocal( "appdata", QLatin1String("feeds.opml") );

    Settings::self()->setPath( newPath );
    Settings::self()->writeConfig();
    synchronize();
}

void KRssLocalResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
    Q_UNUSED( collection );

    changeCommitted( item );
}

void KRssLocalResource::itemRemoved( const Akonadi::Item &item )
{  
    changeCommitted( item );
}

void KRssLocalResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts )
{
    Q_UNUSED( parts );

    changeCommitted( item );
}

void KRssLocalResource::collectionChanged(const Akonadi::Collection& collection)
{  
    changeCommitted( collection );

    if (!writeBackTimer->isActive()) {
        writeBackTimer->start(WriteBackTimeout);
    }
}

void KRssLocalResource::fetchCollections()
{
    CollectionFetchJob *job = new CollectionFetchJob( Collection::root(), CollectionFetchJob::Recursive, this );
    job->fetchScope().setContentMimeTypes( QStringList() << mimeType() );
    connect( job, SIGNAL( result( KJob* ) ), SLOT( fetchCollectionsFinished( KJob* ) ) );
}

void KRssLocalResource::fetchCollectionsFinished(KJob *job)
{
    if ( job->error() )
    {
        qDebug() << "Error occurred";
        return;
    }

    CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob*>( job );
    QList<Collection> collections = fetchJob->collections();
    writeFeedsToOpml( Settings::self()->path(), Util::parsedDescendants( collections, Collection::root() ) );
}

void KRssLocalResource::writeFeedsToOpml(const QString &path, const QList<boost::shared_ptr< const ParsedNode> >& nodes)
{
    KSaveFile file( path );
    if ( !file.open( QIODevice::ReadWrite | QIODevice::Text ) ) {
        error( i18n("Could not open %1: %2", path, file.errorString()) );
        return;
    }
  
    QXmlStreamWriter writer( &file );
    writer.setAutoFormatting( true );
    writer.writeStartDocument();
    OpmlWriter::writeOpml( writer, nodes, titleOpml);
    writer.writeEndDocument();
    
    if ( !file.finalize() ) {
        error( i18n("Could not save %1: %2", path, file.errorString()) );
    }
}


AKONADI_RESOURCE_MAIN( KRssLocalResource )

#include "krsslocalresource.moc"
