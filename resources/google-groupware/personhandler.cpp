/*
    SPDX-FileCopyrightText: 2011-2013 Daniel Vrátil <dvratil@redhat.com>
    SPDX-FileCopyrightText: 2020 Igor Pobiko <igor.poboiko@gmail.com>
    SPDX-FileCopyrightText: 2022-2023 Claudio Cambra <claudio.cambra@kde.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "personhandler.h"
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
//#include <KGAPI/People/PersonCreateJob>
//#include <KGAPI/Contacts/ContactDeleteJob>
#include <KGAPI/People/PersonFetchJob>
//#include <KGAPI/Contacts/ContactFetchPhotoJob>
//#include <KGAPI/Contacts/ContactModifyJob>
#include <KGAPI/People/ContactGroup>
#include <KGAPI/People/ContactGroupMembership>
//#include <KGAPI/Contacts/ContactsGroupCreateJob>
//#include <KGAPI/Contacts/ContactsGroupDeleteJob>
#include <KGAPI/People/ContactGroupFetchJob>
//#include <KGAPI/Contacts/ContactsGroupModifyJob>

namespace {
    constexpr auto myContactsResourceName = "contactGroups/myContacts";
    constexpr auto otherContactsResourceName = "otherContacts";
}

using namespace KGAPI2;
using namespace Akonadi;

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
