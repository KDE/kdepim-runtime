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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef GOOGLERESOURCE_H
#define GOOGLERESOURCE_H

#include <AkonadiAgentBase/ResourceBase>
#include <AkonadiAgentBase/AgentBase>

#include <qwindowdefs.h>

#include "googleaccountmanager.h"

#include <KGAPI/Types>

#include <KLocalizedString>

#define ITEM_PROPERTY "_AkonadiItem"
#define ITEMLIST_PROPERTY "_AkonadiItemList"
#define COLLECTION_PROPERTY "_AkonadiCollection"
#define JOB_PROPERTY "_KGAPI2Job"

namespace KGAPI2 {
class Job;
}

class GoogleSettings;

class GoogleResource : public Akonadi::ResourceBase
{
    Q_OBJECT

public:
    explicit GoogleResource(const QString &id);
    ~GoogleResource() override;

    virtual GoogleSettings *settings() const = 0;
    virtual QList<QUrl> scopes() const = 0;

    void cleanup() override;

public Q_SLOTS:
    void configure(WId windowId) override;

    void reloadConfig();

protected Q_SLOTS:
    bool retrieveItem(const Akonadi::Item &item, const QSet< QByteArray > &parts) override;

    bool handleError(KGAPI2::Job *job, bool cancelTask = true);

    virtual void slotAuthJobFinished(KGAPI2::Job *job);
    virtual void slotGenericJobFinished(KGAPI2::Job *job);

    void emitPercent(KGAPI2::Job *job, int processedCount, int totalCount);

    virtual void slotAbortRequested();
    virtual void slotAccountManagerReady(bool success);
    virtual void slotAccountChanged(const KGAPI2::AccountPtr &account);
    virtual void slotAccountRemoved(const QString &accountName);

protected:
    bool configureKGAPIAccount(const KGAPI2::AccountPtr &account);
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

    template<typename T>
    bool canPerformTask(const Akonadi::Item::List &items, const QString &mimeType = {}) {
        for (const auto &item : items) {
            if (!canPerformTask<T>(item, mimeType)) {
                return false;
            }
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

    void aboutToQuit() override;

    virtual int runConfigurationDialog(WId windowId) = 0;
    virtual void updateResourceName() = 0;

private:
    void abort();

    bool m_isConfiguring = false;
    GoogleAccountManager *m_accountMgr = nullptr;
    KGAPI2::AccountPtr m_account;
};

#endif // GOOGLERESOURCE_H
