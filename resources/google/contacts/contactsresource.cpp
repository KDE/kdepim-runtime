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
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "contactsresource.h"
#include "settingsdialog.h"
#include "settings.h"

#include <AkonadiCore/CollectionModifyJob>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/ItemFetchJob>
#include <AkonadiCore/ItemFetchScope>
#include <AkonadiCore/ItemModifyJob>
#include <AkonadiCore/LinkJob>
#include <AkonadiCore/UnlinkJob>
#include <AkonadiCore/CachePolicy>
#include <AkonadiCore/VectorHelper>

#include <KContacts/Addressee>
#include <KContacts/Picture>

#include <KLocalizedString>
#include <QIcon>


#include <KGAPI/Types>
#include <KGAPI/Account>
#include <KGAPI/AuthJob>
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

#define MYCONTACTS_REMOTEID QStringLiteral("MyContacts")
#define OTHERCONTACTS_REMOTEID QStringLiteral("OtherContacts")

Q_DECLARE_METATYPE(KGAPI2::Job *)
Q_DECLARE_METATYPE(QList<QString>)

using namespace Akonadi;
using namespace KGAPI2;

ContactsResource::ContactsResource(const QString &id)
    : GoogleResource(id)
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

int ContactsResource::runConfigurationDialog(WId windowId)
{
    QScopedPointer<SettingsDialog> settingsDialog(new SettingsDialog(accountManager(), windowId, this));
    settingsDialog->setWindowIcon(QIcon::fromTheme(QStringLiteral("im-google")));

    return settingsDialog->exec();
}

void ContactsResource::updateResourceName()
{
    const QString accountName = Settings::self()->account();
    setName(i18nc("%1 is account name (user@gmail.com)", "Google Contacts (%1)", accountName.isEmpty() ? i18n("not configured") : accountName));
}

QList< QUrl > ContactsResource::scopes() const
{
    const QList< QUrl > scopes = {Account::contactsScopeUrl(), Account::accountInfoScopeUrl()};
    return scopes;
}

void ContactsResource::retrieveItems(const Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }

    // All items are only in top-level collection and Other Contacts collection
    if ((collection.remoteId() != m_rootCollection.remoteId())
        && (collection.remoteId() != OTHERCONTACTS_REMOTEID)) {
        itemsRetrievalDone();
        return;
    }

    ContactFetchJob *fetchJob = new ContactFetchJob(account(), this);
    fetchJob->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    fetchJob->setFetchDeleted(true);
    if (!collection.remoteRevision().isEmpty()) {
        fetchJob->setFetchOnlyUpdated(collection.remoteRevision().toLongLong());
    }
    connect(fetchJob, &ContactFetchJob::progress, this, &ContactsResource::emitPercent);
    connect(fetchJob, &ContactFetchJob::finished, this, &ContactsResource::slotItemsRetrieved);
}

void ContactsResource::retrieveContactsPhotos(const QVariant &arguments)
{
    if (!canPerformTask()) {
        return;
    }

    const QVariantMap map = arguments.toMap();
    const Collection collection = map[ QStringLiteral("collection") ].value<Collection>();
    ItemFetchJob *itemFetchJob = new ItemFetchJob(collection, this);
    itemFetchJob->setProperty("modifiedItems", map[ QStringLiteral("modifiedItems") ]);
    itemFetchJob->fetchScope().fetchFullPayload(true);
    connect(itemFetchJob, &ItemFetchJob::finished, this, &ContactsResource::slotUpdatePhotosItemsRetrieved);
    Q_EMIT status(Running, i18nc("@info:status", "Retrieving photos"));
}

void ContactsResource::retrieveCollections()
{
    if (!canPerformTask()) {
        return;
    }

    ContactsGroupFetchJob *fetchJob = new ContactsGroupFetchJob(account(), this);
    connect(fetchJob, &ContactFetchJob::progress, this, &ContactsResource::emitPercent);
    connect(fetchJob, &ContactFetchJob::finished, this, &ContactsResource::slotCollectionsRetrieved);
}

void ContactsResource::itemAdded(const Item &item, const Collection &collection)
{
    if (!canPerformTask<KContacts::Addressee>(item, KContacts::Addressee::mimeType())) {
        return;
    }

    KContacts::Addressee addressee = item.payload< KContacts::Addressee >();
    ContactPtr contact(new Contact(addressee));

    /* If the contact has been moved into My Contacts group then modify the membership */
    if (collection.remoteId() == MYCONTACTS_REMOTEID) {
        contact->addGroup(QStringLiteral("http://www.google.com/m8/feeds/groups/%1/base/6").arg(QString::fromLatin1(QUrl::toPercentEncoding(account()->accountName()))));
    }

    /* If the contact has been moved to Other Contacts then remove all groups */
    if (collection.remoteId() == OTHERCONTACTS_REMOTEID) {
        contact->clearGroups();
    }

    ContactCreateJob *createJob = new ContactCreateJob(contact, account(), this);
    createJob->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(createJob, &ContactCreateJob::progress, this, &ContactsResource::emitPercent);
    connect(createJob, &ContactCreateJob::finished, this, &ContactsResource::slotCreateJobFinished);
}

void ContactsResource::itemChanged(const Item &item, const QSet< QByteArray > &partIdentifiers)
{
    Q_UNUSED(partIdentifiers);

    if (!canPerformTask<KContacts::Addressee>(item, KContacts::Addressee::mimeType())) {
        return;
    }

    KContacts::Addressee addressee = item.payload< KContacts::Addressee >();
    ContactPtr contact(new Contact(addressee));

    if (item.parentCollection().remoteId() == MYCONTACTS_REMOTEID) {
        contact->addGroup(QStringLiteral("http://www.google.com/m8/feeds/groups/%1/base/6").arg(QString::fromLatin1(QUrl::toPercentEncoding(account()->accountName()))));
    }

    ContactModifyJob *modifyJob = new ContactModifyJob(contact, account(), this);
    modifyJob->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(modifyJob, &ContactModifyJob::progress, this, &ContactsResource::emitPercent);
    connect(modifyJob, &ContactModifyJob::finished, this, &ContactsResource::slotGenericJobFinished);
}

void ContactsResource::itemMoved(const Item &item, const Collection &collectionSource, const Collection &collectionDestination)
{
    if (!canPerformTask<KContacts::Addressee>(item, KContacts::Addressee::mimeType())) {
        return;
    }

    KContacts::Addressee addressee = item.payload< KContacts::Addressee >();
    ContactPtr contact(new Contact(addressee));

    // MyContacts -> OtherContacts
    if (collectionSource.remoteId() == MYCONTACTS_REMOTEID
        && collectionDestination.remoteId() == OTHERCONTACTS_REMOTEID) {
        contact->removeGroup(QStringLiteral("http://www.google.com/m8/feeds/groups/%1/base/6").arg(QString::fromLatin1(QUrl::toPercentEncoding(account()->accountName()))));

        // OtherContacts -> MyContacts
    } else if (collectionSource.remoteId() == OTHERCONTACTS_REMOTEID
               && collectionDestination.remoteId() == MYCONTACTS_REMOTEID) {
        contact->addGroup(QStringLiteral("http://www.google.com/m8/feeds/groups/%1/base/6").arg(QString::fromLatin1(QUrl::toPercentEncoding(account()->accountName()))));
    } else {
        cancelTask(i18n("Invalid source or destination collection"));
        return;
    }

    ContactModifyJob *modifyJob = new ContactModifyJob(contact, account(), this);
    modifyJob->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(modifyJob, &ContactModifyJob::progress, this, &ContactsResource::emitPercent);
    connect(modifyJob, &ContactModifyJob::finished, this, &ContactsResource::slotGenericJobFinished);
}

void ContactsResource::itemRemoved(const Item &item)
{
    if (!canPerformTask()) {
        return;
    }

    ContactDeleteJob *deleteJob = new ContactDeleteJob(item.remoteId(), account(), this);
    deleteJob->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(deleteJob, &ContactDeleteJob::progress, this, &ContactsResource::emitPercent);
    connect(deleteJob, &ContactDeleteJob::finished, this, &ContactsResource::slotGenericJobFinished);

    Q_EMIT status(Running, i18nc("@info:status", "Removing contact"));
}

void ContactsResource::itemLinked(const Item &item, const Collection &collection)
{
    if (!canPerformTask<KContacts::Addressee>(item, KContacts::Addressee::mimeType())) {
        return;
    }

    KContacts::Addressee addressee = item.payload<KContacts::Addressee>();
    ContactPtr contact(new Contact(addressee));

    contact->addGroup(collection.remoteId());

    ContactModifyJob *modifyJob = new ContactModifyJob(contact, account(), this);
    connect(modifyJob, &ContactModifyJob::finished, this, &ContactsResource::slotGenericJobFinished);
}

void ContactsResource::itemUnlinked(const Item &item, const Collection &collection)
{
    if (!canPerformTask<KContacts::Addressee>(item, KContacts::Addressee::mimeType())) {
        return;
    }

    KContacts::Addressee addressee = item.payload<KContacts::Addressee>();
    ContactPtr contact(new Contact(addressee));

    contact->removeGroup(collection.remoteId());

    ContactModifyJob *modifyJob = new ContactModifyJob(contact, account(), this);
    modifyJob->setProperty(ITEM_PROPERTY, QVariant::fromValue(item));
    connect(modifyJob, &ContactModifyJob::finished, this, &ContactsResource::slotGenericJobFinished);
}

void ContactsResource::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    Q_UNUSED(parent);

    if (!canPerformTask()) {
        return;
    }

    ContactsGroupPtr group(new ContactsGroup);
    group->setTitle(collection.name());
    group->setIsSystemGroup(false);

    ContactsGroupCreateJob *createJob = new ContactsGroupCreateJob(group, account(), this);
    createJob->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(createJob, &ContactsGroupCreateJob::finished, this, &ContactsResource::slotCreateJobFinished);
}

void ContactsResource::collectionChanged(const Akonadi::Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }

    ContactsGroupPtr group(new ContactsGroup());
    group->setId(collection.remoteId());

    group->setTitle(collection.displayName());

    ContactsGroupModifyJob *modifyJob = new ContactsGroupModifyJob(group, account(), this);
    modifyJob->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(modifyJob, &ContactsGroupModifyJob::finished, this, &ContactsResource::slotGenericJobFinished);
}

void ContactsResource::collectionRemoved(const Akonadi::Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }

    ContactsGroupDeleteJob *deleteJob = new ContactsGroupDeleteJob(collection.remoteId(), account(), this);
    deleteJob->setProperty(COLLECTION_PROPERTY, QVariant::fromValue(collection));
    connect(deleteJob, &ContactsGroupDeleteJob::finished, this, &ContactsResource::slotGenericJobFinished);
}

void ContactsResource::slotCollectionsRetrieved(KGAPI2::Job *job)
{
    if (!handleError(job)) {
        return;
    }

    ContactsGroupFetchJob *fetchJob = qobject_cast<ContactsGroupFetchJob *>(job);
    const ObjectsList objects = fetchJob->items();

    CachePolicy cachePolicy;
    if (Settings::self()->enableIntervalCheck()) {
        cachePolicy.setInheritFromParent(false);
        cachePolicy.setIntervalCheckTime(Settings::self()->intervalCheckTime());
    }

    m_rootCollection = Collection();
    m_rootCollection.setContentMimeTypes({Collection::virtualMimeType(), Collection::mimeType(), KContacts::Addressee::mimeType()});
    m_rootCollection.setRemoteId(MYCONTACTS_REMOTEID);
    m_rootCollection.setName(fetchJob->account()->accountName());
    m_rootCollection.setParentCollection(Collection::root());
    m_rootCollection.setCachePolicy(cachePolicy);
    m_rootCollection.setRights(Collection::CanCreateItem
                               |Collection::CanChangeItem
                               |Collection::CanDeleteItem);

    EntityDisplayAttribute *attr = m_rootCollection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
    attr->setDisplayName(fetchJob->account()->accountName());
    attr->setIconName(QStringLiteral("im-google"));

    m_collections[ MYCONTACTS_REMOTEID ] = m_rootCollection;

    for (const ObjectPtr &object : qAsConst(objects)) {
        const ContactsGroupPtr group = object.dynamicCast<ContactsGroup>();

        QString realName = group->title();

        if (group->isSystemGroup()) {
            if (group->title().contains(QLatin1String("Coworkers"))) {
                realName = i18nc("Name of a group of contacts", "Coworkers");
            } else if (group->title().contains(QLatin1String("Friends"))) {
                realName = i18nc("Name of a group of contacts", "Friends");
            } else if (group->title().contains(QLatin1String("Family"))) {
                realName = i18nc("Name of a group of contacts", "Family");
            } else if (group->title().contains(QLatin1String("My Contacts"))) {
                // Yes, skip My Contacts group, we store "My Contacts" in root collection
                continue;
            }
        } else {
            if (group->title().contains(QLatin1String("Other Contacts"))) {
                realName = i18nc("Name of a group of contacts", "Other Contacts");
            }
        }

        Collection collection;
        collection.setContentMimeTypes(QStringList() << KContacts::Addressee::mimeType());
        collection.setName(group->id());
        collection.setParentCollection(m_rootCollection);
        collection.setRights(Collection::CanLinkItem
                             |Collection::CanUnlinkItem
                             |Collection::CanChangeItem);
        if (!group->isSystemGroup()) {
            collection.setRights(collection.rights()
                                 |Collection::CanChangeCollection
                                 |Collection::CanDeleteCollection);
        }
        collection.setRemoteId(group->id());
        collection.setVirtual(true);

        EntityDisplayAttribute *attr = collection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
        attr->setDisplayName(realName);
        attr->setIconName(QStringLiteral("view-pim-contacts"));

        m_collections[ collection.remoteId() ] = collection;
    }

    Collection otherCollection;
    otherCollection.setContentMimeTypes(QStringList() << KContacts::Addressee::mimeType());
    otherCollection.setName(i18n("Other Contacts"));
    otherCollection.setParentCollection(m_rootCollection);
    otherCollection.setRights(Collection::CanCreateItem
                              |Collection::CanChangeItem
                              |Collection::CanDeleteItem);
    otherCollection.setRemoteId(OTHERCONTACTS_REMOTEID);

    attr = otherCollection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
    attr->setDisplayName(i18n("Other Contacts"));
    attr->setIconName(QStringLiteral("view-pim-contacts"));
    m_collections[ OTHERCONTACTS_REMOTEID ] = otherCollection;

    collectionsRetrieved(Akonadi::valuesToVector(m_collections));
    job->deleteLater();
}

void ContactsResource::slotItemsRetrieved(KGAPI2::Job *job)
{
    if (!handleError(job)) {
        return;
    }

    ContactFetchJob *fetchJob = qobject_cast<ContactFetchJob *>(job);

    Collection collection = fetchJob->property(COLLECTION_PROPERTY).value<Collection>();

    Item::List changedItems, removedItems;
    QMap<QString, Item::List> groupsMap;
    QList<QString> changedPhotos;
    const ObjectsList objects = fetchJob->items();
    for (const ObjectPtr &object : objects) {
        const ContactPtr contact = object.dynamicCast<Contact>();

        if (((collection.remoteId() == OTHERCONTACTS_REMOTEID) && !contact->groups().isEmpty())
            || ((collection.remoteId() == MYCONTACTS_REMOTEID) && contact->groups().isEmpty())) {
            continue;
        }

        Item item;
        item.setMimeType(KContacts::Addressee::mimeType());
        item.setParentCollection(m_collections[MYCONTACTS_REMOTEID]);
        item.setRemoteId(contact->uid());
        item.setRemoteRevision(contact->etag());
        item.setPayload<KContacts::Addressee>(*contact.dynamicCast<KContacts::Addressee>());

        if (contact->deleted()) {
            removedItems << item;
        } else {
            changedItems << item;
            changedPhotos << contact->uid();
        }

        const QStringList groups = contact->groups();
        for (const QString &group : groups) {
            groupsMap[group] << item;
        }
    }

    itemsRetrievedIncremental(changedItems, removedItems);

    for (QMap<QString, Item::List>::ConstIterator iter = groupsMap.constBegin(), iterEnd = groupsMap.constEnd(); iter != iterEnd; ++iter) {
        new LinkJob(m_collections[iter.key()], iter.value(), this);
    }

    QVariantMap map;
    map[QStringLiteral("collection")] = QVariant::fromValue(collection);
    map[QStringLiteral("modifiedItems")] = QVariant::fromValue(changedPhotos);
    scheduleCustomTask(this, "retrieveContactsPhotos", map);

    const QDateTime local(QDateTime::currentDateTime());
    const QDateTime UTC(local.toUTC());

    collection.setRemoteRevision(QString::number(UTC.toTime_t()));
    new CollectionModifyJob(collection, this);

    job->deleteLater();
}

void ContactsResource::slotUpdatePhotosItemsRetrieved(KJob *job)
{
    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob *>(job);
    const QList<QString> modifiedItems = fetchJob->property("modifiedItems").value< QList<QString> >();
    ContactsList contacts;

    const Item::List items = fetchJob->items();
    for (const Item &item : items) {
        if (modifiedItems.contains(item.remoteId())) {
            const KContacts::Addressee addressee = item.payload<KContacts::Addressee>();
            const ContactPtr contact(new Contact(addressee));
            contacts << contact;
        }
    }

    // Make sure account is still valid
    if (!canPerformTask()) {
        return;
    }

    ContactFetchPhotoJob *photoJob = new ContactFetchPhotoJob(contacts, account(), this);
    photoJob->setProperty(ITEMLIST_PROPERTY, QVariant::fromValue(items));
    photoJob->setProperty("processedItems", 0);
    connect(photoJob, &ContactFetchPhotoJob::photoFetched, this, &ContactsResource::slotUpdatePhotoFinished);
    connect(photoJob, &ContactFetchPhotoJob::finished, this, &ContactsResource::slotGenericJobFinished);
}

void ContactsResource::slotUpdatePhotoFinished(KGAPI2::Job *job, const ContactPtr &contact)
{
    Item::List items = job->property(ITEMLIST_PROPERTY).value<Item::List>();

    int processedItems = job->property("processedItems").toInt();
    processedItems++;
    job->setProperty("processedItems", processedItems);
    emitPercent(job, processedItems, items.count());

    for (Item item : qAsConst(items)) {
        if (item.remoteId() == contact->uid()) {
            item.setPayload<KContacts::Addressee>(*contact.dynamicCast<KContacts::Addressee>());
            new ItemModifyJob(item, this);
            return;
        }
    }
}

void ContactsResource::slotCreateJobFinished(KGAPI2::Job *job)
{
    if (!handleError(job)) {
        return;
    }

    Item item = job->property(ITEM_PROPERTY).value<Item>();
    Collection collection = job->property(COLLECTION_PROPERTY).value<Collection>();
    if (item.isValid()) {
        ContactCreateJob *createJob = qobject_cast<ContactCreateJob *>(job);
        Q_ASSERT(createJob->items().count() == 1);
        ContactPtr contact = createJob->items().at(0).dynamicCast<Contact>();

        item.setRemoteId(contact->uid());
        item.setRemoteRevision(contact->etag());
        changeCommitted(item);
    } else if (collection.isValid()) {
        ContactsGroupCreateJob *createJob = qobject_cast<ContactsGroupCreateJob *>(job);
        Q_ASSERT(createJob->items().count() == 1);
        ContactsGroupPtr group = createJob->items().at(0).dynamicCast<ContactsGroup>();

        collection.setRemoteId(group->id());
        collection.setContentMimeTypes(QStringList() << KContacts::Addressee::mimeType());

        EntityDisplayAttribute *attr = collection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
        attr->setDisplayName(group->title());
        attr->setIconName(QStringLiteral("view-pim-contacts"));

        m_collections[ collection.remoteId() ] = collection;

        changeCommitted(collection);
    }

    job->deleteLater();
}

AKONADI_RESOURCE_MAIN(ContactsResource)
