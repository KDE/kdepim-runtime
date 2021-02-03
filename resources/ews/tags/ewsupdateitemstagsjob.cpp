/*
    SPDX-FileCopyrightText: 2015-2016 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsupdateitemstagsjob.h"

#include <AkonadiCore/AttributeFactory>
#include <AkonadiCore/Tag>
#include <AkonadiCore/TagAttribute>
#include <AkonadiCore/TagFetchJob>
#include <AkonadiCore/TagFetchScope>

#include "ewsclient.h"
#include "ewsglobaltagswritejob.h"
#include "ewsid.h"
#include "ewsitemhandler.h"
#include "ewsresource.h"
#include "ewstagstore.h"
#include "ewsupdateitemrequest.h"

using namespace Akonadi;

EwsUpdateItemsTagsJob::EwsUpdateItemsTagsJob(const Akonadi::Item::List &items, EwsTagStore *tagStore, EwsClient &client, EwsResource *parent)
    : EwsJob(parent)
    , mItems(items)
    , mTagStore(tagStore)
    , mClient(client)
{
}

EwsUpdateItemsTagsJob::~EwsUpdateItemsTagsJob()
{
}

void EwsUpdateItemsTagsJob::start()
{
    Tag::List unknownTags;

    /* In the Exchange world it is not possible to add or remove individual tags from an item
     * - it is necessary to write the full list of tags at once. Unfortunately the tags objects
     * attached to an item only contain the id. They're missing any persistent identification such
     * as the uid. If the EWS resource hasn't seen these tags yet it is necessary to fetch them
     * first before any further processing.
     */
    for (const Item &item : qAsConst(mItems)) {
        Q_FOREACH (const Tag &tag, item.tags()) {
            if (!mTagStore->containsId(tag.id())) {
                unknownTags.append(tag);
            }
        }
    }

    if (!unknownTags.empty()) {
        auto job = new TagFetchJob(unknownTags, this);
        job->fetchScope().setFetchRemoteId(true);
        connect(job, &TagFetchJob::result, this, &EwsUpdateItemsTagsJob::itemsTagsChangedTagsFetched);
    } else {
        doUpdateItemsTags();
    }
}

void EwsUpdateItemsTagsJob::itemsTagsChangedTagsFetched(KJob *job)
{
    if (job->error()) {
        setErrorMsg(job->errorString());
        emitResult();
        return;
    }

    auto tagJob = qobject_cast<TagFetchJob *>(job);
    if (!tagJob) {
        setErrorMsg(QStringLiteral("Invalid TagFetchJob job object"));
        emitResult();
        return;
    }

    /* All unknown tags have been fetched and can be written to Exchange. */
    mTagStore->addTags(tagJob->tags());

    auto res = qobject_cast<EwsResource *>(parent());
    Q_ASSERT(res);
    auto writeJob = new EwsGlobalTagsWriteJob(mTagStore, mClient, res->rootCollection(), this);
    connect(writeJob, &EwsGlobalTagsWriteJob::result, this, &EwsUpdateItemsTagsJob::globalTagsWriteFinished);
    writeJob->start();
}

void EwsUpdateItemsTagsJob::globalTagsWriteFinished(KJob *job)
{
    if (job->error()) {
        setErrorMsg(job->errorString());
        emitResult();
        return;
    }

    doUpdateItemsTags();
}

void EwsUpdateItemsTagsJob::doUpdateItemsTags()
{
    auto req = new EwsUpdateItemRequest(mClient, this);
    Q_FOREACH (const Item &item, mItems) {
        EwsUpdateItemRequest::ItemChange ic(EwsId(item.remoteId(), item.remoteRevision()), EwsItemHandler::mimeToItemType(item.mimeType()));
        if (!item.tags().isEmpty()) {
            QStringList tagList;
            QStringList categoryList;
            tagList.reserve(item.tags().count());
            Q_FOREACH (const Tag &tag, item.tags()) {
                Q_ASSERT(mTagStore->containsId(tag.id()));
                tagList.append(QString::fromLatin1(mTagStore->tagRemoteId(tag.id())));
                QString name = mTagStore->tagName(tag.id());
                if (!name.isEmpty()) {
                    categoryList.append(name);
                }
            }
            EwsUpdateItemRequest::Update *upd = new EwsUpdateItemRequest::SetUpdate(EwsResource::tagsProperty, tagList);
            ic.addUpdate(upd);
            upd = new EwsUpdateItemRequest::SetUpdate(EwsPropertyField(QStringLiteral("item:Categories")), categoryList);
            ic.addUpdate(upd);
        } else {
            EwsUpdateItemRequest::Update *upd = new EwsUpdateItemRequest::DeleteUpdate(EwsResource::tagsProperty);
            ic.addUpdate(upd);
            upd = new EwsUpdateItemRequest::DeleteUpdate(EwsPropertyField(QStringLiteral("item:Categories")));
            ic.addUpdate(upd);
        }
        req->addItemChange(ic);
    }

    connect(req, &EwsUpdateItemRequest::result, this, &EwsUpdateItemsTagsJob::updateItemsTagsRequestFinished);
    req->start();
}

void EwsUpdateItemsTagsJob::updateItemsTagsRequestFinished(KJob *job)
{
    if (job->error()) {
        setErrorMsg(job->errorString());
        emitResult();
        return;
    }

    auto req = qobject_cast<EwsUpdateItemRequest *>(job);
    if (!req) {
        setErrorMsg(QStringLiteral("Invalid EwsUpdateItemRequest job object"));
        return;
    }

    Q_ASSERT(mItems.count() == req->responses().count());

    auto itemIt = mItems.begin();
    Q_FOREACH (const EwsUpdateItemRequest::Response &resp, req->responses()) {
        if (resp.isSuccess()) {
            itemIt->setRemoteRevision(resp.itemId().changeKey());
        }
    }

    emitResult();
}
