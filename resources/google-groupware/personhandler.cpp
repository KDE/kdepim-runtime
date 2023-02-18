/*
    SPDX-FileCopyrightText: 2011-2013 Daniel Vrátil <dvratil@redhat.com>
    SPDX-FileCopyrightText: 2020 Igor Pobiko <igor.poboiko@gmail.com>
    SPDX-FileCopyrightText: 2022-2023 Claudio Cambra <claudio.cambra@kde.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "personhandler.h"
#include "peopleconversionjob.h"
#include "googleresource.h"
#include "googlesettings.h"

#include "googlepeople_debug.h"

#include <Akonadi/CollectionModifyJob>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/LinkJob>
#include <Akonadi/VectorHelper>

#include <KContacts/Addressee>
#include <KContacts/Picture>

#include <kgapicore_export.h>

#include <KGAPI/Account>
#include <KGAPI/People/Membership>
#include <KGAPI/People/Person>
#include <KGAPI/People/PersonMetadata>
#include <KGAPI/People/PersonCreateJob>
#include <KGAPI/People/PersonDeleteJob>
#include <KGAPI/People/PersonFetchJob>
//#include <KGAPI/Contacts/ContactFetchPhotoJob>
#include <KGAPI/People/PersonModifyJob>
#include <KGAPI/People/ContactGroup>
#include <KGAPI/People/ContactGroupMembership>
//#include <KGAPI/Contacts/ContactsGroupCreateJob>
//#include <KGAPI/Contacts/ContactsGroupDeleteJob>
#include <KGAPI/People/ContactGroupFetchJob>
//#include <KGAPI/Contacts/ContactsGroupModifyJob>


using namespace KGAPI2;
using namespace Akonadi;

namespace {
    constexpr auto myContactsResourceName = "contactGroups/myContacts";
    constexpr auto otherContactsResourceName = "contactGroups/otherContacts";
}

QString PersonHandler::mimeType()
{
    return addresseeMimeType();
}

QString PersonHandler::addresseeMimeType()
{
    return KContacts::Addressee::mimeType();
}

bool PersonHandler::canPerformTask(const Item &item)
{
    return GenericHandler::canPerformTask<KContacts::Addressee>(item);
}

bool PersonHandler::canPerformTask(const Item::List &items)
{
    return GenericHandler::canPerformTask<KContacts::Addressee>(items);
}

Collection PersonHandler::collectionFromContactGroup(const People::ContactGroupPtr &group)
{
    Collection collection;
    collection.setContentMimeTypes({addresseeMimeType()});
    collection.setName(group->name());
    collection.setRemoteId(group->resourceName());
    collection.setRemoteRevision(group->etag());

    const auto isSystemGroup = group->groupType() == People::ContactGroup::GroupType::SYSTEM_CONTACT_GROUP;
    auto realName = group->formattedName();
    if (isSystemGroup) {
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
    if (group->resourceName() == QString::fromUtf8(myContactsResourceName)) {
        collection.setRights(Collection::CanCreateItem | Collection::CanChangeItem | Collection::CanDeleteItem);
    } else {
        collection.setRights(Collection::CanLinkItem | Collection::CanUnlinkItem | Collection::CanChangeItem);
        collection.setVirtual(true);
        if (!isSystemGroup) {
            collection.setRights(collection.rights() | Collection::CanChangeCollection | Collection::CanDeleteCollection);
        }
    }

    auto attr = collection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
    attr->setDisplayName(realName);
    attr->setIconName(QStringLiteral("view-pim-contacts"));

    return collection;
}

void PersonHandler::retrieveCollections(const Collection &rootCollection)
{
    m_iface->emitStatus(AgentBase::Running, i18nc("@info:status", "Retrieving contacts groups"));
    qCDebug(GOOGLE_PEOPLE_LOG) << "Retrieving contacts groups...";
    m_collections.clear();

    // Set up Google's special "Other contacts" contacts group
    Collection otherCollection;
    otherCollection.setContentMimeTypes({mimeType()});
    otherCollection.setName(i18n("Other Contacts"));
    otherCollection.setParentCollection(rootCollection);
    otherCollection.setRights(Collection::CanCreateItem | Collection::CanChangeItem | Collection::CanDeleteItem);
    otherCollection.setRemoteId(QString::fromUtf8(otherContactsResourceName));

    auto attr = otherCollection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
    attr->setDisplayName(i18n("Other Contacts"));
    attr->setIconName(QStringLiteral("view-pim-contacts"));

    m_iface->collectionsRetrieved({otherCollection});
    m_collections[QString::fromUtf8(otherContactsResourceName)] = otherCollection;

    auto job = new People::ContactGroupFetchJob(m_settings->accountPtr(), this);
    connect(job, &People::ContactGroupFetchJob::finished, this, [this, rootCollection](KGAPI2::Job *job) {
        if (!m_iface->handleError(job)) {
            return;
        }

        const auto objects = qobject_cast<People::ContactGroupFetchJob *>(job)->items();
        qCDebug(GOOGLE_PEOPLE_LOG) << objects.count() << "contact groups retrieved";

        Collection::List collections;
        collections.reserve(objects.count());

        std::transform(objects.cbegin(), objects.cend(), std::back_inserter(collections), [this, &rootCollection](const ObjectPtr &object) {
            const auto group = object.dynamicCast<People::ContactGroup>();
            qCDebug(GOOGLE_PEOPLE_LOG) << " -" << group->formattedName() << "(" << group->resourceName() << ")";

            auto collection = collectionFromContactGroup(group);
            collection.setParentCollection(rootCollection);

            m_collections[collection.remoteId()] = collection;
            return collection;
        });
        m_iface->collectionsRetrievedFromHandler(collections);
    });
}

void PersonHandler::retrieveItems(const Collection &collection)
{
    // Contacts are stored inside "My Contacts" and "Other Contacts" only
    if ((collection.remoteId() != QString::fromUtf8(otherContactsResourceName)) &&
        (collection.remoteId() != QString::fromUtf8(myContactsResourceName))) {
        m_iface->itemsRetrievalDone();
        return;
    }

    m_iface->emitStatus(AgentBase::Running, i18nc("@info:status", "Retrieving contacts for group '%1'", collection.displayName()));
    qCDebug(GOOGLE_PEOPLE_LOG) << "Retrieving contacts for group" << collection.remoteId() << "...";

    const auto job = new People::PersonFetchJob(m_settings->accountPtr(), this);

    if (!collection.remoteRevision().isEmpty()) {
        job->setSyncToken(collection.remoteRevision());
    }

    connect(job, &People::PersonFetchJob::finished, this, &PersonHandler::slotItemsRetrieved);
}

void PersonHandler::slotItemsRetrieved(KGAPI2::Job *job)
{
    if (!m_iface->handleError(job)) {
        return;
    }

    auto collection = m_iface->currentCollection();

    Item::List changedItems, removedItems;
    QHash<QString, Item::List> groupsMap;

    auto fetchJob = qobject_cast<People::PersonFetchJob *>(job);
    const auto isIncremental = !fetchJob->syncToken().isEmpty();
    const auto objects = fetchJob->items();
    qCDebug(GOOGLE_PEOPLE_LOG) << "Retrieved" << objects.count() << "contacts";

    for (const ObjectPtr &object : objects) {
        const auto person = object.dynamicCast<People::Person>();
        const auto addressee = person->toKContactsAddressee();

        Item item;
        item.setMimeType(mimeType());
        item.setParentCollection(collection);
        item.setRemoteId(person->resourceName());
        item.setRemoteRevision(person->etag());
        item.setPayload<KContacts::Addressee>(addressee);

        if (person->metadata().deleted() ||
            (collection.remoteId() == QString::fromUtf8(otherContactsResourceName) && !person->memberships().isEmpty()) ||
            (collection.remoteId() == QString::fromUtf8(myContactsResourceName) && person->memberships().isEmpty())) {
            qCDebug(GOOGLE_PEOPLE_LOG) << " - removed" << person->resourceName();
            removedItems << item;
        } else {
            qCDebug(GOOGLE_PEOPLE_LOG) << " - changed" << person->resourceName();
            changedItems << item;
        }

        const auto memberships = person->memberships();
        for (const auto &membership : memberships) {
            // We don't link contacts to "My Contacts"
            const auto contactGroupResourceName = membership.contactGroupMembership().contactGroupResourceName();
            if (contactGroupResourceName != QString::fromUtf8(myContactsResourceName)) {
                groupsMap[contactGroupResourceName] << item;
            }
        }
    }

    if (isIncremental) {
        m_iface->itemsRetrievedIncremental(changedItems, removedItems);
    } else {
        m_iface->itemsRetrieved(changedItems);
    }

    for (auto iter = groupsMap.constBegin(); iter != groupsMap.constEnd(); ++iter) {
        new LinkJob(m_collections[iter.key()], iter.value(), this);
    }
    // TODO: unlink if the group was removed!

    collection.setRemoteRevision(fetchJob->receivedSyncToken());
    new CollectionModifyJob(collection, this);

    emitReadyStatus();
}

void PersonHandler::slotPersonCreateJobFinished(KGAPI2::Job *job)
{
    if (!m_iface->handleError(job)) {
        return;
    }

    const auto personCreateJob = qobject_cast<People::PersonCreateJob *>(job);
    const auto createdPeople = personCreateJob->items();

    processUpdatedPeople(job, createdPeople);
}

void PersonHandler::slotPersonModifyJobFinished(KGAPI2::Job *job)
{
    if (!m_iface->handleError(job)) {
        return;
    }

    const auto personModifyJob = qobject_cast<People::PersonModifyJob *>(job);
    const auto modifiedPeople = personModifyJob->items();

    processUpdatedPeople(job, modifiedPeople);
}

void PersonHandler::processUpdatedPeople(KGAPI2::Job *job, const ObjectsList &updatedPeople)
{
    Q_ASSERT(job);
    if (updatedPeople.isEmpty()) {
        return;
    }

    if (job->property(ITEM_PROPERTY).canConvert<Akonadi::Item>()) {
        const auto originalItem = job->property(ITEM_PROPERTY).value<Akonadi::Item>();
        if (!originalItem.isValid()) {
            qCWarning(GOOGLE_PEOPLE_LOG) << "No valid item in received KGAPI job, can't update";
            return;
        }

        const auto person = updatedPeople.first().dynamicCast<People::Person>();
        updatePersonItem(originalItem, person);

    } else if (job->property(ITEMS_PROPERTY).canConvert<Akonadi::Item::List>()) {
        // Be careful not to send an item list or a multi person create job -- only modify jobs.
        // At point of creation we do not yet have resource names for the Akonadi items for newly created people.
        // This means we will not know which Akonadi items correspond to which person.
        const auto originalItems = job->property(ITEMS_PROPERTY).value<Akonadi::Item::List>();
        if (originalItems.isEmpty()) {
            qCWarning(GOOGLE_PEOPLE_LOG) << "No items in items vector in received KGAPI job, can't update";
            return;
        }

        for (const auto &personObject : updatedPeople) {
            const auto person = personObject.dynamicCast<People::Person>();
            const auto matchingItemIt = std::find_if(originalItems.cbegin(), originalItems.cend(), [&person](const Akonadi::Item &item) {
                return item.remoteId() == person->resourceName();
            });

            if (matchingItemIt == originalItems.cend()) {
                qCWarning(GOOGLE_PEOPLE_LOG) << "Could not find matching item for person:" << person->resourceName()
                                             << "cannot update them properly right now.";
                continue;
            }

            const auto matchingItem = *matchingItemIt;
            updatePersonItem(matchingItem, person);
        }
    } else {
        qCWarning(GOOGLE_PEOPLE_LOG) << "Finished job not carrying actionable item property, cannot update.";
    }
}

void PersonHandler::updatePersonItem(const Akonadi::Item &originalItem, const People::PersonPtr &person)
{
    if (person.isNull()) {
        qCWarning(GOOGLE_PEOPLE_LOG) << "Received null person ptr, can't update";
        return;
    } else if (!originalItem.isValid()) {
        qCWarning(GOOGLE_PEOPLE_LOG) << "No valid item in received KGAPI job, can't update";
        return;
    }

    Item newItem = originalItem;
    qCDebug(GOOGLE_PEOPLE_LOG) << "Person" << person->resourceName() << "created";

    newItem.setRemoteId(person->resourceName());
    newItem.setRemoteRevision(person->etag());
    m_iface->itemChangeCommitted(newItem);

    newItem.setPayload<KContacts::Addressee>(person->toKContactsAddressee());
    new ItemModifyJob(newItem, this);
    emitReadyStatus();
}

void PersonHandler::itemAdded(const Item &item, const Collection &collection)
{
    m_iface->emitStatus(AgentBase::Running, i18nc("@info:status", "Adding contact to group '%1'", collection.displayName()));
    const auto addressee = item.payload<KContacts::Addressee>();
    const auto person = People::Person::fromKContactsAddressee(addressee);

    qCDebug(GOOGLE_PEOPLE_LOG) << "Creating people";

    People::ContactGroupMembership contactGroupMembership;
    contactGroupMembership.setContactGroupResourceName(QString::fromUtf8(myContactsResourceName));

    People::Membership membership;
    membership.setContactGroupMembership(contactGroupMembership);
    person->setMemberships({membership});

    auto job = new People::PersonCreateJob(person, m_settings->accountPtr(), this);
    job->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(job, &People::PersonCreateJob::finished, this, &PersonHandler::slotPersonCreateJobFinished);
}

void PersonHandler::sendModifyJob(const Akonadi::Item::List &items, const People::PersonList &people)
{
    auto job = new People::PersonModifyJob(people, m_settings->accountPtr(), this);
    job->setProperty(ITEMS_PROPERTY, QVariant::fromValue(items));
    connect(job, &People::PersonModifyJob::finished, this, &PersonHandler::slotPersonModifyJobFinished);
}

void PersonHandler::sendModifyJob(const Akonadi::Item &item, const People::PersonPtr &person)
{
    auto job = new People::PersonModifyJob(person, m_settings->accountPtr(), this);
    job->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(job, &People::PersonModifyJob::finished, this, &PersonHandler::slotPersonModifyJobFinished);
}

void PersonHandler::itemChanged(const Item &item, const QSet<QByteArray> & /*partIdentifiers*/)
{
    m_iface->emitStatus(AgentBase::Running, i18nc("@info:status", "Changing contact"));
    qCDebug(GOOGLE_PEOPLE_LOG) << "Changing person" << item.remoteId();

    const auto job = new PeopleConversionJob({item}, this);
    connect(job, &PeopleConversionJob::finished, this, [this, item, job] {
        const auto person = job->people().first();
        sendModifyJob(item, person);
    });
    job->start();
}

void PersonHandler::itemsRemoved(const Item::List &items)
{
    m_iface->emitStatus(AgentBase::Running, i18ncp("@info:status", "Removing %1 contact", "Removing %1 contacts", items.count()));
    QStringList peopleResourceNames;
    peopleResourceNames.reserve(items.count());
    std::transform(items.cbegin(), items.cend(), std::back_inserter(peopleResourceNames), [](const Item &item) {
        return item.remoteId();
    });
    qCInfo(GOOGLE_PEOPLE_LOG) << "Removing people" << peopleResourceNames;
    auto job = new People::PersonDeleteJob(peopleResourceNames, m_settings->accountPtr(), this);
    job->setProperty(ITEMS_PROPERTY, QVariant::fromValue(items));
    connect(job, &People::PersonDeleteJob::finished, this, &PersonHandler::slotGenericJobFinished);
}

void PersonHandler::itemsMoved(const Item::List &items, const Collection &collectionSource, const Collection &collectionDestination)
{
    const auto sourceRemoteId = collectionSource.remoteId();
    const auto destinationRemoteId = collectionDestination.remoteId();
    qCDebug(GOOGLE_PEOPLE_LOG) << "Moving people from" << sourceRemoteId << "to" << destinationRemoteId;

    if (!(((sourceRemoteId == QString::fromUtf8(myContactsResourceName)) && (destinationRemoteId == QString::fromUtf8(otherContactsResourceName))) ||
          ((sourceRemoteId == QString::fromUtf8(otherContactsResourceName)) && (destinationRemoteId == QString::fromUtf8(myContactsResourceName))))) {
        m_iface->cancelTask(i18n("Invalid source or destination collection"));
        return;
    }

    m_iface->emitStatus(AgentBase::Running,
                        i18ncp("@info:status",
                               "Moving %1 contact from group '%2' to '%3'",
                               "Moving %1 contacts from group '%2' to '%3'",
                               items.count(),
                               sourceRemoteId,
                               destinationRemoteId));

    const auto job = new PeopleConversionJob(items, this);
    job->setReparentCollectionRemoteId(destinationRemoteId);
    connect(job, &PeopleConversionJob::finished, this, [this, items, job] {
        sendModifyJob(items, job->people());
    });
    job->start();
}

void PersonHandler::itemsLinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection)
{
    m_iface->emitStatus(AgentBase::Running, i18ncp("@info:status", "Linking %1 contact", "Linking %1 contacts", items.count()));
    qCDebug(GOOGLE_PEOPLE_LOG) << "Linking" << items.count() << "contacts to group" << collection.remoteId();

    const auto job = new PeopleConversionJob(items, this);
    job->setNewLinkedCollectionRemoteId(collection.remoteId());
    connect(job, &PeopleConversionJob::finished, this, [this, items, job] {
        sendModifyJob(items, job->people());
    });
    job->start();
}
