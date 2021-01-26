/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef EWSGLOBALTAGSREADJOB_H
#define EWSGLOBALTAGSREADJOB_H

#include "ewsjob.h"

#include <AkonadiCore/Tag>

class EwsTagStore;
class EwsClient;
namespace Akonadi
{
class Collection;
}

class EwsGlobalTagsReadJob : public EwsJob
{
    Q_OBJECT
public:
    EwsGlobalTagsReadJob(EwsTagStore *tagStore, EwsClient &client, const Akonadi::Collection &rootCollection, QObject *parent);
    ~EwsGlobalTagsReadJob() override;

    void start() override;

    const Akonadi::Tag::List &tags() const
    {
        return mTags;
    }

private Q_SLOTS:
    void getFolderRequestFinished(KJob *job);

private:
    EwsTagStore *mTagStore = nullptr;
    EwsClient &mClient;
    const Akonadi::Collection &mRootCollection;
    Akonadi::Tag::List mTags;
};

#endif
