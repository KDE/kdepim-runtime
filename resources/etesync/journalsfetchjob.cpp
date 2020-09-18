/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "journalsfetchjob.h"

#include <QtConcurrent>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/CollectionColorAttribute>
#include <kcontacts/addressee.h>
#include <kcontacts/contactgroup.h>
#include <KCalendarCore/Event>
#include <KCalendarCore/Todo>

#include "collectioncacheattribute.h"
#include "etesync_debug.h"

using namespace EteSyncAPI;
using namespace Akonadi;

JournalsFetchJob::JournalsFetchJob(const EtebaseAccount *account, const Akonadi::Collection &resourceCollection, QObject *parent)
    : KJob(parent)
    , mAccount(account)
    , mResourceCollection(resourceCollection)
{
}

void JournalsFetchJob::start()
{
    qCDebug(ETESYNC_LOG) << "Journals fetch start";
    QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);
    connect(watcher, &QFutureWatcher<void>::finished, this, [this] {
        qCDebug(ETESYNC_LOG) << "emitResult from JournalsFetchJob";
        emitResult();
    });
    QFuture<void> fetchJournalsFuture = QtConcurrent::run(this, &JournalsFetchJob::fetchJournals);
    watcher->setFuture(fetchJournalsFuture);
}

void JournalsFetchJob::fetchJournals()
{
    QString stoken;
    bool done = 0;
    EtebaseCollectionManagerPtr collectionManager(etebase_account_get_collection_manager(mAccount));

    while (!done) {
        EtebaseFetchOptionsPtr fetchOptions(etebase_fetch_options_new());
        etebase_fetch_options_set_stoken(fetchOptions.get(), stoken);
        etebase_fetch_options_set_limit(fetchOptions.get(), 30);

        EtebaseCollectionListResponsePtr collectionList(etebase_collection_manager_list(collectionManager.get(), fetchOptions.get()));
        if (!collectionList) {
            setError(int(etebase_error_get_code()));
            const char *err = etebase_error_get_message();
            setErrorText(QString::fromUtf8(err));
            return;
        }

        stoken = QString::fromUtf8(etebase_collection_list_response_get_stoken(collectionList.get()));
        done = etebase_collection_list_response_is_done(collectionList.get());

        uintptr_t dataLength = etebase_collection_list_response_get_data_length(collectionList.get());

        qCDebug(ETESYNC_LOG) << "Retrieved collection list length" << dataLength;

        const EtebaseCollection *etesyncCollections[dataLength];
        if (etebase_collection_list_response_get_data(collectionList.get(), etesyncCollections)) {
            setError(int(etesync_get_error_code()));
            const char *err = etebase_error_get_message();
            setErrorText(QString::fromUtf8(err));
        }

        Collection collection;

        for (int i = 0; i < dataLength; i++) {
            setupCollection(collection, etesyncCollections[i]);
            saveCollectionCache(collectionManager.get(), etesyncCollections[i], collection);
        }

        int removedCollectionsLength = etebase_collection_list_response_get_removed_memberships_length(collectionList.get());
        if (removedCollectionsLength) {
            const EtebaseRemovedCollection *removedEtesyncCollections[removedCollectionsLength];
            etebase_collection_list_response_get_removed_memberships(collectionList.get(), removedEtesyncCollections);
            for (int i = 0; i < removedCollectionsLength; i++) {
                Collection collection;
                const QString journalUid = QString::fromUtf8(etebase_removed_collection_get_uid(removedEtesyncCollections[i]));
                collection.setRemoteId(journalUid);
                mRemovedCollections.push_back(collection);
            }
        }
    }
}

void JournalsFetchJob::setupCollection(Akonadi::Collection &collection, const EtebaseCollection *etesyncCollection)
{
    qCDebug(ETESYNC_LOG) << "Setting up collection" << etebase_collection_get_uid(etesyncCollection);
    if (!etesyncCollection) {
        qCDebug(ETESYNC_LOG) << "Unable to setup collection - etesyncCollection is null";
        return;
    }

    EtebaseCollectionMetadataPtr metaData(etebase_collection_get_meta(etesyncCollection));

    const QString type = QString::fromUtf8(etebase_collection_metadata_get_collection_type(metaData.get()));

    qCDebug(ETESYNC_LOG) << "Type" << type;

    QStringList mimeTypes;

    auto attr = collection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);

    const QString displayName = QString::fromUtf8(etebase_collection_metadata_get_name(metaData.get()));

    qCDebug(ETESYNC_LOG) << "Name:" << displayName;

    if (type == ETEBASE_COLLECTION_TYPE_ADDRESS_BOOK) {
        mimeTypes.push_back(KContacts::Addressee::mimeType());
        mimeTypes.push_back(KContacts::ContactGroup::mimeType());
        attr->setDisplayName(displayName);
        attr->setIconName(QStringLiteral("view-pim-contacts"));
    } else if (type == ETEBASE_COLLECTION_TYPE_CALENDAR) {
        mimeTypes.push_back(KCalendarCore::Event::eventMimeType());
        attr->setDisplayName(displayName);
        attr->setIconName(QStringLiteral("view-calendar"));
    } else if (type == ETEBASE_COLLECTION_TYPE_TASKS) {
        mimeTypes.push_back(KCalendarCore::Todo::todoMimeType());
        attr->setDisplayName(displayName);
        attr->setIconName(QStringLiteral("view-pim-tasks"));
    } else {
        qCWarning(ETESYNC_LOG) << "Unknown journal type. Cannot set collection mime type.";
    }

    const QString journalUid = QString::fromUtf8(etebase_collection_get_uid(etesyncCollection));
    auto collectionColor = QString::fromUtf8(etebase_collection_metadata_get_color(metaData.get()));
    auto colorAttr = collection.attribute<Akonadi::CollectionColorAttribute>(Collection::AddIfMissing);
    if (collectionColor.isEmpty()) {
        collectionColor = ETESYNC_DEFAULT_COLLECTION_COLOR;
    }
    colorAttr->setColor(collectionColor);

    if (etebase_collection_get_access_level(etesyncCollection) == ETEBASE_COLLECTION_ACCESS_LEVEL_READ_ONLY) {
        collection.setRights(Collection::ReadOnly);
    }

    collection.setRemoteId(journalUid);
    collection.setName(journalUid);
    collection.setContentMimeTypes(mimeTypes);

    if (etebase_collection_is_deleted(etesyncCollection)) {
        mRemovedCollections.push_back(collection);
    }

    collection.setParentCollection(mResourceCollection);

    mCollections.push_back(collection);
}

void JournalsFetchJob::saveCollectionCache(const EtebaseCollectionManager *collectionManager, const EtebaseCollection *etebaseCollection, Collection &collection)
{
    uintptr_t ret_size;
    EtebaseCachePtr cache(etebase_collection_manager_cache_save(collectionManager, etebaseCollection, &ret_size));
    QByteArray cacheData((char *)cache.get(), ret_size);
    CollectionCacheAttribute *collectionCacheAttribute = collection.attribute<CollectionCacheAttribute>(Collection::AddIfMissing);
    collectionCacheAttribute->setCollectionCache(cacheData);
}
