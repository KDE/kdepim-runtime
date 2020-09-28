/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ETESYNCRESOURCE_H
#define ETESYNCRESOURCE_H

#include <AkonadiAgentBase/ResourceBase>
#include <KLocalizedString>

#include "calendarhandler.h"
#include "contacthandler.h"
#include "etesyncadapter.h"
#include "etesyncclientstate.h"
#include "taskhandler.h"

class EteSyncResource : public Akonadi::ResourceBase, public Akonadi::AgentBase::ObserverV2
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
    void collectionChanged(const Akonadi::Collection &collection) override;
    void collectionRemoved(const Akonadi::Collection &collection) override;

    void configure(WId windowId) override;

protected:
    void aboutToQuit() override;

    void initialise();

    void setupCollection(Akonadi::Collection &collection, EteSyncJournal *journal);

    Collection createRootCollection();

    void initialiseDirectory(const QString &path) const;
    QString baseDirectoryPath() const;
    QString collectionsCacheDirectoryPath() const;
    QString itemsCacheDirectoryPath() const;
    EtebaseCollectionPtr getEtesyncCollectionFromCache(const QString &collectionUid);
    EtebaseItemPtr getEtesyncItemFromCache(const QString &itemUid, const EtebaseCollection *parentCollection);

    bool handleError();
    bool handleError(int errorCode);
    bool credentialsRequired();

    const EteSyncJournalPtr &getJournal(const QString &journalUid)
    {
        return mJournalsCache[journalUid];
    }

private Q_SLOTS:
    void onReloadConfiguration();
    void initialiseDone(bool successful);
    void slotItemsRetrieved(KJob *job);
    void slotCollectionsRetrieved(KJob *job);
    void slotTokenRefreshed(bool successful);
    void showErrorDialog(const QString &errorText, const QString &errorDetails = QString(), const QString &title = i18n("Something went wrong"));

private:
    EteSyncClientState::Ptr mClientState;
    std::vector<BaseHandler::Ptr> mHandlers;
    std::map<QString, EteSyncJournalPtr> mJournalsCache;
    QDateTime mJournalsCacheUpdateTime;
    bool mCredentialsRequired = false;
    Akonadi::Collection mRootCollection;

    ContactHandler::Ptr mContactHandler;
    CalendarHandler::Ptr mCalendarHandler;
    TaskHandler::Ptr mTaskHandler;

    friend class ContactHandler;
    friend class CalendarTaskBaseHandler;
    friend class BaseHandler;

    BaseHandler *fetchHandlerForMimeType(const QString &mimeType);
    BaseHandler *fetchHandlerForCollection(const Akonadi::Collection &collection);
};

#endif
