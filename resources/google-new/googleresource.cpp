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


#define ACCESS_TOKEN_PROPERTY "AccessToken"
#define CALENDARS_PROPERTY "_KGAPI2CalendarPtr"
#define ROOT_COLLECTION_REMOTEID QStringLiteral("RootCollection")


Q_DECLARE_METATYPE(KGAPI2::Job *)

using namespace KGAPI2;
using namespace Akonadi;

GoogleResource::GoogleResource(const QString &id)
    : ResourceBase(id)
    , AgentBase::ObserverV2()
{
    AttributeFactory::registerAttribute< DefaultReminderAttribute >();

    connect(this, &GoogleResource::abortRequested, this,
            [this](){ cancelTask(i18n("Aborted")); });
    connect(this, &GoogleResource::reloadConfiguration, this, &GoogleResource::reloadConfig);

    setNeedsNetwork(true);

    changeRecorder()->itemFetchScope().fetchFullPayload(true);
    changeRecorder()->itemFetchScope().setAncestorRetrieval(ItemFetchScope::All);
    changeRecorder()->fetchCollection(true);
    changeRecorder()->collectionFetchScope().setAncestorRetrieval(CollectionFetchScope::All);

    m_settings = new GoogleSettings();
    m_settings->setWindowId(winIdForDialogs());
    connect(m_settings, &GoogleSettings::accountReady, [this](bool ready){
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
                Q_EMIT status(Idle, i18nc("@info:status", "Ready"));
                synchronize();
            });

    Q_EMIT status(NotConfigured, i18n("Waiting for KWallet..."));
    updateResourceName();

    m_freeBusyHandler.reset(new CalendarHandler(this, m_settings));
    m_handlers << m_freeBusyHandler;
    m_handlers << GenericHandler::Ptr(new ContactHandler(this, m_settings));
    m_handlers << GenericHandler::Ptr(new TaskHandler(this, m_settings));

    for (auto handler : m_handlers) {
        connect(handler.data(), &GenericHandler::status, [this](int code, QString message){
                Q_EMIT status(code, message);
            });
        connect(handler.data(), &GenericHandler::percent, [this](int value){
                Q_EMIT percent(value);
            });
        connect(handler.data(), &GenericHandler::collectionsRetrieved, this, &GoogleResource::collectionsPartiallyRetrieved);
    }

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

        Q_EMIT status(Idle, i18nc("@info:status", "Ready"));
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
        Q_EMIT status(Idle, i18nc("@info:status", "Ready"));
    }
}

bool GoogleResource::handleError(KGAPI2::Job *job, bool _cancelTask)
{
    if ((job->error() == KGAPI2::NoError) || (job->error() == KGAPI2::OK)) {
        return true;
    }
    qCDebug(GOOGLE_LOG) << job << job->errorString();
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
    qCDebug(GOOGLE_LOG) << "Job finished";

    const Item item = job->property(ITEM_PROPERTY).value<Item>();
    const Collection collection = job->property(COLLECTION_PROPERTY).value<Collection>();
    if (item.isValid()) {
        changeCommitted(item);
    } else if (collection.isValid()) {
        changeCommitted(collection);
    } else {
        taskDone();
    }

    Q_EMIT status(Idle, i18nc("@info:status", "Ready"));
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
    qCDebug(GOOGLE_LOG) << "Retrieve Collections";
    if (!canPerformTask()) {
        return;
    }

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

    m_collections = { m_rootCollection };

    m_jobs = 0;
    for (auto handler : m_handlers) {
        handler->retrieveCollections();
        m_jobs++;
    }
}

void GoogleResource::collectionsPartiallyRetrieved(const Collection::List& collections)
{
    m_jobs--;
    m_collections << collections;
    if (m_jobs == 0) {
        qCDebug(GOOGLE_LOG) << "Collections retrieved!";
        collectionsRetrieved(m_collections);
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

void GoogleResource::itemRemoved(const Akonadi::Item &item)
{
    if (!canPerformTask()) {
        return;
    }
    auto it = std::find_if(m_handlers.begin(), m_handlers.end(),
            [&item](const GenericHandler::Ptr &handler){
                return handler->mimetype() == item.mimeType();
            });
    if (it != m_handlers.end()) {
        (*it)->itemRemoved(item);
    } else {
        qCWarning(GOOGLE_LOG) << "Could not remove item" << item.mimeType();
        cancelTask(i18n("Invalid payload type"));
    }
}

void GoogleResource::itemMoved(const Akonadi::Item &item, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination)
{
    if (!canPerformTask()) {
        return;
    }
    auto it = std::find_if(m_handlers.begin(), m_handlers.end(),
            [&item](const GenericHandler::Ptr &handler){
                return handler->canPerformTask(item);
            });
    if (it != m_handlers.end()) {
        (*it)->itemMoved(item, collectionSource, collectionDestination);
    } else {
        qCWarning(GOOGLE_LOG) << "Could not move item" << item.mimeType() << "from" << collectionSource.remoteId() << "to" << collectionDestination.remoteId();
        cancelTask(i18n("Invalid payload type"));
    }
}

void GoogleResource::itemLinked(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }
    auto it = std::find_if(m_handlers.begin(), m_handlers.end(),
            [&item](const GenericHandler::Ptr &handler){
                return handler->canPerformTask(item);
            });
    if (it != m_handlers.end()) {
        (*it)->itemLinked(item, collection);
    } else {
        qCWarning(GOOGLE_LOG) << "Could not link item" << item.mimeType() << "to" << collection.remoteId();
        cancelTask(i18n("Invalid payload type"));
    }
}

void GoogleResource::itemUnlinked(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }
    auto it = std::find_if(m_handlers.begin(), m_handlers.end(),
            [&item](const GenericHandler::Ptr &handler){
                return handler->canPerformTask(item);
            });
    if (it != m_handlers.end()) {
        (*it)->itemUnlinked(item, collection);
    } else {
        qCWarning(GOOGLE_LOG) << "Could not unlink item mimetype" << item.mimeType() << "from" << collection.remoteId();
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
