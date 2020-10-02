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

#include "etesync_debug.h"

#define COLLECTIONS_FETCH_BATCH_SIZE 50

using namespace EteSyncAPI;
using namespace Akonadi;

JournalsFetchJob::JournalsFetchJob(const EtebaseAccount *account, const Akonadi::Collection &resourceCollection, const QString &cacheDir, QObject *parent)
    : KJob(parent)
    , mAccount(account)
    , mResourceCollection(resourceCollection)
    , mCacheDir(cacheDir)
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
    if (!mAccount) {
        setError(UserDefinedError);
        setErrorText(QStringLiteral("EntriesFetchJob: Etebase account is empty"));
        return;
    }

    mSyncToken = mResourceCollection.remoteRevision();
    bool done = 0;
    EtebaseCollectionManagerPtr collectionManager(etebase_account_get_collection_manager(mAccount));

    while (!done) {
        EtebaseFetchOptionsPtr fetchOptions(etebase_fetch_options_new());
        etebase_fetch_options_set_stoken(fetchOptions.get(), mSyncToken);
        etebase_fetch_options_set_limit(fetchOptions.get(), COLLECTIONS_FETCH_BATCH_SIZE);

        EtebaseCollectionListResponsePtr collectionList(etebase_collection_manager_list(collectionManager.get(), fetchOptions.get()));
        if (!collectionList) {
            setError(int(etebase_error_get_code()));
            const char *err = etebase_error_get_message();
            setErrorText(QString::fromUtf8(err));
            return;
        }

        mSyncToken = QString::fromUtf8(etebase_collection_list_response_get_stoken(collectionList.get()));
        done = etebase_collection_list_response_is_done(collectionList.get());

        uintptr_t dataLength = etebase_collection_list_response_get_data_length(collectionList.get());

        qCDebug(ETESYNC_LOG) << "Retrieved collection list length" << dataLength;

        std::vector<const EtebaseCollection *> etesyncCollections(dataLength, nullptr);
        if (etebase_collection_list_response_get_data(collectionList.get(), etesyncCollections.data())) {
            setError(int(etebase_error_get_code()));
            const char *err = etebase_error_get_message();
            setErrorText(QString::fromUtf8(err));
        }

        Collection collection;

        for (uintptr_t i = 0; i < dataLength; i++) {
            saveEtebaseCollectionCache(collectionManager.get(), etesyncCollections[i], mCacheDir);
            setupCollection(collection, etesyncCollections[i]);
        }

        uintptr_t removedCollectionsLength = etebase_collection_list_response_get_removed_memberships_length(collectionList.get());
        qCDebug(ETESYNC_LOG) << "Removed collection membership list length" << removedCollectionsLength;
        if (removedCollectionsLength) {
            std::vector<const EtebaseRemovedCollection *> removedEtesyncCollections(removedCollectionsLength, nullptr);
            if (etebase_collection_list_response_get_removed_memberships(collectionList.get(), removedEtesyncCollections.data())) {
                setError(int(etebase_error_get_code()));
                const char *err = etebase_error_get_message();
                setErrorText(QString::fromUtf8(err));
            }
            for (uintptr_t i = 0; i < removedCollectionsLength; i++) {
                Collection collection;
                const QString journalUid = QString::fromUtf8(etebase_removed_collection_get_uid(removedEtesyncCollections[i]));
                collection.setRemoteId(journalUid);
                collection.setParentCollection(mResourceCollection);
                qCDebug(ETESYNC_LOG) << "Removed collection membership" << journalUid;
                mRemovedCollections.push_back(collection);
            }
        }
    }
}

void JournalsFetchJob::setupCollection(Akonadi::Collection &collection, const EtebaseCollection *etesyncCollection)
{
    if (!etesyncCollection) {
        qCDebug(ETESYNC_LOG) << "Unable to setup collection - etesyncCollection is null";
        return;
    }

    qCDebug(ETESYNC_LOG) << "Setting up collection" << etebase_collection_get_uid(etesyncCollection);

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
    collection.setParentCollection(mResourceCollection);

    if (etebase_collection_is_deleted(etesyncCollection)) {
        mRemovedCollections.push_back(collection);
        return;
    }

    mCollections.push_back(collection);
}
