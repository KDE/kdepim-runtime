/*
   Copyright (C) 2011-2013 Daniel Vrátil <dvratil@redhat.com>
                 2020  Igor Poboiko <igor.poboiko@gmail.com>

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

#include "googleresource.h"
#include "googlesettings.h"
#include "googlesettingsdialog.h"
#include "googleresource_debug.h"
#include "settingsadaptor.h"

#include "calendarhandler.h"
#include "contacthandler.h"
#include "taskhandler.h"

#include "defaultreminderattribute.h"

#include <AkonadiCore/AttributeFactory>
#include <AkonadiCore/CachePolicy>
#include <AkonadiCore/ChangeRecorder>
#include <AkonadiCore/CollectionFetchScope>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/ItemFetchScope>

#include <KCalendarCore/Event>

#include <QDialog>
#include <QIcon>
#include <KLocalizedString>

#include <KGAPI/Account>
#include <KGAPI/AccountInfoFetchJob>
#include <KGAPI/AccountInfo>
#include <KGAPI/AuthJob>

#include <algorithm>


#define CALENDARS_PROPERTY "_KGAPI2CalendarPtr"
#define ROOT_COLLECTION_REMOTEID QStringLiteral("RootCollection")


Q_DECLARE_METATYPE(KGAPI2::Job *)

using namespace KGAPI2;
using namespace Akonadi;

GoogleResource::GoogleResource(const QString &id)
    : ResourceBase(id)
    , AgentBase::ObserverV3()
{
    AttributeFactory::registerAttribute< DefaultReminderAttribute >();

    connect(this, &GoogleResource::reloadConfiguration, this, &GoogleResource::reloadConfig);

    setNeedsNetwork(true);

    changeRecorder()->itemFetchScope().fetchFullPayload(true);
    changeRecorder()->itemFetchScope().setAncestorRetrieval(ItemFetchScope::All);
    changeRecorder()->fetchCollection(true);
    changeRecorder()->collectionFetchScope().setAncestorRetrieval(CollectionFetchScope::All);

    m_settings = new GoogleSettings();
    m_settings->setWindowId(winIdForDialogs());
    connect(m_settings, &GoogleSettings::accountReady, this, [this](bool ready){
                if (accountId() > 0) {
                    return;
                }
                if (!ready) {
                    Q_EMIT status(Broken, i18n("Can't access KWallet"));
                    return;
                }
                if (m_settings->accountPtr().isNull()) {
                    Q_EMIT status(NotConfigured);
                    return;
                }
                emitReadyStatus();
                synchronize();
            });

    Q_EMIT status(NotConfigured, i18n("Waiting for KWallet..."));
    updateResourceName();

    m_freeBusyHandler.reset(new CalendarHandler(this, m_settings));
    m_handlers << m_freeBusyHandler;
    m_handlers << GenericHandler::Ptr(new ContactHandler(this, m_settings));
    m_handlers << GenericHandler::Ptr(new TaskHandler(this, m_settings));

    new SettingsAdaptor(m_settings);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Settings"),
                                                 m_settings, QDBusConnection::ExportAdaptors);
}

GoogleResource::~GoogleResource()
{
}

void GoogleResource::cleanup()
{
    m_settings->cleanup();
    ResourceBase::cleanup();
}

void GoogleResource::emitReadyStatus()
{
    Q_EMIT status(Idle, i18nc("@info:status", "Ready"));
}

Akonadi::Collection GoogleResource::rootCollection() const
{
    return m_rootCollection;
}

void GoogleResource::configure(WId windowId)
{
    if (!m_settings->isReady() || m_isConfiguring) {
        Q_EMIT configurationDialogAccepted();
        return;
    }

    m_isConfiguring = true;

    QScopedPointer<GoogleSettingsDialog> settingsDialog(new GoogleSettingsDialog(this, m_settings, windowId));
    settingsDialog->setWindowIcon(QIcon::fromTheme(QStringLiteral("im-google")));
    if (settingsDialog->exec() == QDialog::Accepted) {
        updateResourceName();

        Q_EMIT configurationDialogAccepted();

        if (m_settings->accountPtr().isNull()) {
            Q_EMIT status(NotConfigured, i18n("Configured account does not exist"));
            m_isConfiguring = false;
            return;
        }

        emitReadyStatus();
        synchronize();
    } else {
        updateResourceName();

        Q_EMIT configurationDialogRejected();
    }

    m_isConfiguring = false;
}

QList<QUrl> GoogleResource::scopes() const
{
    // TODO: determine it based on what user wants?
    const QList< QUrl > scopes = {Account::accountInfoScopeUrl(), Account::calendarScopeUrl(),
        Account::contactsScopeUrl(), Account::tasksScopeUrl()};
    return scopes;
}

void GoogleResource::updateResourceName()
{
    const QString accountName = m_settings->account();
    setName(i18nc("%1 is account name (user@gmail.com)", "Google Groupware (%1)", accountName.isEmpty() ? i18n("not configured") : accountName));
}

void GoogleResource::reloadConfig()
{
    const AccountPtr account = m_settings->accountPtr();
    if (account.isNull() || account->accountName().isEmpty()) {
        Q_EMIT status(NotConfigured, i18n("Configured account does not exist"));
    } else {
        emitReadyStatus();
    }
}

bool GoogleResource::handleError(KGAPI2::Job *job, bool _cancelTask)
{
    if ((job->error() == KGAPI2::NoError) || (job->error() == KGAPI2::OK)) {
        return true;
    }
    qCWarning(GOOGLE_LOG) << "Got error:" << job << job->errorString();
    AccountPtr account = job->account();
    if (job->error() == KGAPI2::Unauthorized) {
        const QList<QUrl> resourceScopes = scopes();
        for (const QUrl &scope : resourceScopes) {
            if (!account->scopes().contains(scope)) {
                account->addScope(scope);
            }
        }

        AuthJob *authJob = new AuthJob(account, m_settings->clientId(), m_settings->clientSecret(), this);
        authJob->setProperty(JOB_PROPERTY, QVariant::fromValue(job));
        connect(authJob, &AuthJob::finished, this, &GoogleResource::slotAuthJobFinished);
        return false;
    }

    if (_cancelTask) {
        cancelTask(job->errorString());
    }
    return false;
}

bool GoogleResource::canPerformTask()
{
    if (!m_settings->accountPtr() && accountId() == 0) {
        cancelTask(i18nc("@info:status", "Resource is not configured"));
        Q_EMIT status(NotConfigured, i18nc("@info:status", "Resource is not configured"));
        return false;
    }

    return true;
}

void GoogleResource::slotAuthJobFinished(KGAPI2::Job *job)
{
    if (job->error() != KGAPI2::NoError) {
        cancelTask(i18n("Failed to refresh tokens"));
        return;
    }

    AuthJob *authJob = qobject_cast<AuthJob *>(job);
    AccountPtr account = authJob->account();
    if (!m_settings->storeAccount(account)) {
        qCWarning(GOOGLE_LOG) << "Failed to store account in KWallet";
    }

    KGAPI2::Job *otherJob = job->property(JOB_PROPERTY).value<KGAPI2::Job *>();
    if (otherJob) {
        otherJob->setAccount(account);
        otherJob->restart();
    }
}

void GoogleResource::slotGenericJobFinished(KGAPI2::Job *job)
{
    if (!handleError(job)) {
        return;
    }
    if (job->property(ITEM_PROPERTY).isValid()) {
        qCDebug(GOOGLE_LOG) << "Item change committed";
        changeCommitted(job->property(ITEM_PROPERTY).value<Item>());
    } else if (job->property(ITEMS_PROPERTY).isValid()) {
        qCDebug(GOOGLE_LOG) << "Items changes committed";
        changesCommitted(job->property(ITEMS_PROPERTY).value<Item::List>());
    } else if (job->property(COLLECTION_PROPERTY).isValid()) {
        qCDebug(GOOGLE_LOG) << "Collection change committed";
        changeCommitted(job->property(COLLECTION_PROPERTY).value<Collection>());
    } else {
        qCDebug(GOOGLE_LOG) << "Task done";
        taskDone();
    }

    emitReadyStatus();
}

int GoogleResource::accountId() const
{
    return 0;
}

QDateTime GoogleResource::lastCacheUpdate() const
{
    if (m_freeBusyHandler) {
        return m_freeBusyHandler->lastCacheUpdate();
    }
    return QDateTime();
}

void GoogleResource::canHandleFreeBusy(const QString &email) const
{
    if (m_freeBusyHandler) {
        m_freeBusyHandler->canHandleFreeBusy(email);
    } else {
        handlesFreeBusy(email, false);
    }
}

void GoogleResource::retrieveFreeBusy(const QString &email, const QDateTime &start, const QDateTime &end)
{
    if (m_freeBusyHandler) {
        m_freeBusyHandler->retrieveFreeBusy(email, start, end);
    } else {
        freeBusyRetrieved(email, QString(), false, QString());
    }
}

/*
 * Collection handling
 */
void GoogleResource::retrieveCollections()
{
    if (!canPerformTask()) {
        return;
    }
    qCDebug(GOOGLE_LOG) << "Retrieve Collections";

    setCollectionStreamingEnabled(true);
    CachePolicy cachePolicy;
    if (m_settings->enableIntervalCheck()) {
        cachePolicy.setInheritFromParent(false);
        cachePolicy.setIntervalCheckTime(m_settings->intervalCheckTime());
    }

    // Setting up root collection
    m_rootCollection = Collection();
    m_rootCollection.setContentMimeTypes({ Collection::mimeType(), Collection::virtualMimeType() });
    m_rootCollection.setRemoteId(ROOT_COLLECTION_REMOTEID);
    m_rootCollection.setName(m_settings->accountPtr()->accountName());
    m_rootCollection.setParentCollection(Collection::root());
    m_rootCollection.setRights(Collection::CanCreateCollection);
    m_rootCollection.setCachePolicy(cachePolicy);

    EntityDisplayAttribute *attr = m_rootCollection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
    attr->setDisplayName(m_settings->accountPtr()->accountName());
    attr->setIconName(QStringLiteral("im-google"));

    collectionsRetrieved({ m_rootCollection });

    m_jobs = 0;
    for (auto &handler : m_handlers) {
        handler->retrieveCollections();
        m_jobs++;
    }
}

void GoogleResource::collectionsRetrievedFromHandler(const Collection::List &collections)
{
    collectionsRetrieved(collections);
    m_jobs--;
    if (m_jobs == 0) {
        qCDebug(GOOGLE_LOG) << "All collections retrieved!";
        collectionsRetrievalDone();
        //taskDone(); // ???
        emitReadyStatus();
    }
}

void GoogleResource::retrieveItems(const Akonadi::Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }

    auto it = std::find_if(m_handlers.begin(), m_handlers.end(),
            [&collection](const GenericHandler::Ptr &handler){
                return collection.contentMimeTypes().contains(handler->mimetype());
            });

    if (it != m_handlers.end()) {
        (*it)->retrieveItems(collection);
    } else {
        qCWarning(GOOGLE_LOG) << "Unknown collection" << collection.name();
        itemsRetrieved({});
    }
}

void GoogleResource::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }

    auto it = std::find_if(m_handlers.begin(), m_handlers.end(),
            [&collection, &item](const GenericHandler::Ptr &handler){
                return collection.contentMimeTypes().contains(handler->mimetype())
                    && handler->canPerformTask(item);
            });
    if (it != m_handlers.end()) {
        (*it)->itemAdded(item, collection);
    } else {
        qCWarning(GOOGLE_LOG) << "Could not add item" << item.mimeType();
        cancelTask(i18n("Invalid payload type"));
    }
}

void GoogleResource::itemChanged(const Akonadi::Item &item, const QSet< QByteArray > &partIdentifiers)
{
    Q_UNUSED(partIdentifiers);
    if (!canPerformTask()) {
        return;
    }
    auto it = std::find_if(m_handlers.begin(), m_handlers.end(),
            [&item](const GenericHandler::Ptr &handler){
                return handler->canPerformTask(item);
            });
    if (it != m_handlers.end()) {
        (*it)->itemChanged(item, partIdentifiers);
    } else {
        qCWarning(GOOGLE_LOG) << "Could not change item" << item.mimeType();
        cancelTask(i18n("Invalid payload type"));
    }
}

void GoogleResource::itemsRemoved(const Item::List &items)
{
    if (!canPerformTask()) {
        return;
    }
    // TODO: what if items have different mimetypes?
    const QString mimeType = items.first().mimeType();
    auto it = std::find_if(m_handlers.begin(), m_handlers.end(),
            [&mimeType](const GenericHandler::Ptr &handler){
                return handler->mimetype() == mimeType;
            });
    if (it != m_handlers.end()) {
        (*it)->itemsRemoved(items);
    } else {
        qCWarning(GOOGLE_LOG) << "Could not remove item" << mimeType;
        cancelTask(i18n("Invalid payload type"));
    }
}

void GoogleResource::itemsMoved(const Item::List &items, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination)
{
    if (!canPerformTask()) {
        return;
    }
    // TODO: what if items have different mimetypes?
    auto it = std::find_if(m_handlers.begin(), m_handlers.end(),
            [&item = items.first()](const GenericHandler::Ptr &handler){
                return handler->canPerformTask(item);
            });
    if (it != m_handlers.end()) {
        (*it)->itemsMoved(items, collectionSource, collectionDestination);
    } else {
        qCWarning(GOOGLE_LOG) << "Could not move item" << items.first().mimeType() << "from" << collectionSource.remoteId() << "to" << collectionDestination.remoteId();
        cancelTask(i18n("Invalid payload type"));
    }
}

void GoogleResource::itemsLinked(const Item::List &items, const Akonadi::Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }
    // TODO: what if items have different mimetypes?
    auto it = std::find_if(m_handlers.begin(), m_handlers.end(),
            [&item = items.first()](const GenericHandler::Ptr &handler){
                return handler->canPerformTask(item);
            });
    if (it != m_handlers.end()) {
        (*it)->itemsLinked(items, collection);
    } else {
        qCWarning(GOOGLE_LOG) << "Could not link item" << items.first().mimeType() << "to" << collection.remoteId();
        cancelTask(i18n("Invalid payload type"));
    }
}

void GoogleResource::itemsUnlinked(const Item::List &items, const Akonadi::Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }
    // TODO: what if items have different mimetypes?
    auto it = std::find_if(m_handlers.begin(), m_handlers.end(),
            [&item = items.first()](const GenericHandler::Ptr &handler){
                return handler->canPerformTask(item);
            });
    if (it != m_handlers.end()) {
        (*it)->itemsUnlinked(items, collection);
    } else {
        qCWarning(GOOGLE_LOG) << "Could not unlink item mimetype" << items.first().mimeType() << "from" << collection.remoteId();
        cancelTask(i18n("Invalid payload type"));
    }
}

void GoogleResource::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    if (!canPerformTask()) {
        return;
    }
    auto it = std::find_if(m_handlers.begin(), m_handlers.end(),
            [&collection](const GenericHandler::Ptr &handler){
                return collection.contentMimeTypes().contains(handler->mimetype());
            });
    if (it != m_handlers.end()) {
        (*it)->collectionAdded(collection, parent);
    } else {
        qCWarning(GOOGLE_LOG) << "Could not add collection" << collection.displayName() << "mimetypes:" << collection.contentMimeTypes();
        cancelTask(i18n("Unknown collection mimetype"));
    }
}

void GoogleResource::collectionChanged(const Akonadi::Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }
    auto it = std::find_if(m_handlers.begin(), m_handlers.end(),
            [&collection](const GenericHandler::Ptr &handler){
                return collection.contentMimeTypes().contains(handler->mimetype());
            });
    if (it != m_handlers.end()) {
        (*it)->collectionChanged(collection);
    } else {
        qCWarning(GOOGLE_LOG) << "Could not change collection" << collection.displayName() << "mimetypes:" << collection.contentMimeTypes();
        cancelTask(i18n("Unknown collection mimetype"));
    }
}

void GoogleResource::collectionRemoved(const Akonadi::Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }
    auto it = std::find_if(m_handlers.begin(), m_handlers.end(),
            [&collection](const GenericHandler::Ptr &handler){
                return collection.contentMimeTypes().contains(handler->mimetype());
            });
    if (it != m_handlers.end()) {
        (*it)->collectionRemoved(collection);
    } else {
        qCWarning(GOOGLE_LOG) << "Could not remove collection" << collection.displayName() << "mimetypes:" << collection.contentMimeTypes();
        cancelTask(i18n("Unknown collection mimetype"));
    }
}

AKONADI_RESOURCE_MAIN(GoogleResource)
