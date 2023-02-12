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
