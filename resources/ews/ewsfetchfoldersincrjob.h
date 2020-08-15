/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EWSFETCHFOLDERSINCRJOB_H
#define EWSFETCHFOLDERSINCRJOB_H

#include <QScopedPointer>

#include <AkonadiCore/Collection>

#include "ewsjob.h"
#include "ewsfolder.h"

class EwsClient;
class EwsFetchFoldersIncrJobPrivate;

class EwsFetchFoldersIncrJob : public EwsJob
{
    Q_OBJECT
public:
    EwsFetchFoldersIncrJob(EwsClient &client, const QString &syncState, const Akonadi::Collection &rootCollection, QObject *parent);
    ~EwsFetchFoldersIncrJob() override;

    Akonadi::Collection::List changedFolders() const
    {
        return mChangedFolders;
    }

    Akonadi::Collection::List deletedFolders() const
    {
        return mDeletedFolders;
    }

    const QString &syncState() const
    {
        return mSyncState;
    }

    void start() override;
Q_SIGNALS:
    void status(int status, const QString &message = QString());
    void percent(int progress);
private:
    Akonadi::Collection::List mChangedFolders;
    Akonadi::Collection::List mDeletedFolders;

    QString mSyncState;

    QScopedPointer<EwsFetchFoldersIncrJobPrivate> d_ptr;
    Q_DECLARE_PRIVATE(EwsFetchFoldersIncrJob)
};

#endif
