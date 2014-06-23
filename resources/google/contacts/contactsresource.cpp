/*
    Copyright (C) 2011-2013  Daniel Vrátil <dvratil@redhat.com>

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

#include "contactsresource.h"
#include "settingsdialog.h"
#include "settings.h"

#include <Akonadi/CollectionModifyJob>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/LinkJob>
#include <Akonadi/UnlinkJob>
#include <Akonadi/CachePolicy>

#include <KABC/Addressee>
#include <KABC/Picture>

#include <KDE/KLocalizedString>
#include <KDE/KDebug>

#include <QtCore/QPointer>

#include <LibKGAPI2/Types>
#include <LibKGAPI2/Account>
#include <LibKGAPI2/AuthJob>
#include <LibKGAPI2/Contacts/Contact>
#include <LibKGAPI2/Contacts/ContactCreateJob>
#include <LibKGAPI2/Contacts/ContactDeleteJob>
#include <LibKGAPI2/Contacts/ContactFetchJob>
#include <LibKGAPI2/Contacts/ContactFetchPhotoJob>
#include <LibKGAPI2/Contacts/ContactModifyJob>
#include <LibKGAPI2/Contacts/ContactsGroup>
#include <LibKGAPI2/Contacts/ContactsGroupCreateJob>
#include <LibKGAPI2/Contacts/ContactsGroupDeleteJob>
#include <LibKGAPI2/Contacts/ContactsGroupFetchJob>
#include <LibKGAPI2/Contacts/ContactsGroupModifyJob>

#define MYCONTACTS_REMOTEID QLatin1String( "MyContacts" )
#define OTHERCONTACTS_REMOTEID QLatin1String( "OtherContacts" )


Q_DECLARE_METATYPE( KGAPI2::Job * )
Q_DECLARE_METATYPE( QList<QString> )

using namespace Akonadi;
using namespace KGAPI2;

ContactsResource::ContactsResource( const QString &id ):
    GoogleResource( id )
{
    updateResourceName();
}

ContactsResource::~ContactsResource()
{
}

GoogleSettings *ContactsResource::settings() const
{
    return Settings::self();
}

int ContactsResource::runConfigurationDialog( WId windowId )
{

   QScopedPointer<SettingsDialog> settingsDialog( new SettingsDialog( accountManager(), windowId, this ) );
   settingsDialog->setWindowIcon( KIcon( QLatin1String("im-google") ) );

   return settingsDialog->exec();
}

void ContactsResource::updateResourceName()
{
    const QString accountName = Settings::self()->account();
    setName( i18nc( "%1 is account name (user@gmail.com)", "Google Contacts (%1)", accountName.isEmpty() ? i18n( "not configured" ) : accountName ) );
}

QList< QUrl > ContactsResource::scopes() const
{
    QList< QUrl > scopes;
    scopes << Account::contactsScopeUrl()
           << Account::accountInfoScopeUrl();
    return scopes;
}

void ContactsResource::retrieveItems( const Collection &collection )
{
    if ( !canPerformTask() ) {
        return;
    }

    // All items are only in top-level collection and Other Contacts collection
    if ( ( collection.remoteId() != m_rootCollection.remoteId() ) &&
         ( collection.remoteId() != OTHERCONTACTS_REMOTEID ) ) {
        itemsRetrievalDone();
        return;
    }

    ContactFetchJob *fetchJob = new ContactFetchJob( account(), this );
    fetchJob->setProperty( COLLECTION_PROPERTY, QVariant::fromValue( collection ) );
    fetchJob->setFetchDeleted( true );
    if ( !collection.remoteRevision().isEmpty() ) {
        fetchJob->setFetchOnlyUpdated( collection.remoteRevision().toLongLong() );
    }
    connect( fetchJob, SIGNAL(progress(KGAPI2::Job*,int,int)),
             this, SLOT(emitPercent(KGAPI2::Job*,int,int)) );
    connect( fetchJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotItemsRetrieved(KGAPI2::Job*)) );
}

void ContactsResource::retrieveContactsPhotos( const QVariant &arguments )
{
    if ( !canPerformTask() ) {
        return;
    }

    const QVariantMap map = arguments.toMap();
    const Collection collection = map[ QLatin1String("collection") ].value<Collection>();
    ItemFetchJob *itemFetchJob = new ItemFetchJob( collection, this );
    itemFetchJob->setProperty( "modifiedItems", map[ QLatin1String("modifiedItems") ] );
    itemFetchJob->fetchScope().fetchFullPayload(true);
    connect( itemFetchJob, SIGNAL(finished(KJob*)),
             this, SLOT(slotUpdatePhotosItemsRetrieved(KJob*)) );
    emit status( Running, i18nc( "@info:status", "Retrieving photos" ) );
}

void ContactsResource::retrieveCollections()
{
    if ( !canPerformTask() ) {
        return;
    }

    ContactsGroupFetchJob *fetchJob = new ContactsGroupFetchJob( account(), this );
    connect( fetchJob, SIGNAL(progress(KGAPI2::Job*,int,int)),
             this, SLOT(emitPercent(KGAPI2::Job*,int,int)) );
    connect( fetchJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotCollectionsRetrieved(KGAPI2::Job*)) );
}

void ContactsResource::itemAdded( const Item &item, const Collection &collection )
{
    if ( !canPerformTask<KABC::Addressee>( item, KABC::Addressee::mimeType() ) ) {
        return;
    }

    KABC::Addressee addressee = item.payload< KABC::Addressee >();
    ContactPtr contact( new Contact( addressee ) );

    /* If the contact has been moved into My Contacts group then modify the membership */
    if ( collection.remoteId() == MYCONTACTS_REMOTEID ) {
        contact->addGroup( QString::fromLatin1( "http://www.google.com/m8/feeds/groups/%1/base/6" ).arg( QString::fromLatin1( QUrl::toPercentEncoding( account()->accountName() ) ) ) );
    }

    /* If the contact has been moved to Other Contacts then remove all groups */
    if ( collection.remoteId() == OTHERCONTACTS_REMOTEID ) {
        contact->clearGroups();
    }

    ContactCreateJob *createJob = new ContactCreateJob( contact, account(), this );
    createJob->setProperty( ITEM_PROPERTY, QVariant::fromValue( item ) );
    connect( createJob, SIGNAL(progress(KGAPI2::Job*,int,int)),
             this, SLOT(emitPercent(KGAPI2::Job*,int,int)) );
    connect( createJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotCreateJobFinished(KGAPI2::Job*)) );
}

void ContactsResource::itemChanged( const Item &item, const QSet< QByteArray > &partIdentifiers )
{
    Q_UNUSED( partIdentifiers );

    if ( !canPerformTask<KABC::Addressee>( item, KABC::Addressee::mimeType() ) ) {
        return;
    }

    KABC::Addressee addressee = item.payload< KABC::Addressee >();
    ContactPtr contact( new Contact( addressee ) );

    if ( item.parentCollection().remoteId() == MYCONTACTS_REMOTEID ) {
        contact->addGroup( QString::fromLatin1( "http://www.google.com/m8/feeds/groups/%1/base/6" ).arg( QString::fromLatin1( QUrl::toPercentEncoding( account()->accountName() ) ) ) );
    }

    ContactModifyJob *modifyJob = new ContactModifyJob( contact, account(), this );
    modifyJob->setProperty( ITEM_PROPERTY, QVariant::fromValue( item ) );
    connect( modifyJob, SIGNAL(progress(KGAPI2::Job*,int,int)),
             this, SLOT(emitPercent(KGAPI2::Job*,int,int)) );
    connect( modifyJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotGenericJobFinished(KGAPI2::Job*)) );
}

void ContactsResource::itemMoved( const Item &item, const Collection &collectionSource,
                                  const Collection &collectionDestination )
{
    if ( !canPerformTask<KABC::Addressee>( item, KABC::Addressee::mimeType() ) ) {
        return;
    }

    KABC::Addressee addressee = item.payload< KABC::Addressee >();
    ContactPtr contact( new Contact( addressee ) );

    // MyContacts -> OtherContacts
    if ( collectionSource.remoteId() == MYCONTACTS_REMOTEID &&
            collectionDestination.remoteId() == OTHERCONTACTS_REMOTEID ) {
        contact->removeGroup( QString::fromLatin1( "http://www.google.com/m8/feeds/groups/%1/base/6" ).arg( QString::fromLatin1( QUrl::toPercentEncoding( account()->accountName() ) ) ) );

        // OtherContacts -> MyContacts
    } else if ( collectionSource.remoteId() == OTHERCONTACTS_REMOTEID &&
                collectionDestination.remoteId() == MYCONTACTS_REMOTEID ) {
        contact->addGroup( QString::fromLatin1( "http://www.google.com/m8/feeds/groups/%1/base/6" ).arg( QString::fromLatin1( QUrl::toPercentEncoding( account()->accountName() ) ) ) );

    } else {
        cancelTask( i18n( "Invalid source or destination collection" ) );
        return;
    }

    ContactModifyJob *modifyJob = new ContactModifyJob( contact, account(), this );
    modifyJob->setProperty( ITEM_PROPERTY, QVariant::fromValue( item ) );
    connect( modifyJob, SIGNAL(progress(KGAPI2::Job*,int,int)),
             this, SLOT(emitPercent(KGAPI2::Job*,int,int)) );
    connect( modifyJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotGenericJobFinished(KGAPI2::Job*)) );
}

void ContactsResource::itemRemoved( const Item &item )
{
    if ( !canPerformTask() ) {
        return;
    }

    ContactDeleteJob *deleteJob = new ContactDeleteJob( item.remoteId(), account(), this );
    deleteJob->setProperty( ITEM_PROPERTY, QVariant::fromValue( item ) );
    connect( deleteJob, SIGNAL(progress(KGAPI2::Job*,int,int)),
             this, SLOT(emitPercent(KGAPI2::Job*,int,int)) );
    connect( deleteJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotGenericJobFinished(KGAPI2::Job*)) );

    emit status( Running, i18nc( "@info:status", "Removing contact" ) );
}

void ContactsResource::itemLinked( const Item &item, const Collection &collection )
{
    if ( !canPerformTask<KABC::Addressee>( item, KABC::Addressee::mimeType() ) ) {
        return;
    }

    KABC::Addressee addressee = item.payload<KABC::Addressee>();
    ContactPtr contact( new Contact( addressee ) );

    contact->addGroup( collection.remoteId() );

    ContactModifyJob *modifyJob = new ContactModifyJob( contact, account(), this );
    connect( modifyJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotGenericJobFinished(KGAPI2::Job*)) );
}

void ContactsResource::itemUnlinked( const Item &item, const Collection &collection )
{
    if ( !canPerformTask<KABC::Addressee>( item, KABC::Addressee::mimeType() ) ) {
        return;
    }

    KABC::Addressee addressee = item.payload<KABC::Addressee>();
    ContactPtr contact( new Contact( addressee ) );

    contact->removeGroup( collection.remoteId() );

    ContactModifyJob *modifyJob = new ContactModifyJob( contact, account(), this );
    modifyJob->setProperty( ITEM_PROPERTY, QVariant::fromValue( item ) );
    connect( modifyJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotGenericJobFinished(KGAPI2::Job*)) );
}

void ContactsResource::collectionAdded( const Akonadi::Collection &collection,
                                        const Akonadi::Collection &parent )
{
    Q_UNUSED( parent );

    if ( !canPerformTask() ) {
        return;
    }

    ContactsGroupPtr group( new ContactsGroup );
    group->setTitle( collection.name() );
    group->setIsSystemGroup( false );

    ContactsGroupCreateJob *createJob = new ContactsGroupCreateJob( group, account(), this );
    createJob->setProperty( COLLECTION_PROPERTY, QVariant::fromValue( collection ) );
    connect( createJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotCreateJobFinished(KGAPI2::Job*)) );
}

void ContactsResource::collectionChanged( const Akonadi::Collection &collection )
{
    if ( !canPerformTask() ) {
        return;
    }

    ContactsGroupPtr group( new ContactsGroup() );
    group->setId( collection.remoteId() );

    group->setTitle( collection.displayName() );

    ContactsGroupModifyJob *modifyJob = new ContactsGroupModifyJob( group, account(), this );
    modifyJob->setProperty( COLLECTION_PROPERTY, QVariant::fromValue( collection ) );
    connect( modifyJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotGenericJobFinished(KGAPI2::Job*)) );
}

void ContactsResource::collectionRemoved( const Akonadi::Collection &collection )
{
    if ( !canPerformTask() ) {
        return;
    }

    ContactsGroupDeleteJob *deleteJob = new ContactsGroupDeleteJob( collection.remoteId(), account(), this );
    deleteJob->setProperty( COLLECTION_PROPERTY, QVariant::fromValue( collection ) );
    connect( deleteJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotGenericJobFinished(KGAPI2::Job*)) );
}


void ContactsResource::slotCollectionsRetrieved( KGAPI2::Job *job )
{
    if ( !handleError( job ) ) {
        return;
    }

    ContactsGroupFetchJob *fetchJob = qobject_cast<ContactsGroupFetchJob *>( job );
    const ObjectsList objects = fetchJob->items();

    CachePolicy cachePolicy;
    if ( Settings::self()->enableIntervalCheck() ) {
        cachePolicy.setInheritFromParent( false );
        cachePolicy.setIntervalCheckTime( Settings::self()->intervalCheckTime() );
    }

    m_rootCollection = Collection();
    m_rootCollection.setContentMimeTypes( QStringList() << Collection::virtualMimeType()
                                                        << KABC::Addressee::mimeType() );
    m_rootCollection.setRemoteId( MYCONTACTS_REMOTEID );
    m_rootCollection.setName( fetchJob->account()->accountName() );
    m_rootCollection.setParentCollection( Collection::root() );
    m_rootCollection.setCachePolicy( cachePolicy );
    m_rootCollection.setRights( Collection::CanCreateCollection |
                                Collection::CanCreateItem |
                                Collection::CanChangeItem |
                                Collection::CanDeleteItem);

    EntityDisplayAttribute *attr = m_rootCollection.attribute<EntityDisplayAttribute>( Entity::AddIfMissing );
    attr->setDisplayName( fetchJob->account()->accountName() );
    attr->setIconName( QLatin1String( "im-google" ) );

    m_collections[ MYCONTACTS_REMOTEID ] = m_rootCollection;

    foreach( const ObjectPtr & object, objects ) {
        const ContactsGroupPtr group = object.dynamicCast<ContactsGroup>();

        QString realName = group->title();

        if ( group->isSystemGroup() ) {
            if ( group->title().contains( QLatin1String( "Coworkers" ) ) ) {
                realName = i18nc( "Name of a group of contacts", "Coworkers" );
            } else if ( group->title().contains( QLatin1String( "Friends" ) ) ) {
                realName = i18nc( "Name of a group of contacts", "Friends" );
            } else if ( group->title().contains( QLatin1String( "Family" ) ) ) {
                realName = i18nc( "Name of a group of contacts", "Family" );
            } else if ( group->title().contains( QLatin1String( "My Contacts" ) ) ) {
                // Yes, skip My Contacts group, we store "My Contacts" in root collection
                continue;
            }
        } else {
            if ( group->title().contains( QLatin1String( "Other Contacts" ) ) ) {
                realName = i18nc( "Name of a group of contacts", "Other Contacts" );
            }
        }

        Collection collection;
        collection.setContentMimeTypes( QStringList() << KABC::Addressee::mimeType() );
        collection.setName( group->id() );
        collection.setParentCollection( m_rootCollection );
        collection.setRights( Collection::CanLinkItem |
                              Collection::CanUnlinkItem |
                              Collection::CanChangeItem );
        if ( !group->isSystemGroup() ) {
            collection.setRights( collection.rights() |
                                  Collection::CanChangeCollection |
                                  Collection::CanDeleteCollection );
        }
        collection.setRemoteId( group->id() );
        collection.setVirtual( true );

        EntityDisplayAttribute *attr = collection.attribute<EntityDisplayAttribute>( Entity::AddIfMissing );
        attr->setDisplayName( realName );
        attr->setIconName( QLatin1String("view-pim-contacts") );

        m_collections[ collection.remoteId() ] = collection;
    }

    Collection otherCollection;
    otherCollection.setContentMimeTypes( QStringList() << KABC::Addressee::mimeType() );
    otherCollection.setName( i18n( "Other Contacts" ) );
    otherCollection.setParentCollection( m_rootCollection );
    otherCollection.setRights( Collection::CanCreateItem |
                               Collection::CanChangeItem |
                               Collection::CanDeleteItem );
    otherCollection.setRemoteId( OTHERCONTACTS_REMOTEID );

    attr = otherCollection.attribute<EntityDisplayAttribute>( Entity::AddIfMissing );
    attr->setDisplayName( i18n( "Other Contacts" ) );
    attr->setIconName( QLatin1String("view-pim-contacts") );
    m_collections[ OTHERCONTACTS_REMOTEID ] = otherCollection;

    collectionsRetrieved( m_collections.values() );
    job->deleteLater();
}

void ContactsResource::slotItemsRetrieved( KGAPI2::Job *job )
{
    if ( !handleError( job ) ) {
        return;
    }

    ContactFetchJob *fetchJob = qobject_cast<ContactFetchJob *>( job );
    const ObjectsList objects = fetchJob->items();

    Collection collection = fetchJob->property( COLLECTION_PROPERTY ).value<Collection>();

    Item::List changedItems, removedItems;
    QMap<QString, Item::List> groupsMap;
    QList<QString> changedPhotos;
    foreach( const ObjectPtr & object, objects ) {
        const ContactPtr contact = object.dynamicCast<Contact>();

        if ( ( ( collection.remoteId() == OTHERCONTACTS_REMOTEID ) && !contact->groups().isEmpty() ) ||
             ( ( collection.remoteId() == MYCONTACTS_REMOTEID ) && contact->groups().isEmpty() ) ) {
            continue;
        }

        Item item;
        item.setMimeType( KABC::Addressee::mimeType() );
        item.setParentCollection( m_collections[MYCONTACTS_REMOTEID] );
        item.setRemoteId( contact->uid() );
        item.setRemoteRevision( contact->etag() );
        item.setPayload<KABC::Addressee>( *contact.dynamicCast<KABC::Addressee>() );

        if ( contact->deleted() ) {
            removedItems << item;
        } else {
            changedItems << item;
            changedPhotos << contact->uid();
        }

        const QStringList groups = contact->groups();
        foreach( const QString & group, groups ) {
            groupsMap[group] << item;
        }
    }

    itemsRetrievedIncremental( changedItems, removedItems );

    QMap<QString, Item::List>::ConstIterator iter;

    for ( iter = groupsMap.constBegin(); iter != groupsMap.constEnd(); ++iter ) {
        new LinkJob( m_collections[iter.key()], iter.value(), this );
    }

    QVariantMap map;
    map[QLatin1String("collection")] = QVariant::fromValue(collection);
    map[QLatin1String("modifiedItems")] = QVariant::fromValue(changedPhotos);
    scheduleCustomTask( this, "retrieveContactsPhotos", map );

    collection.setRemoteRevision( QString::number( KDateTime::currentUtcDateTime().toTime_t() ) );
    new CollectionModifyJob( collection, this );

    job->deleteLater();
}

void ContactsResource::slotUpdatePhotosItemsRetrieved( KJob *job )
{
    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>(job);
    const Item::List items = fetchJob->items();
    const QList<QString> modifiedItems = fetchJob->property( "modifiedItems" ).value< QList<QString> >();
    ContactsList contacts;

    foreach( const Item &item, items ) {
        if ( modifiedItems.contains( item.remoteId() )) {
            const KABC::Addressee addressee = item.payload<KABC::Addressee>();
            const ContactPtr contact( new Contact( addressee ) );
            contacts << contact;
        }
    }

    // Make sure account is still valid
    if ( !canPerformTask() ) {
        return;
    }

    ContactFetchPhotoJob *photoJob = new ContactFetchPhotoJob( contacts, account(), this );
    photoJob->setProperty( ITEMLIST_PROPERTY, QVariant::fromValue( items ) );
    photoJob->setProperty( "processedItems", 0 );
    connect( photoJob, SIGNAL(photoFetched(KGAPI2::Job*,KGAPI2::ContactPtr)),
             this, SLOT(slotUpdatePhotoFinished(KGAPI2::Job*,KGAPI2::ContactPtr)) );
    connect( photoJob, SIGNAL(finished(KGAPI2::Job*)),
             this, SLOT(slotGenericJobFinished(KGAPI2::Job*)) );
}

void ContactsResource::slotUpdatePhotoFinished( KGAPI2::Job *job, const ContactPtr &contact )
{
    Item::List items = job->property( ITEMLIST_PROPERTY ).value<Item::List>();

    int processedItems = job->property( "processedItems" ).toInt();
    processedItems++;
    job->setProperty( "processedItems", processedItems );
    emitPercent( job, processedItems, items.count() );

    foreach( Item item, items ) {
        if ( item.remoteId() == contact->uid() ) {
            item.setPayload<KABC::Addressee>( *contact.dynamicCast<KABC::Addressee>() );
            new ItemModifyJob( item, this );
            return;
        }
    }
}

void ContactsResource::slotCreateJobFinished( KGAPI2::Job* job )
{
    if ( !handleError( job ) ) {
        return;
    }

    Item item = job->property( ITEM_PROPERTY ).value<Item>();
    Collection collection = job->property( COLLECTION_PROPERTY ).value<Collection>();
    if ( item.isValid() ) {
        ContactCreateJob *createJob = qobject_cast<ContactCreateJob*>( job );
        Q_ASSERT( createJob->items().count() == 1);
        ContactPtr contact = createJob->items().first().dynamicCast<Contact>();

        item.setRemoteId( contact->uid() );
        item.setRemoteRevision( contact->etag() );
        changeCommitted( item );
    } else if ( collection.isValid() ) {
        ContactsGroupCreateJob *createJob = qobject_cast<ContactsGroupCreateJob*>( job );
        Q_ASSERT( createJob->items().count() == 1);
        ContactsGroupPtr group = createJob->items().first().dynamicCast<ContactsGroup>();

        collection.setRemoteId( group->id() );
        collection.setContentMimeTypes( QStringList() << KABC::Addressee::mimeType() );

        EntityDisplayAttribute *attr = collection.attribute<EntityDisplayAttribute>( Entity::AddIfMissing );
        attr->setDisplayName( group->title() );
        attr->setIconName( QLatin1String("view-pim-contacts") );

        m_collections[ collection.remoteId() ] = collection;

        changeCommitted( collection );
    }

    job->deleteLater();
}


AKONADI_RESOURCE_MAIN( ContactsResource )
