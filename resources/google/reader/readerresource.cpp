/*
    Copyright (C) 2013  Daniel Vrátil <dvratil@redhat.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "readerresource.h"
#include "common/googlesettingsdialog.h"
#include "settingsdialog.h"
#include "settings.h"
#include "changeditemscache.h"

#include <QtCore/QTimer>
#include <QtCore/QVarLengthArray>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusPendingReply>

#include <Akonadi/ChangeRecorder>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/CollectionModifyJob>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/CollectionFetchJob>

#include <KRss/Item>
#include <KRss/FeedCollection>

#include <KDE/KLocalizedString>
#include <KDE/KDateTime>
#include <KDE/KUrl>
#include <KDE/KStandardDirs>

#include <LibKGAPI2/Account>
#include <LibKGAPI2/Reader/ReaderService>
#include <LibKGAPI2/Reader/EditTokenFetchJob>
#include <LibKGAPI2/Reader/ItemFetchJob>
#include <LibKGAPI2/Reader/StreamFetchJob>
#include <LibKGAPI2/Reader/StreamMoveJob>
#include <LibKGAPI2/Reader/StreamRenameJob>
#include <LibKGAPI2/Reader/StreamSubscribeJob>
#include <LibKGAPI2/Reader/StreamUnsubscribeJob>
#include <LibKGAPI2/Reader/Stream>
#include <LibKGAPI2/Reader/Item>
#include <LibKGAPI2/Object>

using namespace Akonadi;
using namespace KGAPI2;

Q_DECLARE_METATYPE(KRss::FeedCollection)

ReaderResource::ReaderResource( const QString& id ):
    GoogleResource( id )
{
    setNeedsNetwork( true );
    setOnline( true );

    changeRecorder()->itemFetchScope().fetchFullPayload();

    m_cache = new ChangedItemsCache( id, this );
    m_cacheTimer = new QTimer( this );
    connect( m_cacheTimer, SIGNAL(timeout()),
             this, SLOT(slotCacheTimeout()) );

    m_favicons = new QDBusInterface( "org.kde.kded", "/modules/favicons", "org.kde.FavIcon" );
    connect( m_favicons, SIGNAL(iconChanged(bool,QString,QString)),
             this, SLOT(iconChanged(bool,QString,QString)) );
}

ReaderResource::~ReaderResource()
{
}

int ReaderResource::runConfigurationDialog(WId windowId)
{
   QScopedPointer<SettingsDialog> settingsDialog( new SettingsDialog( accountManager(), windowId, this ) );
   settingsDialog->setWindowIcon( KIcon( "im-google" ) );

   return settingsDialog->exec();
}

QList< QUrl > ReaderResource::scopes() const
{
    QList<QUrl> scopes;
    scopes << Reader::ReaderService::scopeUrl();
    return scopes;
}

GoogleSettings* ReaderResource::settings() const
{
    return Settings::self();
}

void ReaderResource::updateResourceName()
{
    const QString accountName = Settings::self()->account();
    setName( i18nc( "%1 is account name (user@gmail.com)", "Google Reader (%1)", accountName.isEmpty() ? i18n( "not configured" ) : accountName ) );
}

bool ReaderResource::canPerformTask(bool needsToken)
{
    if ( needsToken && m_editToken.isEmpty() ) {
        return false;
    }

    return GoogleResource::canPerformTask();
}

void ReaderResource::retrieveItems(const Collection& collection)
{
    if ( !canPerformTask() ) {
        return;
    }

    Reader::ItemFetchJob *fetchJob = new Reader::ItemFetchJob( collection.remoteId(), account(), this );
    fetchJob->setProperty( COLLECTION_PROPERTY, QVariant::fromValue( collection ) );
    connect( fetchJob, SIGNAL(progress(KGAPI2::Job*,int,int)),
             this, SLOT(emitPercent(KGAPI2::Job*,int,int)) );
    connect( fetchJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotItemsRetrieved(KGAPI2::Job*)) );
}

bool ReaderResource::retrieveItem( const Akonadi::Item& item, const QSet< QByteArray >& parts )
{
  Q_UNUSED( parts );

  itemRetrieved( item );

  return true;
}

void ReaderResource::retrieveCollections()
{
    if ( !canPerformTask() ) {
        return;
    }

    Reader::StreamFetchJob *fetchJob = new Reader::StreamFetchJob( account(), this );
    connect( fetchJob, SIGNAL(progress(KGAPI2::Job*,int,int)),
             this, SLOT(emitPercent(KGAPI2::Job*,int,int)) );
    connect( fetchJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotCollectionsRetrieved(KGAPI2::Job*)) );
}

void ReaderResource::itemChanged( const Akonadi::Item& item, const QSet< QByteArray >& partIdentifiers )
{
    Q_UNUSED ( partIdentifiers );

    /* Store the item in out internal cache. We don't want to spam Google with
     * zillions of HTTP requests when user clicks 'Mark all as read'. Instead
     * we accumulate the events and then we send one big request to change tags
     * of all the items. */
    m_cache->addItem( item );

    /* Restart the cache timer. If no new itemChanged() signal comes within 5
     * seconds, submit whatever's in the cache. */
    m_cacheTimer->start( 5000 );

    /* Unfortunately in order to get next change information, we have to lie
     * and claim that we have successfuly stored the change on Google servers,
     * which is not true. */
    changeCommitted( item );
}

void ReaderResource::collectionAdded( const Akonadi::Collection& collection, const Akonadi::Collection& parent )
{
    if ( !canPerformTask( true ) ) {
        return;
    }

    KRss::FeedCollection feedCollection = collection;

    /* An empty folder is not stored on server */ 
    if ( feedCollection.isFolder() ) {
        changeCommitted( collection );
        return;
    }

    Reader::StreamSubscribeJob *subscribeJob =
        new Reader::StreamSubscribeJob( feedCollection.url(), m_editToken,
                                        parent.remoteId(), account(), this );
    subscribeJob->setProperty( COLLECTION_PROPERTY, QVariant::fromValue( collection ) );
    connect( subscribeJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotGenericJobFinished(KGAPI2::Job*)) );
}

void ReaderResource::collectionChanged( const Akonadi::Collection& collection, const QSet< QByteArray >& changedAttributes )
{
    if ( !canPerformTask( true ) ) {
        return;
    }

    KRss::FeedCollection feedCollection = collection;
    if ( feedCollection.isFolder() ) {
        // Fetch all child collections and change their category
        CollectionFetchJob *fetchJob = new CollectionFetchJob( collection, CollectionFetchJob::FirstLevel );
        fetchJob->setProperty( COLLECTION_PROPERTY, QVariant::fromValue( collection ) );
        connect( fetchJob, SIGNAL(finished(KJob*)),
                 this, SLOT(slotRenameFolderFetchDone(KJob*)) );
        return;
    }

    QString name;
    if ( collection.hasAttribute<EntityDisplayAttribute>() ) {
        EntityDisplayAttribute *attr = collection.attribute<EntityDisplayAttribute>();
        name = attr->displayName();
    } else {
        name = feedCollection.title();
    }

    Reader::StreamRenameJob *renameJob = 
        new Reader::StreamRenameJob(collection.remoteId(), name, m_editToken,
                                    account(), this);
    renameJob->setProperty( COLLECTION_PROPERTY, QVariant::fromValue( collection ) );
    connect( renameJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotGenericJobFinished(KGAPI2::Job*)) );
}

void ReaderResource::collectionMoved( const Akonadi::Collection& collection, const Akonadi::Collection& collectionSource, const Akonadi::Collection& collectionDestination )
{
    if ( !canPerformTask( true ) ) {
        return;
    }

    Reader::StreamMoveJob *moveJob =
        new Reader::StreamMoveJob( collection.remoteId(), collectionSource.remoteId(),
                                   collectionDestination.remoteId(), m_editToken,
                                   account(), this );
    moveJob->setProperty( COLLECTION_PROPERTY, QVariant::fromValue( collection ) );
    connect( moveJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotGenericJobFinished(KGAPI2::Job*)) );
}

void ReaderResource::collectionRemoved( const Akonadi::Collection& collection )
{
    if ( !canPerformTask( true ) ) {
        return;
    }

    KRss::FeedCollection feedCollection;

    if ( feedCollection.isFolder() ) {
        changeCommitted( collection );
        return;
    }

    Reader::StreamUnsubscribeJob *unsubscribeJob =
        new Reader::StreamUnsubscribeJob( collection.remoteId(), m_editToken,
                                          account(), this );
    unsubscribeJob->setProperty( COLLECTION_PROPERTY, QVariant::fromValue( collection ) );
    connect( unsubscribeJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotGenericJobFinished(KGAPI2::Job*)) );
}

void ReaderResource::fetchEditToken()
{
    if ( !canPerformTask() ) {
        return;
    }

    Reader::EditTokenFetchJob *fetchJob = new Reader::EditTokenFetchJob( account(), this );
    connect( fetchJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotEditTokenRetrieved(KGAPI2::Job*)) );
}

void ReaderResource::slotEditTokenRetrieved( KGAPI2::Job* job )
{
    Reader::EditTokenFetchJob *fetchJob = qobject_cast<Reader::EditTokenFetchJob*>( job );
    m_editToken = fetchJob->editToken();
}

void ReaderResource::slotCollectionsRetrieved( KGAPI2::Job *job )
{
    if ( !handleError( job ) ) {
        return;
    }

    Reader::StreamFetchJob *fetchJob = qobject_cast<Reader::StreamFetchJob*>( job );
    ObjectsList objects = fetchJob->items();

    m_rootCollection = Collection();
    m_rootCollection.setContentMimeTypes( QStringList() << Collection::mimeType() );
    m_rootCollection.setRemoteId( account()->accountName() );
    m_rootCollection.setName( account()->accountName() );
    m_rootCollection.setParent( Collection::root() );
    m_rootCollection.setRights( Collection::CanCreateCollection );

    EntityDisplayAttribute *attr = m_rootCollection.attribute<EntityDisplayAttribute>( Entity::AddIfMissing );
    attr->setDisplayName( account()->accountName() );
    attr->setIconName( QLatin1String( "im-google" ) );

    m_collections[ account()->accountName() ] = m_rootCollection;

    while ( !objects.isEmpty() ) {

        Reader::StreamPtr stream = objects.first().dynamicCast<Reader::Stream>();

        KRss::FeedCollection collection;
        collection.setRemoteId( stream->id() );
        collection.setName( stream->title() );
        collection.setRights( Collection::CanChangeItem |
                              Collection::CanChangeCollection |
                              Collection::CanDeleteCollection );
        collection.setContentMimeTypes( QStringList() << KRss::Item::mimeType() );

        attr = collection.attribute<EntityDisplayAttribute>( Entity::AddIfMissing );
        attr->setDisplayName( stream->title() );

        if ( stream->categories().isEmpty() ) {
            collection.setParent( m_rootCollection );
        } else {
            const QString category = stream->categories().keys().first();
            if ( m_collections.contains( category ) ) {
                collection.setParent( m_collections[ category ] );
            } else {
                KRss::FeedCollection parent;
                parent.setRemoteId( category );
                parent.setName( stream->categories().value( category ) );
                parent.setRights( Collection::CanCreateCollection |
                                  Collection::CanChangeCollection |
                                  Collection::CanDeleteCollection );
                parent.setContentMimeTypes( QStringList() << Collection::mimeType() );
                parent.setParent( m_rootCollection );
                parent.setIsFolder( true );

                attr = parent.attribute<EntityDisplayAttribute>( Entity::AddIfMissing );
                attr->setDisplayName( stream->categories().value( category ) );

                m_collections[ parent.remoteId() ] = parent;

                collection.setParent( parent );
            }
        }

        m_collections[ collection.remoteId() ] = collection;

        objects.removeFirst();
    }

    collectionsRetrieved( m_collections.values() );


    Q_FOREACH( const Collection &collection, m_collections ) {
        KRss::FeedCollection feedCollection( collection );
        if ( feedCollection.isFolder() ) {
            kDebug() << "Skipping favicon for" << feedCollection.id();
            continue;
        }

        kDebug() << "Requesting favicon fetch for" << feedCollection.id();
        fetchFavicon( feedCollection );
    }
}

void ReaderResource::slotItemsRetrieved( KGAPI2::Job *job )
{
    if ( !handleError( job ) ) {
        return;
    }

    Collection collection = job->property( COLLECTION_PROPERTY ).value<Collection>();
    Item::List items;

    Reader::ItemFetchJob *fetchJob = qobject_cast<Reader::ItemFetchJob*>( job );
    const ObjectsList objects = fetchJob->items();

    Q_FOREACH( const ObjectPtr &obj, objects ) {
        Reader::ItemPtr rdItem = obj.dynamicCast<Reader::Item>();
        Akonadi::Item akItem;

        akItem.setRemoteId( rdItem->id() );
        akItem.setRemoteRevision( rdItem->etag());
        akItem.setMimeType( KRss::Item::mimeType() );
        akItem.setPayload<KRss::Item>( KRss::Item( *rdItem.dynamicCast<KRss::Item>() ) );

        items << akItem;
    }

    itemsRetrievedIncremental( items, Item::List() );

    collection.setRemoteRevision( QString::number( KDateTime::currentUtcDateTime().toTime_t() ) );
    new CollectionModifyJob( collection, this );
}

void ReaderResource::slotRenameFolderFetchDone( KJob *job )
{
    CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob*>( job );
    Collection folder = fetchJob->property( COLLECTION_PROPERTY ).value<Collection>();
    const Collection::List collections = fetchJob->collections();

    const QString newFolderRemoteId = QLatin1String("user/-/label/") + folder.name();
    Q_FOREACH( const Collection &collection, collections ) {
        Reader::StreamMoveJob( collection.remoteId(), folder.remoteId(),
                               newFolderRemoteId, m_editToken, account(),
                               this );
    }

    // Modify the folder collection so that remoteId matches the new label
    folder.setRemoteId(newFolderRemoteId);
    new CollectionModifyJob( folder );
}

void ReaderResource::slotCacheTimeout()
{

}

void ReaderResource::fetchFavicon( KRss::FeedCollection& collection )
{
    QString url = collection.remoteId();
    url = url.mid( 5 ); // remove the "feed/" prefix from URL

    // an evil blocking dbus call. Should not take long though and does not
    // impact any user visible part, so I gues it's fine....
    QDBusReply<QString> reply =  m_favicons->call( "iconForUrl", url );

    kDebug() << reply.isValid();
    if ( !reply.isValid() ) {
        return;
    }

    const QString iconName = reply;
    kDebug() << collection.url().url() << iconName;
    if ( iconName.isEmpty() ) {
        const QString host = QUrl( url ).host();
        m_pendingIcons.insertMulti( host, collection );
        m_favicons->call( "downloadHostIcon", url );
        return;
    }

    const QString iconFile = KGlobal::dirs()->findResource( "cache", iconName + ".png" );
    kDebug() << iconFile;
    EntityDisplayAttribute *attr = collection.attribute<EntityDisplayAttribute>( Entity::AddIfMissing );
    attr->setIconName( iconFile );

    new CollectionModifyJob( collection, this );
}

void ReaderResource::iconChanged(bool success, const QString& host, const QString& iconName)
{
    kDebug() << host << iconName;
    if ( !success || !m_pendingIcons.contains( host ) ) {
        // remove only one ocurrance of host, there could be more
        m_pendingIcons.take( host );
        return;
    }

    Collection collection = m_pendingIcons.value( host );
    m_pendingIcons.remove( host );

    const QString iconFile = KGlobal::dirs()->findResource( "cache", iconName + ".png" );
    EntityDisplayAttribute *attr = collection.attribute<EntityDisplayAttribute>( Entity::AddIfMissing );
    attr->setIconName( iconFile );

    new CollectionModifyJob( collection, this );

}


AKONADI_RESOURCE_MAIN( ReaderResource )
