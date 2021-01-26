/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EWSGLOBALTAGSWRITEJOB_H
#define EWSGLOBALTAGSWRITEJOB_H

#include "ewsjob.h"

class EwsTagStore;
class EwsClient;
namespace Akonadi
{
class Collection;
}

class EwsGlobalTagsWriteJob : public EwsJob
{
    Q_OBJECT
public:
    EwsGlobalTagsWriteJob(EwsTagStore *tagStore, EwsClient &client, const Akonadi::Collection &rootCollection, QObject *parent);
    ~EwsGlobalTagsWriteJob() override;

    void start() override;
private Q_SLOTS:
    void updateFolderRequestFinished(KJob *job);

private:
    EwsTagStore *mTagStore = nullptr;
    EwsClient &mClient;
    const Akonadi::Collection &mRootCollection;
};

#endif
