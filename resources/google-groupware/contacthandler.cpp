/*
    SPDX-FileCopyrightText: 2011-2013 Daniel Vrátil <dvratil@redhat.com>
    SPDX-FileCopyrightText: 2020 Igor Pobiko <igor.poboiko@gmail.com>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "contacthandler.h"
#include "googleresource.h"
#include "googlesettings.h"

#include "googlecontacts_debug.h"

#include <AkonadiCore/CollectionModifyJob>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/EntityHiddenAttribute>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>
#include <AkonadiCore/ItemModifyJob>
#include <AkonadiCore/LinkJob>
#include <AkonadiCore/VectorHelper>

#include <KContacts/Addressee>
#include <KContacts/Picture>

#include <KGAPI/Account>
#include <KGAPI/Contacts/Contact>
#include <KGAPI/Contacts/ContactCreateJob>
#include <KGAPI/Contacts/ContactDeleteJob>
#include <KGAPI/Contacts/ContactFetchJob>
#include <KGAPI/Contacts/ContactFetchPhotoJob>
#include <KGAPI/Contacts/ContactModifyJob>
#include <KGAPI/Contacts/ContactsGroup>
#include <KGAPI/Contacts/ContactsGroupCreateJob>
#include <KGAPI/Contacts/ContactsGroupDeleteJob>
#include <KGAPI/Contacts/ContactsGroupFetchJob>
#include <KGAPI/Contacts/ContactsGroupModifyJob>

#define OTHERCONTACTS_REMOTEID QStringLiteral("OtherContacts")

using namespace KGAPI2;
using namespace Akonadi;

QString ContactHandler::mimeType()
{
    return KContacts::Addressee::mimeType();
}

bool ContactHandler::canPerformTask(const Item &item)
{
    return GenericHandler::canPerformTask<KContacts::Addressee>(item);
}

bool ContactHandler::canPerformTask(const Item::List &items)
{
    return GenericHandler::canPerformTask<KContacts::Addressee>(items);
}

QString ContactHandler::myContactsRemoteId() const
{
    return QStringLiteral("http://www.google.com/m8/feeds/groups/%1/base/6").arg(QString::fromLatin1(QUrl::toPercentEncoding(m_settings->accountPtr()->accountName())));
}

void ContactHandler::setupCollection(Collection &collection, const ContactsGroupPtr &group)
{
    collection.setContentMimeTypes({ mimeType() });
    collection.setName(group->id());
    collection.setRemoteId(group->id());

    QString realName = group->title();
    if (group->isSystemGroup()) {
        if (realName.contains(QLatin1String("Coworkers"))) {
            realName = i18nc("Name of a group of contacts", "Coworkers");
        } else if (realName.contains(QLatin1String("Friends"))) {
            realName = i18nc("Name of a group of contacts", "Friends");
        } else if (realName.contains(QLatin1String("Family"))) {
            realName = i18nc("Name of a group of contacts", "Family");
        } else if (realName.contains(QLatin1String("My Contacts"))) {
            realName = i18nc("Name of a group of contacts", "My Contacts");
        }
    }

    // "My Contacts" is the only one not virtual
    if (group->id() == myContactsRemoteId()) {
        collection.setRights(Collection::CanCreateItem
                             |Collection::CanChangeItem
                             |Collection::CanDeleteItem);
    } else {
        collection.setRights(Collection::CanLinkItem
                             |Collection::CanUnlinkItem
                             |Collection::CanChangeItem);
        collection.setVirtual(true);
        if (!group->isSystemGroup()) {
            collection.setRights(collection.rights()
                                 |Collection::CanChangeCollection
                                 |Collection::CanDeleteCollection);
        }
    }

    auto attr = collection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
    attr->setDisplayName(realName);
    attr->setIconName(QStringLiteral("view-pim-contacts"));
}

void ContactHandler::retrieveCollections(const Collection &rootCollection)
{
    m_iface->emitStatus(AgentBase::Running, i18nc("@info:status", "Retrieving contacts groups"));
    qCDebug(GOOGLE_CONTACTS_LOG) << "Retrieving contacts groups...";
    m_collections.clear();

    Collection otherCollection;
    otherCollection.setContentMimeTypes({ mimeType() });
    otherCollection.setName(i18n("Other Contacts"));
    otherCollection.setParentCollection(rootCollection);
    otherCollection.setRights(Collection::CanCreateItem
                              |Collection::CanChangeItem
                              |Collection::CanDeleteItem);
    otherCollection.setRemoteId(OTHERCONTACTS_REMOTEID);

    auto attr = otherCollection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
    attr->setDisplayName(i18n("Other Contacts"));
    attr->setIconName(QStringLiteral("view-pim-contacts"));

    m_iface->collectionsRetrieved({ otherCollection });
    m_collections[ OTHERCONTACTS_REMOTEID ] = otherCollection;

    auto job = new ContactsGroupFetchJob(m_settings->accountPtr(), this);
    connect(job, &ContactFetchJob::finished, this, [this, rootCollection](KGAPI2::Job *job){
        if (!m_iface->handleError(job)) {
            return;
        }
        qCDebug(GOOGLE_CONTACTS_LOG) << "Contacts groups retrieved";

        const ObjectsList objects = qobject_cast<ContactsGroupFetchJob *>(job)->items();
        Collection::List collections;
        collections.reserve(objects.count());
        std::transform(objects.cbegin(), objects.cend(), std::back_inserter(collections),
                       [this, &rootCollection](const ObjectPtr &object){
            const ContactsGroupPtr group = object.dynamicCast<ContactsGroup>();
            qCDebug(GOOGLE_CONTACTS_LOG) << " -" << group->title() << "(" << group->id() << ")";
            Collection collection;
            setupCollection(collection, group);
            collection.setParentCollection(rootCollection);
            m_collections[ collection.remoteId() ] = collection;
            return collection;
        });
        m_iface->collectionsRetrievedFromHandler(collections);
    });
}

void ContactHandler::retrieveItems(const Collection &collection)
{
    // Contacts are stored inside "My Contacts" and "Other Contacts" only
    if ((collection.remoteId() != OTHERCONTACTS_REMOTEID)
        && (collection.remoteId() != myContactsRemoteId())) {
        m_iface->itemsRetrievalDone();
        return;
    }
    m_iface->emitStatus(AgentBase::Running, i18nc("@info:status", "Retrieving contacts for group '%1'", collection.displayName()));
    qCDebug(GOOGLE_CONTACTS_LOG) << "Retreiving contacts for group" << collection.remoteId() << "...";

    auto job = new ContactFetchJob(m_settings->accountPtr(), this);
    if (!collection.remoteRevision().isEmpty()) {
        job->setFetchOnlyUpdated(collection.remoteRevision().toLongLong());
        job->setFetchDeleted(true);
    } else {
        // No need to fetch deleted items for a non-incremental update
        job->setFetchDeleted(false);
    }
    connect(job, &ContactFetchJob::finished, this, &ContactHandler::slotItemsRetrieved);
}

void ContactHandler::slotItemsRetrieved(KGAPI2::Job *job)
{
    if (!m_iface->handleError(job)) {
        return;
    }

    Collection collection = m_iface->currentCollection();

    Item::List changedItems, removedItems;
    QHash<QString, Item::List> groupsMap;
    QStringList changedPhotos;

    auto fetchJob = qobject_cast<ContactFetchJob *>(job);
    bool isIncremental = (fetchJob->fetchOnlyUpdated() > 0);
    const ObjectsList objects = fetchJob->items();
    qCDebug(GOOGLE_CONTACTS_LOG) << "Retrieved" << objects.count() << "contacts";
    for (const ObjectPtr &object : objects) {
        const ContactPtr contact = object.dynamicCast<Contact>();

        Item item;
        item.setMimeType(mimeType());
        item.setParentCollection(collection);
        item.setRemoteId(contact->uid());
        item.setRemoteRevision(contact->etag());
        item.setPayload<KContacts::Addressee>(*contact.dynamicCast<KContacts::Addressee>());

        if (contact->deleted()
            || (collection.remoteId() == OTHERCONTACTS_REMOTEID && !contact->groups().isEmpty())
            || (collection.remoteId() == myContactsRemoteId() && contact->groups().isEmpty())) {
            qCDebug(GOOGLE_CONTACTS_LOG) << " - removed" << contact->uid();
            removedItems << item;
        } else {
            qCDebug(GOOGLE_CONTACTS_LOG) << " - changed" << contact->uid();
            changedItems << item;
            changedPhotos << contact->uid();
        }

        const QStringList groups = contact->groups();
        for (const QString &group : groups) {
            // We don't link contacts to "My Contacts"
            if (group != myContactsRemoteId()) {
                groupsMap[group] << item;
            }
        }
    }

    if (isIncremental) {
        m_iface->itemsRetrievedIncremental(changedItems, removedItems);
    } else {
        m_iface->itemsRetrieved(changedItems);
    }

    for (auto iter = groupsMap.constBegin(), iterEnd = groupsMap.constEnd(); iter != iterEnd; ++iter) {
        new LinkJob(m_collections[iter.key()], iter.value(), this);
    }
    // TODO: unlink if the group was removed!

    if (!changedPhotos.isEmpty()) {
        QVariantMap map;
        map[QStringLiteral("collection")] = QVariant::fromValue(collection);
        map[QStringLiteral("modified")] = QVariant::fromValue(changedPhotos);
        m_iface->scheduleCustomTask(this, "retrieveContactsPhotos", map);
    }

    const QDateTime local(QDateTime::currentDateTime());
    const QDateTime UTC(local.toUTC());

    collection.setRemoteRevision(QString::number(UTC.toSecsSinceEpoch()));
    new CollectionModifyJob(collection, this);

    emitReadyStatus();
}

void ContactHandler::retrieveContactsPhotos(const QVariant &argument)
{
    if (!m_iface->canPerformTask()) {
        return;
    }
    const auto map = argument.value<QVariantMap>();
    const auto collection = map[QStringLiteral("collection")].value<Collection>();
    const auto changedPhotos = map[QStringLiteral("modified")].toStringList();
    m_iface->emitStatus(AgentBase::Running, i18ncp("@info:status", "Retrieving %1 contact photo for group '%2'",
                                                   "Retrieving %1 contact photos for group '%2'",
                                                   changedPhotos.count(), collection.displayName()));

    Item::List items;
    items.reserve(changedPhotos.size());
    std::transform(changedPhotos.cbegin(), changedPhotos.cend(), std::back_inserter(items),
                   [](const QString &contact){
        Item item;
        item.setRemoteId(contact);
        return item;
    });
    auto job = new ItemFetchJob(items, this);
    job->setCollection(collection);
    job->fetchScope().fetchFullPayload(true);
    connect(job, &ItemFetchJob::finished, this, &ContactHandler::slotUpdatePhotosItemsRetrieved);
}

void ContactHandler::slotUpdatePhotosItemsRetrieved(KJob *job)
{
    // Make sure account is still valid
    if (!m_iface->canPerformTask()) {
        return;
    }
    const Item::List items = qobject_cast<ItemFetchJob *>(job)->items();
    qCDebug(GOOGLE_CONTACTS_LOG) << "Fetched" << items.count() << "contacts for photo update";
    ContactsList contacts;
    contacts.reserve(items.size());
    std::transform(items.cbegin(), items.cend(), std::back_inserter(contacts),
                   [](const Item &item){
        return ContactPtr(new Contact(item.payload<KContacts::Addressee>()));
    });

    auto photoJob = new ContactFetchPhotoJob(contacts, m_settings->accountPtr(), this);
    photoJob->setProperty("processedItems", 0);
    connect(photoJob, &ContactFetchPhotoJob::photoFetched, this, [this, items](KGAPI2::Job *job, const ContactPtr &contact){
        qCDebug(GOOGLE_CONTACTS_LOG) << " - fetched photo for contact" << contact->uid();
        int processedItems = job->property("processedItems").toInt();
        processedItems++;
        job->setProperty("processedItems", processedItems);
        m_iface->emitPercent(100.0f*processedItems / items.count());

        auto it = std::find_if(items.cbegin(), items.cend(), [&contact](const Item &item){
            return item.remoteId() == contact->uid();
        });
        if (it != items.cend()) {
            Item newItem(*it);
            newItem.setPayload<KContacts::Addressee>(*contact.dynamicCast<KContacts::Addressee>());
            new ItemModifyJob(newItem, this);
        }
    });

    connect(photoJob, &ContactFetchPhotoJob::finished, this, &ContactHandler::slotGenericJobFinished);
}

void ContactHandler::itemAdded(const Item &item, const Collection &collection)
{
    m_iface->emitStatus(AgentBase::Running, i18nc("@info:status", "Adding contact to group '%1'", collection.displayName()));
    ContactPtr contact(new Contact(item.payload<KContacts::Addressee>()));
    qCDebug(GOOGLE_CONTACTS_LOG) << "Creating contact";

    if (collection.remoteId() == myContactsRemoteId()) {
        contact->addGroup(myContactsRemoteId());
    }

    auto job = new ContactCreateJob(contact, m_settings->accountPtr(), this);
    connect(job, &ContactCreateJob::finished, this, [this, item](KGAPI2::Job *job){
        if (!m_iface->handleError(job)) {
            return;
        }
        ContactPtr contact = qobject_cast<ContactCreateJob *>(job)->items().first().dynamicCast<Contact>();
        Item newItem = item;
        qCDebug(GOOGLE_CONTACTS_LOG) << "Contact" << contact->uid() << "created";
        newItem.setRemoteId(contact->uid());
        newItem.setRemoteRevision(contact->etag());
        m_iface->itemChangeCommitted(newItem);
        newItem.setPayload<KContacts::Addressee>(*contact.dynamicCast<KContacts::Addressee>());
        new ItemModifyJob(newItem, this);
        emitReadyStatus();
    });
}

void ContactHandler::itemChanged(const Item &item, const QSet< QByteArray > & /*partIdentifiers*/)
{
    m_iface->emitStatus(AgentBase::Running, i18nc("@info:status", "Changing contact"));
    qCDebug(GOOGLE_CONTACTS_LOG) << "Changing contact" << item.remoteId();
    ContactPtr contact(new Contact(item.payload<KContacts::Addressee>()));
    auto job = new ContactModifyJob(contact, m_settings->accountPtr(), this);
    job->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(job, &ContactModifyJob::finished, this, &ContactHandler::slotGenericJobFinished);
}

void ContactHandler::itemsRemoved(const Item::List &items)
{
    m_iface->emitStatus(AgentBase::Running, i18ncp("@info:status", "Removing %1 contact", "Removing %1 contacts", items.count()));
    QStringList contactIds;
    contactIds.reserve(items.count());
    std::transform(items.cbegin(), items.cend(), std::back_inserter(contactIds),
                   [](const Item &item){
        return item.remoteId();
    });
    qCDebug(GOOGLE_CONTACTS_LOG) << "Removing contacts" << contactIds;
    auto job = new ContactDeleteJob(contactIds, m_settings->accountPtr(), this);
    job->setProperty(ITEMS_PROPERTY, QVariant::fromValue(items));
    connect(job, &ContactDeleteJob::finished, this, &ContactHandler::slotGenericJobFinished);
}

void ContactHandler::itemsMoved(const Item::List &items, const Collection &collectionSource, const Collection &collectionDestination)
{
    qCDebug(GOOGLE_CONTACTS_LOG) << "Moving contacts from" << collectionSource.remoteId() << "to" << collectionDestination.remoteId();
    if (!(((collectionSource.remoteId() == myContactsRemoteId()) && (collectionDestination.remoteId() == OTHERCONTACTS_REMOTEID))
          || ((collectionSource.remoteId() == OTHERCONTACTS_REMOTEID) && (collectionDestination.remoteId() == myContactsRemoteId())))) {
        m_iface->cancelTask(i18n("Invalid source or destination collection"));
    }

    m_iface->emitStatus(AgentBase::Running, i18ncp("@info:status", "Moving %1 contact from group '%2' to '%3'",
                                                   "Moving %1 contacts from group '%2' to '%3'",
                                                   items.count(), collectionSource.remoteId(), collectionDestination.remoteId()));
    ContactsList contacts;
    contacts.reserve(items.count());
    std::transform(items.cbegin(), items.cend(), std::back_inserter(contacts),
                   [this, &collectionSource, &collectionDestination](const Item &item){
        ContactPtr contact(new Contact(item.payload<KContacts::Addressee>()));
        // MyContacts -> OtherContacts
        if (collectionSource.remoteId() == myContactsRemoteId()
            && collectionDestination.remoteId() == OTHERCONTACTS_REMOTEID) {
            contact->clearGroups();
            // OtherContacts -> MyContacts
        } else if (collectionSource.remoteId() == OTHERCONTACTS_REMOTEID
                   && collectionDestination.remoteId() == myContactsRemoteId()) {
            contact->addGroup(myContactsRemoteId());
        }

        return contact;
    });
    qCDebug(GOOGLE_CONTACTS_LOG) << "Moving contacts from" << collectionSource.remoteId() << "to" << collectionDestination.remoteId();
    auto job = new ContactModifyJob(contacts, m_settings->accountPtr(), this);
    job->setProperty(ITEMS_PROPERTY, QVariant::fromValue(items));
    connect(job, &ContactModifyJob::finished, this, &ContactHandler::slotGenericJobFinished);
}

void ContactHandler::itemsLinked(const Item::List &items, const Collection &collection)
{
    m_iface->emitStatus(AgentBase::Running, i18ncp("@info:status", "Linking %1 contact", "Linking %1 contacts", items.count()));
    qCDebug(GOOGLE_CONTACTS_LOG) << "Linking" << items.count() << "contacts to group" << collection.remoteId();

    ContactsList contacts;
    contacts.reserve(items.count());
    std::transform(items.cbegin(), items.cend(), std::back_inserter(contacts),
                   [this, &collection](const Item &item){
        ContactPtr contact(new Contact(item.payload<KContacts::Addressee>()));
        contact->addGroup(collection.remoteId());
        return contact;
    });
    auto job = new ContactModifyJob(contacts, m_settings->accountPtr(), this);
    job->setProperty(ITEMS_PROPERTY, QVariant::fromValue(items));
    connect(job, &ContactModifyJob::finished, this, &ContactHandler::slotGenericJobFinished);
}

void ContactHandler::itemsUnlinked(const Item::List &items, const Collection &collection)
{
    m_iface->emitStatus(AgentBase::Running, i18ncp("@info:status", "Unlinking %1 contact", "Unlinking %1 contacts", items.count()));
    qCDebug(GOOGLE_CONTACTS_LOG) << "Unlinking" << items.count() << "contacts from group" << collection.remoteId();

    ContactsList contacts;
    contacts.reserve(items.count());
    std::transform(items.cbegin(), items.cend(), std::back_inserter(contacts),
                   [&collection](const Item &item){
        ContactPtr contact(new Contact(item.payload<KContacts::Addressee>()));
        contact->removeGroup(collection.remoteId());
        return contact;
    });
    auto job = new ContactModifyJob(contacts, m_settings->accountPtr(), this);
    job->setProperty(ITEMS_PROPERTY, QVariant::fromValue(items));
    connect(job, &ContactModifyJob::finished, this, &ContactHandler::slotGenericJobFinished);
}

void ContactHandler::collectionAdded(const Collection &collection, const Collection & /*parent*/)
{
    m_iface->emitStatus(AgentBase::Running, i18nc("@info:status", "Creating new contact group '%1'", collection.displayName()));
    qCDebug(GOOGLE_CONTACTS_LOG) << "Adding contact group" << collection.displayName();
    ContactsGroupPtr group(new ContactsGroup);
    group->setTitle(collection.name());
    group->setIsSystemGroup(false);

    auto job = new ContactsGroupCreateJob(group, m_settings->accountPtr(), this);
    connect(job, &ContactsGroupCreateJob::finished, this, [this, collection](KGAPI2::Job *job){
        if (!m_iface->handleError(job)) {
            return;
        }

        ContactsGroupPtr group = qobject_cast<ContactsGroupCreateJob *>(job)->items().first().dynamicCast<ContactsGroup>();
        qCDebug(GOOGLE_CONTACTS_LOG) << "Contact group created:" << group->id();
        Collection newCollection(collection);
        setupCollection(newCollection, group);
        m_collections[ newCollection.remoteId() ] = newCollection;
        m_iface->collectionChangeCommitted(newCollection);
        emitReadyStatus();
    });
}

void ContactHandler::collectionChanged(const Collection &collection)
{
    m_iface->emitStatus(AgentBase::Running, i18nc("@info:status", "Changing contact group '%1'", collection.displayName()));
    qCDebug(GOOGLE_CONTACTS_LOG) << "Changing contact group" << collection.remoteId();

    ContactsGroupPtr group(new ContactsGroup());
    group->setId(collection.remoteId());
    group->setTitle(collection.displayName());

    auto job = new ContactsGroupModifyJob(group, m_settings->accountPtr(), this);
    job->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(job, &ContactsGroupModifyJob::finished, this, &ContactHandler::slotGenericJobFinished);
}

void ContactHandler::collectionRemoved(const Collection &collection)
{
    m_iface->emitStatus(AgentBase::Running, i18nc("@info:status", "Removing contact group '%1'", collection.displayName()));
    qCDebug(GOOGLE_CONTACTS_LOG) << "Removing contact group" << collection.remoteId();
    auto job = new ContactsGroupDeleteJob(collection.remoteId(), m_settings->accountPtr(), this);
    job->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(job, &ContactsGroupDeleteJob::finished, this, &ContactHandler::slotGenericJobFinished);
}
