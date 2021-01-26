/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EWSAKONADITAGSSYNCJOB_H
#define EWSAKONADITAGSSYNCJOB_H

#include "ewsjob.h"

class EwsTagStore;
class EwsClient;
namespace Akonadi
{
class Collection;
}

class EwsAkonadiTagsSyncJob : public EwsJob
{
    Q_OBJECT
public:
    EwsAkonadiTagsSyncJob(EwsTagStore *tagStore, EwsClient &client, const Akonadi::Collection &rootCollection, QObject *parent);
    ~EwsAkonadiTagsSyncJob() override;

    void start() override;

private Q_SLOTS:
    void tagFetchFinished(KJob *job);
    void tagWriteFinished(KJob *job);

private:
    EwsTagStore *mTagStore = nullptr;
    EwsClient &mClient;
    const Akonadi::Collection &mRootCollection;
};

#endif
