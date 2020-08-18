/*
 * Copyright (C) 2020 by Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "etesyncresource.h"

#include <kcontacts/addressee.h>
#include <kcontacts/contactgroup.h>
#include <kwindowsystem.h>

#include <AkonadiCore/CachePolicy>
#include <AkonadiCore/ChangeRecorder>
#include <AkonadiCore/CollectionColorAttribute>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/CollectionModifyJob>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/ItemFetchScope>
#include <KCalendarCore/Event>
#include <KCalendarCore/Todo>
#include <KLocalizedString>
#include <QDBusConnection>

#include "entriesfetchjob.h"
#include "etesync_debug.h"
#include "etesyncadapter.h"
#include "journalsfetchjob.h"
#include "settings.h"
#include "settingsadaptor.h"
#include "setupwizard.h"

using namespace EteSyncAPI;
using namespace Akonadi;

#define ROOT_COLLECTION_REMOTEID QStringLiteral("EteSyncRootCollection")

EteSyncResource::EteSyncResource(const QString &id)
    : ResourceBase(id)
{
    Settings::instance(KSharedConfig::openConfig());
    new SettingsAdaptor(Settings::self());

    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Settings"),
                                                 Settings::self(),
                                                 QDBusConnection::ExportAdaptors);

    setName(i18n("EteSync Resource"));

    setNeedsNetwork(true);

    changeRecorder()->itemFetchScope().fetchFullPayload(true);
    changeRecorder()->itemFetchScope().setAncestorRetrieval(ItemFetchScope::All);
    changeRecorder()->fetchCollection(true);
    changeRecorder()->collectionFetchScope().setAncestorRetrieval(CollectionFetchScope::All);

    // Make local resource directory
    initialiseDirectory(baseDirectoryPath());

    mClientState = EteSyncClientState::Ptr(new EteSyncClientState());
    connect(mClientState.get(), &EteSyncClientState::clientInitialised, this, &EteSyncResource::initialiseDone);
    mClientState->init();

    mHandlers.clear();
    mHandlers.push_back(BaseHandler::Ptr(new CalendarHandler(this)));
    mHandlers.push_back(BaseHandler::Ptr(new ContactHandler(this)));
    mHandlers.push_back(BaseHandler::Ptr(new TaskHandler(this)));

    mContactHandler = ContactHandler::Ptr(new ContactHandler(this));
    mCalendarHandler = CalendarHandler::Ptr(new CalendarHandler(this));
    mTaskHandler = TaskHandler::Ptr(new TaskHandler(this));

    connect(this, &Akonadi::AgentBase::reloadConfiguration, this, &EteSyncResource::onReloadConfiguration);

    qCDebug(ETESYNC_LOG) << "Resource started";
}

void EteSyncResource::cleanup()
{
    mClientState->invalidateToken();
    QDir dir(baseDirectoryPath());
    dir.removeRecursively();
    ResourceBase::cleanup();
}

void EteSyncResource::configure(WId windowId)
{
    SetupWizard wizard(mClientState.get());

    if (windowId) {
        wizard.setAttribute(Qt::WA_NativeWindow, true);
        KWindowSystem::setMainWindow(wizard.windowHandle(), windowId);
    }
    const int result = wizard.exec();
    if (result == QDialog::Accepted) {
        synchronize();
        mClientState->saveSettings();
        Q_EMIT configurationDialogAccepted();
    } else {
        Q_EMIT configurationDialogRejected();
    }
}

void EteSyncResource::retrieveCollections()
{
    qCDebug(ETESYNC_LOG) << "Retrieving collections";
    setCollectionStreamingEnabled(true);

    mJournalsCache.clear();
    mClientState->refreshUserInfo();

    auto job = new JournalsFetchJob(mClientState->client(), this);
    connect(job, &JournalsFetchJob::finished, this, &EteSyncResource::slotCollectionsRetrieved);
    job->start();
}

Collection EteSyncResource::createRootCollection()
{
    Collection rootCollection = Collection();
    rootCollection.setContentMimeTypes({Collection::mimeType()});
    rootCollection.setName(mClientState->username());
    rootCollection.setRemoteId(ROOT_COLLECTION_REMOTEID);
    rootCollection.setParentCollection(Collection::root());
    rootCollection.setRights(Collection::CanCreateCollection);

    Akonadi::CachePolicy cachePolicy;
    cachePolicy.setInheritFromParent(false);
    cachePolicy.setSyncOnDemand(false);
    cachePolicy.setCacheTimeout(-1);
    cachePolicy.setIntervalCheckTime(5);
    rootCollection.setCachePolicy(cachePolicy);

    EntityDisplayAttribute *attr = rootCollection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
    attr->setDisplayName(mClientState->username());
    attr->setIconName(QStringLiteral("akonadi-etesync"));

    return rootCollection;
}

void EteSyncResource::slotCollectionsRetrieved(KJob *job)
{
    if (job->error()) {
        qCWarning(ETESYNC_LOG) << "Error in fetching journals";
        handleTokenError();
        return;
    }
    EteSyncJournal **journals = qobject_cast<JournalsFetchJob *>(job)->journals();
    Collection::List list;
    const Collection &rootCollection = createRootCollection();
    list.push_back(rootCollection);
    for (EteSyncJournal **iter = journals; *iter; iter++) {
        Collection collection;
        collection.setParentCollection(rootCollection);
        setupCollection(collection, *iter);
        mJournalsCache[collection.remoteId()] = EteSyncJournalPtr(*iter);
        list.push_back(collection);
    }
    mJournalsCacheUpdateTime = QDateTime::currentDateTime();
    free(journals);
    collectionsRetrieved(list);
    collectionsRetrievalDone();
}

bool EteSyncResource::handleTokenError()
{
    if (etesync_get_error_code() == EteSyncErrorCode::ETESYNC_ERROR_CODE_UNAUTHORIZED) {
        qCDebug(ETESYNC_LOG) << "Invalid token";
        deferTask();
        connect(mClientState.get(), &EteSyncClientState::tokenRefreshed, this, &EteSyncResource::taskDone);
        scheduleCustomTask(mClientState.get(), "refreshToken", QVariant(), ResourceBase::Prepend);
        return false;
    }
    return true;
}

void EteSyncResource::setupCollection(Collection &collection, EteSyncJournal *journal)
{
    if (!journal) {
        qCDebug(ETESYNC_LOG) << "Unable to setup collection - journal is null";
        return;
    }
    EteSyncCryptoManagerPtr cryptoManager = etesync_journal_get_crypto_manager(journal, mClientState->derived(), mClientState->keypair());

    EteSyncCollectionInfoPtr info(etesync_journal_get_info(journal, cryptoManager.get()));

    const QString type = QStringFromCharPtr(CharPtr(etesync_collection_info_get_type(info.get())));

    QStringList mimeTypes;

    auto attr = collection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);

    const QString displayName = QStringFromCharPtr(CharPtr(etesync_collection_info_get_display_name(info.get())));

    if (type == QStringLiteral(ETESYNC_COLLECTION_TYPE_ADDRESS_BOOK)) {
        mimeTypes.push_back(KContacts::Addressee::mimeType());
        mimeTypes.push_back(KContacts::ContactGroup::mimeType());
        attr->setDisplayName(displayName);
        attr->setIconName(QStringLiteral("view-pim-contacts"));
    } else if (type == QStringLiteral(ETESYNC_COLLECTION_TYPE_CALENDAR)) {
        mimeTypes.push_back(KCalendarCore::Event::eventMimeType());
        attr->setDisplayName(displayName);
        attr->setIconName(QStringLiteral("view-calendar"));
    } else if (type == QStringLiteral(ETESYNC_COLLECTION_TYPE_TASKS)) {
        mimeTypes.push_back(KCalendarCore::Todo::todoMimeType());
        attr->setDisplayName(displayName);
        attr->setIconName(QStringLiteral("view-pim-tasks"));
    } else {
        qCWarning(ETESYNC_LOG) << "Unknown journal type. Cannot set collection mime type.";
    }

    const QString journalUid = QStringFromCharPtr(CharPtr(etesync_journal_get_uid(journal)));
    auto collectionColor = etesync_collection_info_get_color(info.get());
    auto colorAttr = collection.attribute<Akonadi::CollectionColorAttribute>(Collection::AddIfMissing);
    colorAttr->setColor(collectionColor);

    if (etesync_journal_is_read_only(journal)) {
        collection.setRights(Collection::ReadOnly);
    }

    collection.setRemoteId(journalUid);
    collection.setName(journalUid);
    collection.setContentMimeTypes(mimeTypes);
}

BaseHandler *EteSyncResource::fetchHandlerForMimeType(const QString &mimeType)
{
    auto it = std::find_if(mHandlers.cbegin(), mHandlers.cend(),
                           [&mimeType](const BaseHandler::Ptr &handler) {
                               return handler->mimeType() == mimeType;
                           });

    if (it != mHandlers.cend()) {
        return it->get();
    } else {
        return nullptr;
    }
}

BaseHandler *EteSyncResource::fetchHandlerForCollection(const Akonadi::Collection &collection)
{
    auto it = std::find_if(mHandlers.cbegin(), mHandlers.cend(),
                           [&collection](const BaseHandler::Ptr &handler) {
                               return collection.contentMimeTypes().contains(handler->mimeType());
                           });

    if (it != mHandlers.cend()) {
        return it->get();
    } else {
        return nullptr;
    }
}

void EteSyncResource::retrieveItems(const Akonadi::Collection &collection)
{
    qCDebug(ETESYNC_LOG) << "Retrieving entries for journal" << collection.remoteId();
    const int timeSinceLastCacheUpdate = mJournalsCacheUpdateTime.secsTo(QDateTime::currentDateTime());
    if (timeSinceLastCacheUpdate <= 30) {
        const QString journalUid = collection.remoteId();
        const EteSyncJournalPtr &journal = getJournal(journalUid);
        const QString lastEntryUid = QStringFromCharPtr(CharPtr(etesync_journal_get_last_uid(journal.get())));
        if (lastEntryUid == collection.remoteRevision()) {
            itemsRetrievalDone();
            return;
        }
    }

    auto job = new EntriesFetchJob(mClientState->client(), collection, this);

    connect(job, &EntriesFetchJob::finished, this, &EteSyncResource::slotItemsRetrieved);

    job->start();
}

void EteSyncResource::slotItemsRetrieved(KJob *job)
{
    if (job->error()) {
        qCWarning(ETESYNC_LOG) << job->errorText();
        handleTokenError();
        return;
    }

    qCDebug(ETESYNC_LOG) << "Retrieving entries";
    auto entries = qobject_cast<EntriesFetchJob *>(job)->getEntries();

    Collection collection = qobject_cast<EntriesFetchJob *>(job)->collection();

    QString prevUid = qobject_cast<EntriesFetchJob *>(job)->getPrevUid();

    auto handler = fetchHandlerForCollection(collection);

    if (handler) {
        handler->setupItems(entries, collection, prevUid);
    } else {
        qCWarning(ETESYNC_LOG) << "Unknown collection" << collection.name();
        itemsRetrieved({});
    }
}

void EteSyncResource::aboutToQuit()
{
}

void EteSyncResource::onReloadConfiguration()
{
    qCDebug(ETESYNC_LOG) << "Resource config reload";
    synchronize();
}

void EteSyncResource::initialiseDone(bool successful)
{
    qCDebug(ETESYNC_LOG) << "Resource intialised";
    if (successful) {
        synchronize();
    }
}

QString EteSyncResource::baseDirectoryPath() const
{
    return Settings::self()->basePath();
}

void EteSyncResource::initialiseDirectory(const QString &path) const
{
    QDir dir(path);

    // if folder does not exists, create it
    QDir::root().mkpath(dir.absolutePath());

    // check whether warning file is in place...
    QFile file(dir.absolutePath() + QStringLiteral("/WARNING_README.txt"));
    if (!file.exists()) {
        // ... if not, create it
        file.open(QIODevice::WriteOnly);
        file.write(
            "Important warning!\n\n"
            "Do not create or copy vCards inside this folder manually, they are managed by the Akonadi framework!\n");
        file.close();
    }
}

void EteSyncResource::itemAdded(const Akonadi::Item &item,
                                const Akonadi::Collection &collection)
{
    qCDebug(ETESYNC_LOG) << "Item added" << item.mimeType();
    qCDebug(ETESYNC_LOG) << "Journal UID" << collection.remoteId();

    auto handler = fetchHandlerForMimeType(item.mimeType());
    if (handler) {
        handler->itemAdded(item, collection);
    } else {
        qCWarning(ETESYNC_LOG) << "Could not add item" << item.mimeType();
        cancelTask(i18n("Invalid payload type"));
    }
}

void EteSyncResource::itemChanged(const Akonadi::Item &item,
                                  const QSet<QByteArray> &parts)
{
    qCDebug(ETESYNC_LOG) << "Item changed" << item.mimeType();
    qCDebug(ETESYNC_LOG) << "Journal UID" << item.parentCollection().remoteId();

    auto handler = fetchHandlerForMimeType(item.mimeType());
    if (handler) {
        handler->itemChanged(item, parts);
    } else {
        qCWarning(ETESYNC_LOG) << "Could not change item" << item.mimeType();
        cancelTask(i18n("Invalid payload type"));
    }
}

void EteSyncResource::itemRemoved(const Akonadi::Item &item)
{
    qCDebug(ETESYNC_LOG) << "Item removed" << item.mimeType();

    auto handler = fetchHandlerForMimeType(item.mimeType());
    if (handler) {
        handler->itemRemoved(item);
    } else {
        qCWarning(ETESYNC_LOG) << "Could not remove item" << item.mimeType();
        cancelTask(i18n("Invalid payload type"));
    }
}

void EteSyncResource::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    qCDebug(ETESYNC_LOG) << "Collection added" << collection.mimeType();

    auto handler = fetchHandlerForCollection(collection);
    if (handler) {
        handler->collectionAdded(collection, parent);
    } else {
        qCWarning(ETESYNC_LOG) << "Could not add collection" << collection.displayName() << "mimetypes:" << collection.contentMimeTypes();
        cancelTask(i18n("Unknown collection mimetype"));
    }
}

void EteSyncResource::collectionChanged(const Akonadi::Collection &collection)
{
    qCDebug(ETESYNC_LOG) << "Collection changed" << collection.mimeType();

    auto handler = fetchHandlerForCollection(collection);
    if (handler) {
        handler->collectionChanged(collection);
    } else {
        qCWarning(ETESYNC_LOG) << "Could not change collection" << collection.displayName() << "mimetypes:" << collection.contentMimeTypes();
        cancelTask(i18n("Unknown collection mimetype"));
    }
}

void EteSyncResource::collectionRemoved(const Akonadi::Collection &collection)
{
    qCDebug(ETESYNC_LOG) << "Collection removed" << collection.mimeType();

    auto handler = fetchHandlerForCollection(collection);
    if (handler) {
        handler->collectionRemoved(collection);
    } else {
        qCWarning(ETESYNC_LOG) << "Could not remove collection" << collection.displayName() << "mimetypes:" << collection.contentMimeTypes();
        cancelTask(i18n("Unknown collection mimetype"));
    }
}

AKONADI_RESOURCE_MAIN(EteSyncResource)
