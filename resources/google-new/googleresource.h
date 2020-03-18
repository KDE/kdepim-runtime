/*
   Copyright (C) 2011-2013 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef GOOGLERESOURCE_H
#define GOOGLERESOURCE_H

#include <AkonadiAgentBase/ResourceBase>
#include <AkonadiAgentBase/AgentBase>
#include <Akonadi/Calendar/FreeBusyProviderBase>

#include <qwindowdefs.h>

#include "googleaccountmanager.h"
#include "generichandler.h"

#include <KGAPI/Types>

#include <KLocalizedString>

#define ITEM_PROPERTY "_AkonadiItem"
#define COLLECTION_PROPERTY "_AkonadiCollection"
#define JOB_PROPERTY "_KGAPI2Job"

namespace KGAPI2 {
class Job;
}

class GoogleSettings;

class GoogleResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV2, public Akonadi::FreeBusyProviderBase
{
    Q_OBJECT

public:
    explicit GoogleResource(const QString &id);
    ~GoogleResource() override;

    QList<QUrl> scopes() const;

    void cleanup() override;
public Q_SLOTS:
    void configure(WId windowId) override;
    void reloadConfig();
protected:
    int runConfigurationDialog(WId windowId);
    void updateResourceName();
    // Freebusy
    QDateTime lastCacheUpdate() const override;
    void canHandleFreeBusy(const QString &email) const override;
    void retrieveFreeBusy(const QString &email, const QDateTime &start, const QDateTime &end) override;
    void updateAccountToken(const KGAPI2::AccountPtr &account, KGAPI2::Job *restartJob = nullptr);

    template<typename T>
    bool canPerformTask(const Akonadi::Item &item, const QString &mimeType = QString())
    {
        if (item.isValid() && !item.hasPayload<T>()) {
            cancelTask(i18n("Invalid item payload."));
            return false;
        } else if (item.isValid() && mimeType != item.mimeType()) {
            cancelTask(i18n("Invalid payload MIME type. Expected %1, found %2", mimeType, item.mimeType()));
            return false;
        }

        return canPerformTask();
    }

    bool canPerformTask();

    KGAPI2::AccountPtr account() const;
    /**
     * KAccounts support abstraction.
     *
     * Returns 0 when compiled without KAccounts or not configured for KAccounts
     */
    int accountId() const;

    GoogleAccountManager *accountManager() const;
    Akonadi::Collection rootCollection() const;
protected Q_SLOTS:
    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet< QByteArray > &partIdentifiers) override;
    void itemRemoved(const Akonadi::Item &item) override;
    void itemMoved(const Akonadi::Item &item, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination) override;
    void itemLinked(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemUnlinked(const Akonadi::Item &item, const Akonadi::Collection &collection) override;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override;
    void collectionChanged(const Akonadi::Collection &collection) override;
    void collectionRemoved(const Akonadi::Collection &collection) override;

    bool handleError(KGAPI2::Job *job, bool cancelTask = true);
    void collectionsPartiallyRetrieved(const Akonadi::Collection::List &collections);

    virtual void slotAuthJobFinished(KGAPI2::Job *job);
    virtual void slotGenericJobFinished(KGAPI2::Job *job);
private:
    bool m_isConfiguring = false;
    GoogleAccountManager *m_accountMgr = nullptr;
    KGAPI2::AccountPtr m_account;
    Akonadi::Collection m_rootCollection;
    Akonadi::Collection::List m_collections;

    QList<GenericHandler::Ptr> m_handlers;
    int m_jobs;

    friend class CalendarHandler;
    friend class ContactHandler;
    friend class TaskHandler;
};

#endif // GOOGLERESOURCE_H
