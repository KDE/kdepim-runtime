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
#include "../common/batchjobrunner.h"

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

#include <QPointer>

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
    , Akonadi::AgentBase::ObserverV4()
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
 
    CachePolicy cachePolicy;
    if (Settings::self()->enableIntervalCheck()) {
        cachePolicy.setInheritFromParent(false);
        cachePolicy.setIntervalCheckTime(Settings::self()->intervalCheckTime());
    }

    Collection root;
    root.setContentMimeTypes({ Collection::mimeType(), KContacts::Addressee::mimeType() });
    root.setRemoteId(MYCONTACTS_REMOTEID);
    root.setName(account()->accountName());
    root.setParentCollection(Collection::root());
    root.setCachePolicy(cachePolicy);
    root.setRights(Collection::CanCreateItem | Collection::CanChangeItem | Collection::CanDeleteItem);

    EntityDisplayAttribute *attr = root.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
    attr->setDisplayName(account()->accountName());
    attr->setIconName(QStringLiteral("im-google"));

    Collection other;
    other.setContentMimeTypes(QStringList() << KContacts::Addressee::mimeType());
    other.setName(i18n("Other Contacts"));
    other.setParentCollection(root);
    other.setRights(Collection::CanCreateItem | Collection::CanChangeItem | Collection::CanDeleteItem);
    other.setRemoteId(OTHERCONTACTS_REMOTEID);

    attr = other.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
    attr->setDisplayName(i18n("Other Contacts"));
    attr->setIconName(QStringLiteral("view-pim-contacts"));

    collectionsRetrieved({ root, other });
}

void ContactsResource::retrieveTags()
{
    qDebug() << "Started tag retrieval";
    if (!canPerformTask()) {
        return;
    }

    auto fetch = new ContactsGroupFetchJob(account(), this);
    connect(fetch, &KGAPI2::Job::finished,
            [this](KGAPI2::Job *job) {
                const auto items = qobject_cast<ContactsGroupFetchJob*>(job)->items();
                Tag::List tags;
                for (const auto &item : items) {
                    const auto group = item.dynamicCast<ContactsGroup>();

                    Tag tag;
                    tag.setType(Tag::GENERIC);
                    tag.setRemoteId(group->id().toUtf8());
                    tag.setName(group->title());
                    tag.setGid(group->title().toUtf8());
                    tags.push_back(tag);
                    qDebug() << "Added tag" << group->title() << group->id();
                }

                tagsRetrieved(tags, {});
            });
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
    } else if (collection.remoteId() == OTHERCONTACTS_REMOTEID) {
        /* If the contact has been moved to Other Contacts then remove all groups */
        contact->clearGroups();
    }

    ContactCreateJob *createJob = new ContactCreateJob(contact, account(), this);
    connect(createJob, &ContactCreateJob::progress, this, &ContactsResource::emitPercent);
    connect(createJob, &ContactCreateJob::finished,
            this, [this, item = item](KGAPI2::Job *job) mutable {
                if (!handleError(job)) {
                    return;
                }

                auto createJob = static_cast<ContactCreateJob*>(job);
                Q_ASSERT(createJob->items().size() == 1);
                const auto contact = createJob->items().at(0).dynamicCast<Contact>();

                item.setRemoteId(contact->uid());
                item.setRemoteRevision(contact->etag());

                changeCommitted(item);

                job->deleteLater();
            });
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
#if 0
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
#endif

void ContactsResource::itemsRemoved(const Item::List &items)
{
    if (!canPerformTask()) {
        return;
    }

    QStringList ids;
    ids.reserve(items.size());
    std::transform(items.cbegin(), items.cend(), std::back_inserter(ids),
                   [](const auto &item) { return item.remoteId(); });

    ContactDeleteJob *deleteJob = new ContactDeleteJob(ids, account(), this);
    deleteJob->setProperty(ITEMLIST_PROPERTY, QVariant::fromValue(items));
    connect(deleteJob, &ContactDeleteJob::progress, this, &ContactsResource::emitPercent);
    connect(deleteJob, &ContactDeleteJob::finished, this, &ContactsResource::slotGenericJobFinished);

    Q_EMIT status(Running, i18nc("@info:status", "Removing contact"));
}

void ContactsResource::tagAdded(const Akonadi::Tag &tag)
{
    changeCommitted(tag);
}

void ContactsResource::tagRemoved(const Akonadi::Tag &tag)
{
    changeCommitted(tag);
}


void ContactsResource::itemsTagsChanged(const Item::List &items,
                                        const QSet<Akonadi::Tag> &addedTags,
                                        const QSet<Akonadi::Tag> &removedTags)
{
    if (!canPerformTask<KContacts::Addressee>(items, KContacts::Addressee::mimeType())) {
        return;
    }

    new BatchJobRunner<Item::List>(items,
        [this, addedTags, removedTags](const Item &item) -> KGAPI2::Job * {
            auto addressee = item.payload<KContacts::Addressee>();
            ContactPtr contact(new Contact(addressee));
            for (const auto &tag : addedTags) {
                contact->addGroup(QString::fromUtf8(tag.remoteId()));
            }
            for (const auto &tag : removedTags) {
                contact->removeGroup(QString::fromUtf8(tag.remoteId()));
            }

            return new ContactModifyJob(contact, account(), this);
        },
        [this](const Item::List &items) {
            changesCommitted(items);
        });
}

void ContactsResource::slotItemsRetrieved(KGAPI2::Job *job)
{
    if (!handleError(job)) {
        return;
    }

    ContactFetchJob *fetchJob = qobject_cast<ContactFetchJob *>(job);

    Collection collection = fetchJob->property(COLLECTION_PROPERTY).value<Collection>();

    Item::List changedItems, removedItems;
    QList<QString> changedPhotos;
    const ObjectsList objects = fetchJob->items();
    for (const ObjectPtr &object : objects) {
        const ContactPtr contact = object.dynamicCast<Contact>();

        Item item;
        item.setMimeType(KContacts::Addressee::mimeType());
        item.setParentCollection(collection);
        item.setRemoteId(contact->uid());
        item.setRemoteRevision(contact->etag());
        if (contact->deleted()) {
            removedItems.push_back(item);
            continue;
        }

        item.setPayload<KContacts::Addressee>(*contact.dynamicCast<KContacts::Addressee>());

        const QStringList groups = contact->groups();
        for (const QString &group : groups) {
            Tag tag;
            tag.setType(Tag::GENERIC);
            tag.setRemoteId(group.toUtf8());
            item.setTag(tag);
        }

        changedItems.push_back(item);
    }

    itemsRetrievedIncremental(changedItems, removedItems);

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


AKONADI_RESOURCE_MAIN(ContactsResource)
