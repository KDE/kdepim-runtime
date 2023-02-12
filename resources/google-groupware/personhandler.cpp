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

constexpr auto myContactsResourceName = "contactGroups/myContacts";

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
