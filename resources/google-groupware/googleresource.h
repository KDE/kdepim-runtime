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

#include "calendarhandler.h"
#include "generichandler.h"


#define JOB_PROPERTY "_KGAPI2Job"

namespace KGAPI2 {
class Job;
}

class GoogleSettings;
class GoogleResourceState;

class GoogleResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV3, public Akonadi::FreeBusyProviderBase
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

    bool canPerformTask();
    /**
     * KAccounts support abstraction.
     *
     * Returns 0 when compiled without KAccounts or not configured for KAccounts
     */
    int accountId() const;

    void emitReadyStatus();
    void collectionsRetrievedFromHandler(const Akonadi::Collection::List &collections);
protected Q_SLOTS:
    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &collection) override;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet< QByteArray > &partIdentifiers) override;
    void itemsRemoved(const Akonadi::Item::List &items) override;
    void itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &collectionSource, const Akonadi::Collection &collectionDestination) override;
    void itemsLinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection) override;
    void itemsUnlinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection) override;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override;
    void collectionChanged(const Akonadi::Collection &collection) override;
    void collectionRemoved(const Akonadi::Collection &collection) override;

    bool handleError(KGAPI2::Job *job, bool cancelTask = true);

    void slotAuthJobFinished(KGAPI2::Job *job);
private:
    bool m_isConfiguring = false;
    GoogleSettings *m_settings = nullptr;
    Akonadi::Collection m_rootCollection;

    GoogleResourceState *m_iface;

    std::vector<GenericHandler::Ptr> m_handlers;
    FreeBusyHandler::Ptr m_freeBusyHandler;
    int m_jobs;

    friend class GoogleSettingsDialog;
    friend class GoogleResourceState;

    GenericHandler *fetchHandlerByMimetype(const QString &mimeType);
    GenericHandler *fetchHandlerForCollection(const Akonadi::Collection &collection);
};

#endif // GOOGLERESOURCE_H
