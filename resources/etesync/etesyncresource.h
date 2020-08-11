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

#ifndef ETESYNCRESOURCE_H
#define ETESYNCRESOURCE_H

#include <kcontacts/addressee.h>
#include <kcontacts/vcardconverter.h>

#include <AkonadiAgentBase/ResourceBase>

#include "calendarhandler.h"
#include "contacthandler.h"
#include "etesyncadapter.h"
#include "etesyncclientstate.h"
#include "taskhandler.h"

class EteSyncResource : public Akonadi::ResourceBase,
                        public Akonadi::AgentBase::ObserverV2
{
    Q_OBJECT

public:
    explicit EteSyncResource(const QString &id);
    ~EteSyncResource() override = default;

    void cleanup() override;
Q_SIGNALS:

    void clientInitialised(bool successful);

protected Q_SLOTS:
    void retrieveCollections() override;
    void retrieveItems(const Akonadi::Collection &col) override;

    void itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection) override;
    void itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &parts) override;
    void itemRemoved(const Akonadi::Item &item) override;

    void collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent) override;
    void collectionChanged(const Akonadi::Collection &collection);
    void collectionRemoved(const Akonadi::Collection &collection);

    void configure(WId windowId) override;

protected:
    void aboutToQuit() override;

    void initialise();

    void setupCollection(Akonadi::Collection &collection, EteSyncJournal *journal);

    void initialiseDirectory(const QString &path) const;

    QString getLocalContact(QString contactUid) const;

    void updateLocalContact(const KContacts::Addressee &contact);

    void deleteLocalContact(const KContacts::Addressee &contact);

    QString baseDirectoryPath() const;

    bool handleTokenError();

    const EteSyncJournalPtr &getJournal(QString journalUid)
    {
        return mJournalsCache[journalUid];
    }

private Q_SLOTS:
    void onReloadConfiguration();
    void initialiseDone(bool successful);
    void slotItemsRetrieved(KJob *job);
    void slotCollectionsRetrieved(KJob *job);

private:
    Akonadi::Collection mResourceCollection;
    EteSyncClientState *mClientState = nullptr;
    std::vector<BaseHandler::Ptr> mHandlers;
    std::map<QString, EteSyncJournalPtr> mJournalsCache;
    QDateTime mJournalsCacheUpdateTime;

    ContactHandler::Ptr mContactHandler = nullptr;
    CalendarHandler::Ptr mCalendarHandler = nullptr;
    TaskHandler::Ptr mTaskHandler = nullptr;

    friend class ContactHandler;
    friend class CalendarTaskBaseHandler;
    friend class BaseHandler;

    BaseHandler *fetchHandlerForMimeType(const QString &mimeType);
    BaseHandler *fetchHandlerForCollection(const Akonadi::Collection &collection);
};

#endif
