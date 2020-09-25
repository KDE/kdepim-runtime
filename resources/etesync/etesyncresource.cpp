/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "etesyncresource.h"

#include <kcontacts/addressee.h>
#include <kcontacts/contactgroup.h>
#include <kwindowsystem.h>

#include <AkonadiCore/AttributeFactory>
#include <AkonadiCore/CachePolicy>
#include <AkonadiCore/ChangeRecorder>
#include <AkonadiCore/CollectionColorAttribute>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/CollectionModifyJob>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/ItemFetchScope>
#include <KCalendarCore/Event>
#include <KCalendarCore/Todo>
#include <KMessageBox>
#include <QDBusConnection>

#include "etebasecacheattribute.h"
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

    AttributeFactory::registerAttribute<EtebaseCacheAttribute>();

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
        mClientState->saveSettings();
        mCredentialsRequired = false;
        qCDebug(ETESYNC_LOG) << "Setting online";
        setOnline(true);
        synchronize();
        Q_EMIT configurationDialogAccepted();
    } else {
        qCDebug(ETESYNC_LOG) << "Setting offline";
        setOnline(false);
        Q_EMIT configurationDialogRejected();
    }
}

void EteSyncResource::retrieveCollections()
{
    qCDebug(ETESYNC_LOG) << "Retrieving collections";

    if (credentialsRequired()) {
        deferTask();
        return;
    }

    mRootCollection = createRootCollection();

    mJournalsCache.clear();

    auto job = new JournalsFetchJob(mClientState->account(), mRootCollection, this);
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

    // Keep collection list stoken preserved
    rootCollection.setRemoteRevision(mRootCollection.remoteRevision());

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
        qCWarning(ETESYNC_LOG) << "EteSync error" << job->error() << job->errorText();
        handleError(job->error());
        return;
    }

    qCDebug(ETESYNC_LOG) << "slotCollectionsRetrieved()";

    QString sToken = qobject_cast<JournalsFetchJob *>(job)->syncToken();
    mRootCollection.setRemoteRevision(sToken);

    Collection::List collections = {mRootCollection};
    collections.append(qobject_cast<JournalsFetchJob *>(job)->collections());
    Collection::List removedCollections = qobject_cast<JournalsFetchJob *>(job)->removedCollections();

    collectionsRetrievedIncremental(collections, removedCollections);

    mJournalsCacheUpdateTime = QDateTime::currentDateTime();

    qCDebug(ETESYNC_LOG) << "Collections retrieval done";
}

/**
 * Handles all EteSync errors (except CONFLICT).
 * To be called immediately after an EteSync operation.
 * CONFLICT error has been handled seperately in BaseHandler.
 */
bool EteSyncResource::handleError(int errorCode)
{
    qCDebug(ETESYNC_LOG) << "handleError - code" << errorCode;
    switch (errorCode) {
    case ETESYNC_ERROR_CODE_UNAUTHORIZED:
        qCDebug(ETESYNC_LOG) << "Invalid token";
        deferTask();
        connect(mClientState.get(), &EteSyncClientState::tokenRefreshed, this, &EteSyncResource::slotTokenRefreshed);
        scheduleCustomTask(mClientState.get(), "refreshToken", QVariant(), ResourceBase::Prepend);
        return true;
    case ETESYNC_ERROR_CODE_GENERIC:
    case ETESYNC_ERROR_CODE_ENCODING:
    case ETESYNC_ERROR_CODE_INTEGRITY:
    case ETESYNC_ERROR_CODE_ENCRYPTION:
    case ETESYNC_ERROR_CODE_ENCRYPTION_MAC:
    case ETESYNC_ERROR_CODE_PERMISSION_DENIED:
    case ETESYNC_ERROR_CODE_INVALID_DATA:
    case ETESYNC_ERROR_CODE_CONNECTION:
    case ETESYNC_ERROR_CODE_HTTP:
        qCDebug(ETESYNC_LOG) << "Cancelling task";
        cancelTask();
        return true;
    }
    return false;
}

bool EteSyncResource::handleError()
{
    return handleError(etesync_get_error_code());
}

bool EteSyncResource::credentialsRequired()
{
    if (mCredentialsRequired) {
        qCDebug(ETESYNC_LOG) << "Credentials required";
        showErrorDialog(i18n("Your EteSync credentials were changed. Please click OK to re-enter your credentials."), i18n(CharPtr(etesync_get_error_message()).get()), i18n("Credentials Changed"));
        configure(winIdForDialogs());
    }
    return mCredentialsRequired;
}

void EteSyncResource::slotTokenRefreshed(bool successful)
{
    if (!successful) {
        if (etesync_get_error_code() == ETESYNC_ERROR_CODE_HTTP) {
            qCDebug(ETESYNC_LOG) << "HTTP Error while tokenRefresh";
            mCredentialsRequired = true;
        }
    }
    taskDone();
}

void EteSyncResource::showErrorDialog(const QString &errorText, const QString &errorDetails, const QString &title)
{
    QWidget *parent = QWidget::find(winIdForDialogs());
    QDialog *dialog = new QDialog(parent, Qt::Dialog);
    dialog->setAttribute(Qt::WA_NativeWindow, true);
    KWindowSystem::setMainWindow(dialog->windowHandle(), winIdForDialogs());
    KMessageBox::detailedSorry(dialog, errorText, errorDetails, title);
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
    qCDebug(ETESYNC_LOG) << "Retrieving items for collection" << collection.remoteId();
    const int timeSinceLastCacheUpdate = mJournalsCacheUpdateTime.secsTo(QDateTime::currentDateTime());
    if (timeSinceLastCacheUpdate <= 30) {
        qCDebug(ETESYNC_LOG) << "Retrieve items called immediately after collection fetch";
        if (!collection.hasAttribute<EtebaseCacheAttribute>()) {
            qCDebug(ETESYNC_LOG) << "No cache for collection" << collection.remoteId();
            cancelTask(i18n("No cache for collection '%1'", collection.remoteId()));
            return;
        }
        EtebaseCollectionManagerPtr collectionManager(etebase_account_get_collection_manager(mClientState->account()));
        const QByteArray collectionCache = collection.attribute<EtebaseCacheAttribute>()->etebaseCache();
        EtebaseCollectionPtr etesyncCollection(etebase_collection_manager_cache_load(collectionManager.get(), collectionCache.constData(), collectionCache.size()));

        const QString sToken = QString::fromUtf8(etebase_collection_get_stoken(etesyncCollection.get()));
        qCDebug(ETESYNC_LOG) << "Comparing" << sToken << "and" << collection.remoteRevision();
        if (sToken == collection.remoteRevision()) {
            qCDebug(ETESYNC_LOG) << "Already up-to-date: Fetched collection and cached collection have the same stoken";
            itemsRetrievalDone();
            return;
        }
    }

    if (credentialsRequired()) {
        deferTask();
        return;
    }

    auto job = new EntriesFetchJob(mClientState->account(), collection, this);

    connect(job, &EntriesFetchJob::finished, this, &EteSyncResource::slotItemsRetrieved);

    job->start();
}

void EteSyncResource::slotItemsRetrieved(KJob *job)
{
    if (job->error()) {
        qCDebug(ETESYNC_LOG) << "Error in fetching entries";
        qCWarning(ETESYNC_LOG) << "EteSync error" << job->error() << job->errorText();
        handleError(job->error());
    }

    Item::List items = qobject_cast<EntriesFetchJob *>(job)->items();
    Item::List removedItems = qobject_cast<EntriesFetchJob *>(job)->removedItems();

    qCDebug(ETESYNC_LOG) << "Updating collection sync token";
    Collection collection = qobject_cast<EntriesFetchJob *>(job)->collection();
    qCDebug(ETESYNC_LOG) << "Setting collection" << collection.remoteId() << "'s sync token to" << collection.remoteRevision();
    new CollectionModifyJob(collection, this);

    itemsRetrievedIncremental(items, removedItems);

    qCDebug(ETESYNC_LOG) << "Items retrieval done";
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

void EteSyncResource::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    qCDebug(ETESYNC_LOG) << "Item added" << item.mimeType();
    qCDebug(ETESYNC_LOG) << "Journal UID" << collection.remoteId();

    if (credentialsRequired()) {
        deferTask();
        return;
    }

    auto handler = fetchHandlerForMimeType(item.mimeType());
    if (handler) {
        handler->itemAdded(item, collection);
    } else {
        qCWarning(ETESYNC_LOG) << "Could not add item" << item.mimeType();
        cancelTask(i18n("Invalid payload type"));
    }
}

void EteSyncResource::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    qCDebug(ETESYNC_LOG) << "Item changed" << item.mimeType();
    qCDebug(ETESYNC_LOG) << "Journal UID" << item.parentCollection().remoteId();

    if (credentialsRequired()) {
        deferTask();
        return;
    }

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

    if (credentialsRequired()) {
        deferTask();
        return;
    }

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

    if (credentialsRequired()) {
        deferTask();
        return;
    }

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

    if (credentialsRequired()) {
        deferTask();
        return;
    }

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

    if (credentialsRequired()) {
        deferTask();
        return;
    }

    auto handler = fetchHandlerForCollection(collection);
    if (handler) {
        handler->collectionRemoved(collection);
    } else {
        qCWarning(ETESYNC_LOG) << "Could not remove collection" << collection.displayName() << "mimetypes:" << collection.contentMimeTypes();
        cancelTask(i18n("Unknown collection mimetype"));
    }
}

AKONADI_RESOURCE_MAIN(EteSyncResource)
