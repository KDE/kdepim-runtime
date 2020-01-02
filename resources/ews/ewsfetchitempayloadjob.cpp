/*
    SPDX-FileCopyrightText: 2020 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsfetchitempayloadjob.h"

#include <KLocalizedString>

#include "ewsgetitemrequest.h"
#include "ewsitemhandler.h"

#include "ewsresource_debug.h"

using namespace Akonadi;

EwsFetchItemPayloadJob::EwsFetchItemPayloadJob(EwsClient &client, QObject *parent, const Akonadi::Item::List &items)
    : EwsJob(parent)
    , mItems(items)
    , mClient(client)
{
}

void EwsFetchItemPayloadJob::start()
{
    auto req = new EwsGetItemRequest(mClient, this);
    EwsId::List ids;
    ids.reserve(mItems.count());
    for (const Item &item : std::as_const(mItems)) {
        ids << EwsId(item.remoteId(), item.remoteRevision());
    }
    req->setItemIds(ids);
    EwsItemShape shape(EwsShapeIdOnly);
    shape << EwsPropertyField(QStringLiteral("item:MimeContent"));
    req->setItemShape(shape);
    connect(req, &EwsGetItemRequest::result, this, &EwsFetchItemPayloadJob::itemFetchFinished);
    req->start();
}

void EwsFetchItemPayloadJob::itemFetchFinished(KJob *job)
{
    if (!success) {
        setErrorText(i18nc("@info:status", "Failed to process items retrieval request"));
        emitResult();
        return;
    }
    auto req = qobject_cast<EwsGetItemRequest *>(job);
    if (!req) {
        qCWarning(EWSRES_LOG) << QStringLiteral("Invalid EwsGetItemRequest job object");
        setErrorText(i18nc("@info:status", "Failed to retrieve items - internal error"));
        emitResult();
        return;
    }

    QHash<QString, Item> itemHash;
    itemHash.reserve(mItems.count());
    for (const Item &item : mItems) {
        itemHash.insert(item.remoteId(), item);
    }

    const EwsGetItemRequest::Response &resp = req->responses()[0];
    if (!resp.isSuccess()) {
        qCWarningNC(EWSRES_AGENTIF_LOG) << QStringLiteral("retrieveItems: Item fetch failed.");
        setErrorText(i18nc("@info:status", "Failed to retrieve items"));
        emitResult();
        return;
    }

    if (mItems.size() != req->responses().size()) {
        qCWarningNC(EWSRES_AGENTIF_LOG) << QStringLiteral("retrieveItems: incorrect number of responses.");
        setErrorText(i18nc("@info:status", "Failed to retrieve items - incorrect number of responses"));
        emitResult();
        return;
    }

    Q_FOREACH (const EwsGetItemRequest::Response &resp, req->responses()) {
        const EwsItem &ewsItem = resp.item();
        auto id = ewsItem[EwsItemFieldItemId].value<EwsId>();
        auto it = itemHash.find(id.id());
        if (it == itemHash.end()) {
            qCWarningNC(EWSRES_AGENTIF_LOG) << QStringLiteral("retrieveItems: Akonadi item not found for item %1.").arg(id.id());
            setErrorText(i18nc("@info:status", "Failed to retrieve items - Akonadi item not found for item %1", id.id()));
            emitResult();
            return;
        }
        EwsItemType type = ewsItem.internalType();
        if (type == EwsItemTypeUnknown) {
            qCWarningNC(EWSRES_AGENTIF_LOG) << QStringLiteral("retrieveItems: Unknown item type for item %1!").arg(id.id());
            setErrorText(i18nc("@info:status", "Failed to retrieve items - Unknown item type for item %1", id.id()));
            emitResult();
            return;
        }
        if (!EwsItemHandler::itemHandler(type)->setItemPayload(*it, ewsItem)) {
            qCWarningNC(EWSRES_AGENTIF_LOG) << "retrieveItems: Failed to fetch item payload";
            setErrorText(i18nc("@info:status", "Failed to fetch item payload"));
            emitResult();
            return;
        }
    }

    mResultItems = itemHash.values().toVector();

    emitResult();
}
