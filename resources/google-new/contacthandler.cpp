/*
    Copyright (C) 2011-2013  Daniel Vrátil <dvratil@redhat.com>
                  2020  Igor Poboiko <igor.poboiko@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "contacthandler.h"
#include "googleresource.h"
#include "kgapiversionattribute.h"
#include "settings.h"

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

#include "googleresource_debug.h"

#define ALLCONTACTS_REMOTEID QStringLiteral("AllContacts")
#define OTHERCONTACTS_REMOTEID QStringLiteral("OtherContacts")
#define MODIFIED_PROPERTY "modifiedItems"

using namespace KGAPI2;
using namespace Akonadi;

QString ContactHandler::mimetype()
{
    return KContacts::Addressee::mimeType();
}

bool ContactHandler::canPerformTask(const Item &item)
{
    return m_resource->canPerformTask<KContacts::Addressee>(item, mimetype());
}

QString ContactHandler::myContactsRemoteId()
{
    return QStringLiteral("http://www.google.com/m8/feeds/groups/%1/base/6").arg(QString::fromLatin1(QUrl::toPercentEncoding(m_resource->account()->accountName())));
}

void ContactHandler::retrieveCollections()
{
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Retrieving contacts groups"));
    qCDebug(GOOGLE_LOG) << "Retrieving contacts groups...";

    m_allCollection.setContentMimeTypes({ KContacts::Addressee::mimeType() });
    m_allCollection.setName(i18n("All Contacts"));
    m_allCollection.setParentCollection(m_resource->rootCollection());
    m_allCollection.setRemoteId(ALLCONTACTS_REMOTEID);
    m_allCollection.attribute<EntityHiddenAttribute>(Collection::AddIfMissing);
    m_allCollection.setRights(Collection::ReadOnly);
    m_collections[ ALLCONTACTS_REMOTEID ] = m_allCollection;

    Collection otherCollection;
    otherCollection.setContentMimeTypes({ KContacts::Addressee::mimeType() });
    otherCollection.setName(i18n("Other Contacts"));
    otherCollection.setParentCollection(m_resource->rootCollection());
    otherCollection.setRights(Collection::CanCreateItem
                              |Collection::CanChangeItem
                              |Collection::CanDeleteItem);
    otherCollection.setRemoteId(OTHERCONTACTS_REMOTEID);
    otherCollection.setVirtual(true);

    auto attr = otherCollection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
    attr->setDisplayName(i18n("Other Contacts"));
    attr->setIconName(QStringLiteral("view-pim-contacts"));

    m_collections[ OTHERCONTACTS_REMOTEID ] = otherCollection;

    auto job = new ContactsGroupFetchJob(m_resource->account(), this);
    connect(job, &ContactFetchJob::finished, this, &ContactHandler::slotCollectionsRetrieved);
}

void ContactHandler::slotCollectionsRetrieved(KGAPI2::Job* job)
{
    if (!m_resource->handleError(job)) {
        return;
    }
    qCDebug(GOOGLE_LOG) << "Contacts groups retrieved";

    const ObjectsList objects = qobject_cast<ContactsGroupFetchJob *>(job)->items();
    for (const auto &object : objects) {
        const ContactsGroupPtr group = object.dynamicCast<ContactsGroup>();
        qCDebug(GOOGLE_LOG) << "Retrieved contact group:" << group->id() << "(" << group->title() << ")";

        QString realName = group->title();

        if (group->isSystemGroup()) {
            if (group->title().contains(QLatin1String("Coworkers"))) {
                realName = i18nc("Name of a group of contacts", "Coworkers");
            } else if (group->title().contains(QLatin1String("Friends"))) {
                realName = i18nc("Name of a group of contacts", "Friends");
            } else if (group->title().contains(QLatin1String("Family"))) {
                realName = i18nc("Name of a group of contacts", "Family");
            } else if (group->title().contains(QLatin1String("My Contacts"))) {
                realName = i18nc("Name of a group of contacts", "My Contacts");
            }
        }

        Collection collection;
        collection.setContentMimeTypes({ KContacts::Addressee::mimeType() });
        collection.setName(group->id());
        collection.setParentCollection(m_resource->rootCollection());
        collection.setRights(Collection::CanLinkItem
                             |Collection::CanUnlinkItem
                             |Collection::CanChangeItem);
        if (!group->isSystemGroup()) {
            collection.setRights(collection.rights()
                                 |Collection::CanChangeCollection
                                 |Collection::CanDeleteCollection);
        }
        if (group->id() == myContactsRemoteId()) {
            collection.setRights(collection.rights()
                                 |Collection::CanCreateItem
                                 |Collection::CanChangeItem
                                 |Collection::CanDeleteItem);
        }
        collection.setRemoteId(group->id());
        collection.setVirtual(true);

        auto attr = collection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
        attr->setDisplayName(realName);
        attr->setIconName(QStringLiteral("view-pim-contacts"));

        m_collections[ collection.remoteId() ] = collection;
    }

    Q_EMIT collectionsRetrieved(valuesToVector(m_collections));
    emitReadyStatus();
}

void ContactHandler::retrieveItems(const Collection &collection)
{
    // All items are stored in "Everything" collection
    if (collection.remoteId() != ALLCONTACTS_REMOTEID) {
        m_resource->itemsRetrievalDone();
        return;
    }
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Retrieving contacts"));
    qCDebug(GOOGLE_LOG) << "Retreiving contacts...";

    auto job = new ContactFetchJob(m_resource->account(), this);
    job->setFetchDeleted(true);
    if (!collection.remoteRevision().isEmpty()) {
        job->setFetchOnlyUpdated(collection.remoteRevision().toLongLong());
    }
    connect(job, &ContactFetchJob::finished, this, &ContactHandler::slotItemsRetrieved);
}

void ContactHandler::slotItemsRetrieved(KGAPI2::Job *job)
{
    if (!m_resource->handleError(job)) {
        return;
    }

    Item::List changedItems, removedItems;
    QMap<QString, Item::List> groupsMap;
    QList<QString> changedPhotos;
    auto fetchJob = qobject_cast<ContactFetchJob *>(job);
    bool isIncremental = (fetchJob->fetchOnlyUpdated() > 0);
    const ObjectsList objects = fetchJob->items();
    qCDebug(GOOGLE_LOG) << "Retrieved" << objects.count() << "contacts";
    for (const ObjectPtr &object : objects) {
        const ContactPtr contact = object.dynamicCast<Contact>();

        Item item;
        item.setMimeType(KContacts::Addressee::mimeType());
        item.setParentCollection(m_allCollection);
        item.setRemoteId(contact->uid());
        item.setRemoteRevision(contact->etag());
        item.setPayload<KContacts::Addressee>(*contact.dynamicCast<KContacts::Addressee>());

        if (contact->deleted()) {
            qCDebug(GOOGLE_LOG) << " - removed" << contact->uid();
            removedItems << item;
        } else {
            qCDebug(GOOGLE_LOG) << " - changed" << contact->uid();
            changedItems << item;
            changedPhotos << contact->uid();
        }

        const QStringList groups = contact->groups();
        for (const QString &group : groups) {
            groupsMap[group] << item;
        }
        if (contact->groups().isEmpty()) {
            groupsMap[OTHERCONTACTS_REMOTEID] << item;
        }
    }

    if (isIncremental) {
        m_resource->itemsRetrievedIncremental(changedItems, removedItems);
    } else {
        m_resource->itemsRetrieved(changedItems);
    }

    for (auto iter = groupsMap.constBegin(), iterEnd = groupsMap.constEnd(); iter != iterEnd; ++iter) {
        new LinkJob(m_collections[iter.key()], iter.value(), this);
    }

    if (!changedPhotos.isEmpty()) {
        qCDebug(GOOGLE_LOG) << "Requesting photos for" << changedPhotos << "contacts...";
        m_resource->scheduleCustomTask(this, "retrieveContactsPhotos", QVariant::fromValue(changedPhotos));
    }

    const QDateTime local(QDateTime::currentDateTime());
    const QDateTime UTC(local.toUTC());

    m_allCollection.setRemoteRevision(QString::number(UTC.toSecsSinceEpoch()));
    new CollectionModifyJob(m_allCollection, this);

    emitReadyStatus();
}

void ContactHandler::retrieveContactsPhotos(const QVariant &argument)
{
    if (!m_resource->canPerformTask()) {
        return;
    }
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Retrieving contacts photos"));
    qCDebug(GOOGLE_LOG) << "Retrieve contacts photos" << argument << "called";
    auto job = new ItemFetchJob(m_allCollection, this);
    job->setProperty(MODIFIED_PROPERTY, argument);
    job->fetchScope().fetchFullPayload(true);
    connect(job, &ItemFetchJob::finished, this, &ContactHandler::slotUpdatePhotosItemsRetrieved);
}

void ContactHandler::slotUpdatePhotosItemsRetrieved(KJob *job)
{
    auto fetchJob = qobject_cast<ItemFetchJob *>(job);
    const auto modifiedItems = fetchJob->property(MODIFIED_PROPERTY).value<QStringList>();
    ContactsList contacts;
    const Item::List items = fetchJob->items();
    qCDebug(GOOGLE_LOG) << "Fetched" << items.count() << "contacts for photo update";
    for (const Item &item : items) {
        if (modifiedItems.contains(item.remoteId())) {
            const KContacts::Addressee addressee = item.payload<KContacts::Addressee>();
            const ContactPtr contact(new Contact(addressee));
            contacts << contact;
            qCDebug(GOOGLE_LOG) << " -" << contact->uid();
        }
    }

    // Make sure account is still valid
    if (!m_resource->canPerformTask()) {
        return;
    }

    qCDebug(GOOGLE_LOG) << "Starting fetching photos...";
    auto photoJob = new ContactFetchPhotoJob(contacts, m_resource->account(), this);
    photoJob->setProperty("processedItems", 0);
    connect(photoJob, &ContactFetchPhotoJob::photoFetched, [this, &items](KGAPI2::Job *job, const ContactPtr &contact){
            qCDebug(GOOGLE_LOG) << " - fetched photo for contact" << contact->uid();
            int processedItems = job->property("processedItems").toInt();
            processedItems++;
            job->setProperty("processedItems", processedItems);
            Q_EMIT percent((float)processedItems / items.count());

            for (const Item item : items) {
                if (item.remoteId() == contact->uid()) {
                    Item newItem = item;
                    newItem.setPayload<KContacts::Addressee>(*contact.dynamicCast<KContacts::Addressee>());
                    new ItemModifyJob(newItem, this);
                    return;
                }
            }
        });

    connect(photoJob, &ContactFetchPhotoJob::finished, m_resource, &GoogleResource::slotGenericJobFinished);
}

void ContactHandler::itemAdded(const Item &item, const Collection &collection)
{
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Adding contact to group '%1'", collection.displayName()));
    qCDebug(GOOGLE_LOG) << "Contact added to group" << collection.remoteId();
    auto addressee = item.payload< KContacts::Addressee >();
    ContactPtr contact(new Contact(addressee));

    if (collection.remoteId() == myContactsRemoteId()) {
        /* Adding contact to "My Contacts" system group explicitly */
        contact->addGroup(myContactsRemoteId());
    }

    auto job = new ContactCreateJob(contact, m_resource->account(), this);
    connect(job, &ContactCreateJob::finished, [this, item](KGAPI2::Job* job){
            if (!m_resource->handleError(job)) {
                return;
            }
            qCDebug(GOOGLE_LOG) << "Contact created!";
            ContactPtr contact = qobject_cast<ContactCreateJob *>(job)->items().first().dynamicCast<Contact>();
            Item newItem = item;
            newItem.setRemoteId(contact->uid());
            newItem.setRemoteRevision(contact->etag());
            m_resource->changeCommitted(newItem);
            emitReadyStatus();
        });
}

void ContactHandler::itemChanged(const Item &item, const QSet< QByteArray > &partIdentifiers)
{
    Q_UNUSED(partIdentifiers);
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Changing contact"));
    qCDebug(GOOGLE_LOG) << "Changing contact" << item.remoteId();
    KContacts::Addressee addressee = item.payload< KContacts::Addressee >();
    ContactPtr contact(new Contact(addressee));

    ContactModifyJob *modifyJob = new ContactModifyJob(contact, m_resource->account(), this);
    modifyJob->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(modifyJob, &ContactModifyJob::finished, m_resource, &GoogleResource::slotGenericJobFinished);
}

void ContactHandler::itemRemoved(const Item &item)
{
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Removing contact"));
    qCDebug(GOOGLE_LOG) << "Removing contact" << item.remoteId();
    auto job = new ContactDeleteJob(item.remoteId(), m_resource->account(), this);
    job->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(job, &ContactDeleteJob::finished, m_resource, &GoogleResource::slotGenericJobFinished);
}

void ContactHandler::itemMoved(const Item &item, const Collection &collectionSource, const Collection &collectionDestination)
{
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Moving contact"));
    qCDebug(GOOGLE_LOG) << "Moving contact" << item.remoteId() << "from" << collectionSource.remoteId() << "to" << collectionDestination.remoteId();
    KContacts::Addressee addressee = item.payload< KContacts::Addressee >();
    ContactPtr contact(new Contact(addressee));

/*    // MyContacts -> OtherContacts
    if (collectionSource.remoteId() == MYCONTACTS_REMOTEID
        && collectionDestination.remoteId() == OTHERCONTACTS_REMOTEID) {
        contact->removeGroup(QStringLiteral("http://www.google.com/m8/feeds/groups/%1/base/6").arg(QString::fromLatin1(QUrl::toPercentEncoding(m_resource->account()->accountName()))));

        // OtherContacts -> MyContacts
    } else if (collectionSource.remoteId() == OTHERCONTACTS_REMOTEID
               && collectionDestination.remoteId() == MYCONTACTS_REMOTEID) {
        contact->addGroup(QStringLiteral("http://www.google.com/m8/feeds/groups/%1/base/6").arg(QString::fromLatin1(QUrl::toPercentEncoding(m_resource->account()->accountName()))));
    } else {*/
        m_resource->cancelTask(i18n("Invalid source or destination collection"));
        return;
//    }

    auto job = new ContactModifyJob(contact, m_resource->account(), this);
    job->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(job, &ContactModifyJob::finished, m_resource, &GoogleResource::slotGenericJobFinished);
}

void ContactHandler::collectionAdded(const Collection &collection, const Collection &parent)
{
    Q_UNUSED(parent);
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Creating new contact group '%1'", collection.displayName()));
    qCDebug(GOOGLE_LOG) << "Adding contact group" << collection.displayName();
    ContactsGroupPtr group(new ContactsGroup);
    group->setTitle(collection.name());
    group->setIsSystemGroup(false);

    auto job = new ContactsGroupCreateJob(group, m_resource->account(), this);
    connect(job, &ContactsGroupCreateJob::finished, [this, &collection](KGAPI2::Job* job){
            if (!m_resource->handleError(job)) {
                return;
            }

            ContactsGroupPtr group = qobject_cast<ContactsGroupCreateJob *>(job)->items().first().dynamicCast<ContactsGroup>();
            qCDebug(GOOGLE_LOG) << "Contact group created:" << group->id();
            Collection newCollection = collection;
            newCollection.setRemoteId(group->id());
            newCollection.setContentMimeTypes(QStringList() << KContacts::Addressee::mimeType());

            auto attr = newCollection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
            attr->setDisplayName(group->title());
            attr->setIconName(QStringLiteral("view-pim-contacts"));

            m_collections[ newCollection.remoteId() ] = newCollection;
            m_resource->changeCommitted(newCollection);
            emitReadyStatus();
        });
}

void ContactHandler::collectionChanged(const Collection &collection)
{
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Changing contact group '%1'", collection.displayName()));
    qCDebug(GOOGLE_LOG) << "Changing contact group" << collection.remoteId();

    ContactsGroupPtr group(new ContactsGroup());
    group->setId(collection.remoteId());
    group->setTitle(collection.displayName());

    auto job = new ContactsGroupModifyJob(group, m_resource->account(), this);
    job->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(job, &ContactsGroupModifyJob::finished, m_resource, &GoogleResource::slotGenericJobFinished);
}

void ContactHandler::collectionRemoved(const Collection &collection)
{
    Q_EMIT status(AgentBase::Running, i18nc("@info:status", "Removing contact group '%1'", collection.displayName()));
    qCDebug(GOOGLE_LOG) << "Removing contact group" << collection.remoteId();
    auto job = new ContactsGroupDeleteJob(collection.remoteId(), m_resource->account(), this);
    job->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(job, &ContactsGroupDeleteJob::finished, m_resource, &GoogleResource::slotGenericJobFinished);
}
