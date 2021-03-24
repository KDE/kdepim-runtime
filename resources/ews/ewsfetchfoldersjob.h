/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QScopedPointer>

#include <AkonadiCore/Collection>

#include "ewsfolder.h"
#include "ewsjob.h"

class EwsClient;
class EwsFetchFoldersJobPrivate;

class EwsFetchFoldersJob : public EwsJob
{
    Q_OBJECT
public:
    EwsFetchFoldersJob(EwsClient &client, const Akonadi::Collection &rootCollection, QObject *parent);
    ~EwsFetchFoldersJob() override;

    Akonadi::Collection::List folders() const
    {
        return mFolders;
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
    Akonadi::Collection::List mFolders;

    QString mSyncState;

    QScopedPointer<EwsFetchFoldersJobPrivate> d_ptr;
    Q_DECLARE_PRIVATE(EwsFetchFoldersJob)
};

