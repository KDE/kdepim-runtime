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

#include "calendarhandler.h"
#include "contacthandler.h"
#include "taskhandler.h"
#include "defaultreminderattribute.h"
#include "googleresource_debug.h"
#include "kgapiversionattribute.h"
#include "settings.h"
#include "settingsdialog.h"

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


#define ACCESS_TOKEN_PROPERTY "AccessToken"
#define CALENDARS_PROPERTY "_KGAPI2CalendarPtr"
#define ROOT_COLLECTION_REMOTEID QStringLiteral("RootCollection")


Q_DECLARE_METATYPE(KGAPI2::Job *)

using namespace KGAPI2;
using namespace Akonadi;

GoogleResource::GoogleResource(const QString &id)
    : ResourceBase(id)
    , AgentBase::ObserverV2()
    , m_isConfiguring(false)
{
    AttributeFactory::registerAttribute< DefaultReminderAttribute >();
    AttributeFactory::registerAttribute<KGAPIVersionAttribute>();

    connect(this, &GoogleResource::abortRequested, this,
            [this](){ cancelTask(i18n("Aborted")); });
    connect(this, &GoogleResource::reloadConfiguration, this, &GoogleResource::reloadConfig);

    setNeedsNetwork(true);

    changeRecorder()->itemFetchScope().fetchFullPayload(true);
    changeRecorder()->itemFetchScope().setAncestorRetrieval(ItemFetchScope::All);
    changeRecorder()->fetchCollection(true);
    changeRecorder()->collectionFetchScope().setAncestorRetrieval(CollectionFetchScope::All);

    m_accountMgr = new GoogleAccountManager(this);
    connect(m_accountMgr, &GoogleAccountManager::accountChanged, this, [this](const AccountPtr &account){
            if (accountId() > 0) {
                return;
            }
            m_account = account;
        });
    connect(m_accountMgr, &GoogleAccountManager::accountRemoved, this, [this](const QString &accountName){
            if (accountId() > 0) {
                return;
            }
            if (m_account && m_account->accountName() != accountName) {
                return;
            }
            Q_EMIT status(NotConfigured, i18n("Configured account has been removed"));
            m_account.clear();
            Settings::self()->setAccount(QString());
        });
    connect(m_accountMgr, &GoogleAccountManager::managerReady, this, [this](bool ready){
            if (accountId() > 0) {
                return;
            }
            if (!ready) {
                Q_EMIT status(Broken, i18n("Can't access KWallet"));
                return;
            }
            const QString accountName = Settings::self()->account();
            if (accountName.isEmpty()) {
                Q_EMIT status(NotConfigured);
                return;
            }
            m_account = m_accountMgr->findAccount(accountName);
            if (m_account.isNull()) {
                Q_EMIT status(NotConfigured, i18n("Configured account does not exist"));
                return;
            }
            Q_EMIT status(Idle, i18nc("@info:status", "Ready"));
            synchronize();
        });

    Q_EMIT status(NotConfigured, i18n("Waiting for KWallet..."));
    updateResourceName();

    m_freeBusyHandler.reset(new CalendarHandler(this));
    m_handlers << m_freeBusyHandler;
    m_handlers << GenericHandler::Ptr(new ContactHandler(this));
    m_handlers << GenericHandler::Ptr(new TaskHandler(this));

    for (auto handler : m_handlers) {
        connect(handler.data(), &GenericHandler::status, [this](int code, QString message){
                Q_EMIT status(code, message);
            });
        connect(handler.data(), &GenericHandler::percent, [this](int value){
                Q_EMIT percent(value);
            });
        connect(handler.data(), &GenericHandler::collectionsRetrieved, this, &GoogleResource::collectionsPartiallyRetrieved);
    }
}

GoogleResource::~GoogleResource()
{
}

void GoogleResource::cleanup()
{
    accountManager()->cleanup(Settings::self()->account());
    ResourceBase::cleanup();
}

AccountPtr GoogleResource::account() const
{
    return m_account;
}

GoogleAccountManager *GoogleResource::accountManager() const
{
    return m_accountMgr;
}

Akonadi::Collection GoogleResource::rootCollection() const
{
    return m_rootCollection;
}

void GoogleResource::configure(WId windowId)
{
    if (!m_accountMgr->isReady() || m_isConfiguring) {
        Q_EMIT configurationDialogAccepted();
        return;
    }

    m_isConfiguring = true;

    QScopedPointer<SettingsDialog> settingsDialog(new SettingsDialog(accountManager(), windowId, this));
    settingsDialog->setWindowIcon(QIcon::fromTheme(QStringLiteral("im-google")));
    if (settingsDialog->exec() == QDialog::Accepted) {
        updateResourceName();

        Q_EMIT configurationDialogAccepted();

        m_account = accountManager()->findAccount(Settings::self()->account());
        if (m_account.isNull()) {
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
    const QString accountName = Settings::self()->account();
    setName(i18nc("%1 is account name (user@gmail.com)", "Google Groupware (%1)", accountName.isEmpty() ? i18n("not configured") : accountName));
}

void GoogleResource::updateAccountToken(const AccountPtr &account, KGAPI2::Job *restartJob)
{
    if (!Settings::self()->account().isEmpty()) {
        AuthJob *authJob = new AuthJob(account, Settings::self()->clientId(), Settings::self()->clientSecret(), this);
        authJob->setProperty(JOB_PROPERTY, QVariant::fromValue(restartJob));
        connect(authJob, &AuthJob::finished, this, &GoogleResource::slotAuthJobFinished);
    }
}

void GoogleResource::reloadConfig()
{
    const QString accountName = Settings::self()->account();

    if (!accountName.isEmpty()) {
        m_account = m_accountMgr->findAccount(accountName);
        if (m_account.isNull()) {
            Q_EMIT status(NotConfigured, i18n("Configured account does not exist"));
            return;
        }
    } else {
        Q_EMIT status(NotConfigured);
        return;
    }

    Q_EMIT status(Idle, i18nc("@info:status", "Ready"));
}

bool GoogleResource::handleError(KGAPI2::Job *job, bool _cancelTask)
{
    if ((job->error() == KGAPI2::NoError) || (job->error() == KGAPI2::OK)) {
        return true;
    }
    qCDebug(GOOGLE_LOG) << job << job->errorString();

    if (job->error() == KGAPI2::Unauthorized) {
        const QList<QUrl> resourceScopes = scopes();
        for (const QUrl &scope : resourceScopes) {
            if (!m_account->scopes().contains(scope)) {
                m_account->addScope(scope);
            }
        }

        updateAccountToken(m_account, job);
        return false;
    }

    if (_cancelTask) {
        cancelTask(job->errorString());
    }
    return false;
}

bool GoogleResource::canPerformTask()
{
    if (!m_account && accountId() == 0) {
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
    m_account = authJob->account();
    if (!m_accountMgr->storeAccount(m_account)) {
        qWarning() << "Failed to store account in KWallet";
    }

    KGAPI2::Job *otherJob = job->property(JOB_PROPERTY).value<KGAPI2::Job *>();
    if (otherJob) {
        otherJob->setAccount(m_account);
        otherJob->restart();
    }

    job->deleteLater();
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
    if (Settings::self()->enableIntervalCheck()) {
        cachePolicy.setInheritFromParent(false);
        cachePolicy.setIntervalCheckTime(Settings::self()->intervalCheckTime());
    }

    // Setting up root collection
    m_rootCollection = Collection();
    m_rootCollection.setContentMimeTypes({ Collection::mimeType(), Collection::virtualMimeType() });
    m_rootCollection.setRemoteId(ROOT_COLLECTION_REMOTEID);
    m_rootCollection.setName(account()->accountName());
    m_rootCollection.setParentCollection(Collection::root());
    m_rootCollection.setRights(Collection::CanCreateCollection);
    m_rootCollection.setCachePolicy(cachePolicy);

    EntityDisplayAttribute *attr = m_rootCollection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
    attr->setDisplayName(account()->accountName());
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

    bool found = false;
    for (auto handler : m_handlers) {
        if (collection.contentMimeTypes().contains(handler->mimetype())) {
            handler->retrieveItems(collection);
            found = true;
            break;
        }
    }
    if (!found) {
        qCWarning(GOOGLE_LOG) << "Unknown collection" << collection.name();
        itemsRetrieved({});
    }
}

void GoogleResource::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }

    if (collection.parentCollection() == Akonadi::Collection::root()) {
        cancelTask(i18n("The top-level collection cannot contain anything"));
        return;
    }

    bool found = false;
    for (auto handler : m_handlers) {
        if (collection.contentMimeTypes().contains(handler->mimetype())
                && handler->canPerformTask(item)) {
            handler->itemAdded(item, collection);
            found = true;
            break;
        }
    }
    if (!found) {
        cancelTask(i18n("Invalid payload type"));
    }
}

void GoogleResource::itemChanged(const Akonadi::Item &item, const QSet< QByteArray > &partIdentifiers)
{
    Q_UNUSED(partIdentifiers);
    if (!canPerformTask()) {
        return;
    }

    bool found = false;
    for (auto handler : m_handlers) {
        if (handler->canPerformTask(item)) {
            handler->itemChanged(item, partIdentifiers);
            found = true;
            break;
        }
    }
    if (!found) {
        cancelTask(i18n("Invalid payload type"));
    }
}

void GoogleResource::itemRemoved(const Akonadi::Item &item)
{
    if (!canPerformTask()) {
        return;
    }

    bool found = false;
    for (auto handler : m_handlers) {
        if (item.mimeType() == handler->mimetype()) {
            handler->itemRemoved(item);
            found = true;
            break;
        }
    }
    if (!found) {
        cancelTask(i18n("Invalid payload type"));
    }
}

void GoogleResource::itemMoved(const Akonadi::Item &item, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination)
{
    if (!canPerformTask()) {
        return;
    }

    if (collectionDestination.parentCollection() == Akonadi::Collection::root()) {
        cancelTask(i18n("The top-level collection cannot contain anything"));
        return;
    }

    bool found = false;
    for (auto handler : m_handlers) {
        if (handler->canPerformTask(item)) {
            handler->itemMoved(item, collectionSource, collectionDestination);
            found = true;
            break;
        }
    }
    if (!found) {
        cancelTask(i18n("Invalid payload type"));
    }
}

void GoogleResource::itemLinked(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }

    bool found = false;
    for (auto handler : m_handlers) {
        if (handler->canPerformTask(item)) {
            handler->itemLinked(item, collection);
            found = true;
            break;
        }
    }
    if (!found) {
        cancelTask(i18n("Invalid payload type"));
    }
}

void GoogleResource::itemUnlinked(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }

    bool found = false;
    for (auto handler : m_handlers) {
        if (handler->canPerformTask(item)) {
            handler->itemUnlinked(item, collection);
            found = true;
            break;
        }
    }
    if (!found) {
        cancelTask(i18n("Invalid payload type"));
    }
}

void GoogleResource::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    if (!canPerformTask()) {
        return;
    }
}

void GoogleResource::collectionChanged(const Akonadi::Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }
}

void GoogleResource::collectionRemoved(const Akonadi::Collection &collection)
{
    if (!canPerformTask()) {
        return;
    }
}

AKONADI_RESOURCE_MAIN(GoogleResource)
