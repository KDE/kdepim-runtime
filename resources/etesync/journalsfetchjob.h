/*
 * SPDX-FileCopyrightText: 2020 Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef ETESYNCJOURNALSFETCHJOB_H
#define ETESYNCJOURNALSFETCHJOB_H

#include <KJob>

#include "etebaseadapter.h"
#include "etesyncadapter.h"

#include <AkonadiCore/Collection>

namespace EteSyncAPI {
class JournalsFetchJob : public KJob
{
    Q_OBJECT

public:
    explicit JournalsFetchJob(const EtebaseAccount *account, const Akonadi::Collection &resourceCollection, const QString &cacheDir = QString(), QObject *parent = nullptr);

    void start() override;

    Akonadi::Collection::List collections() const
    {
        return mCollections;
    }

    Akonadi::Collection::List removedCollections() const
    {
        return mRemovedCollections;
    }

    QString syncToken() const
    {
        return mSyncToken;
    }

protected:
    void fetchJournals();
    void setupCollection(Akonadi::Collection &collection, const EtebaseCollection *etesyncCollection);

private:
    const EtebaseAccount *mAccount = nullptr;
    Akonadi::Collection::List mCollections;
    Akonadi::Collection::List mRemovedCollections;
    const Akonadi::Collection mResourceCollection;
    QString mSyncToken;
    QString mCacheDir;
};
} // namespace EteSyncAPI

#endif
