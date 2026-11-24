/*
    SPDX-FileCopyrightText: 2020 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "ewsfetchitempayloadjob.h"

#include <KLocalizedString>
#include <QSet>

#include "ewsitemhandler.h"

#include "ewsresource_debug.h"

using namespace Akonadi;

constexpr unsigned ChunkSize = 10;

EwsFetchItemPayloadJob::EwsFetchItemPayloadJob(EwsClient &client, QObject *parent, const Akonadi::Item::List &items)
    : EwsJob(parent)
    , mItems(items)
    , mClient(client)
    , mChunkedJob(ChunkSize)
{
}

void EwsFetchItemPayloadJob::start()
{
    EwsId::List ids;
    ids.reserve(mItems.count());
    for (const Item &item : std::as_const(mItems)) {
        ids << EwsId(item.remoteId(), item.remoteRevision());
    }

    // Properties that the handlers of the requested item types need besides the MIME content.
    QSet<EwsItemType> types;
    for (const Item &item : std::as_const(mItems)) {
        types.insert(EwsItemHandler::mimeToItemType(item.mimeType()));
    }
    QList<EwsPropertyField> properties{EwsPropertyField(QStringLiteral("item:MimeContent"))};
    for (const EwsItemType type : std::as_const(types)) {
        if (EwsItemHandler *handler = (type != EwsItemTypeItem) ? EwsItemHandler::itemHandler(type) : nullptr) {
            const auto handlerProperties = handler->payloadProperties();
            for (const EwsPropertyField &property : handlerProperties) {
                if (!properties.contains(property)) {
                    properties.append(property);
                }
            }
        }
    }

    mChunkedJob.setItems(ids);
    mChunkedJob.start(
        [this, properties](const EwsId::List::const_iterator &firstId, const EwsId::List::const_iterator &lastId) {
            auto req = new EwsGetItemRequest(mClient, this);
            EwsId::List ids;
            for (auto it = firstId; it != lastId; ++it) {
                ids.append(*it);
            }
            req->setItemIds(ids);
            EwsItemShape shape(EwsShapeIdOnly);
            for (const EwsPropertyField &property : properties) {
                shape << property;
            }
            req->setItemShape(shape);
            return req;
        },
        [](EwsGetItemRequest *req) {
            return req->responses();
        },
        [this](unsigned int progress) {
            Q_EMIT reportPercent(progress);
        },
        [this](bool success, const QString &error) {
            itemFetchFinished(success, error);
        });
}

void EwsFetchItemPayloadJob::itemFetchFinished(bool success, const QString & /*error*/)
{
    if (!success) {
        setErrorText(i18nc("@info:status", "Failed to process items retrieval request"));
        emitResult();
        return;
    }

    const EwsGetItemRequest::Response &firstResp = mChunkedJob.responses()[0];
    if (!firstResp.isSuccess()) {
        qCWarningNC(EWSRES_AGENTIF_LOG) << QStringLiteral("retrieveItems: Item fetch failed.");
        setErrorText(i18nc("@info:status", "Failed to retrieve items"));
        emitResult();
        return;
    }

    if (mItems.size() != mChunkedJob.responses().size()) {
        qCWarningNC(EWSRES_AGENTIF_LOG) << QStringLiteral("retrieveItems: incorrect number of responses.");
        setErrorText(i18nc("@info:status", "Failed to retrieve items - incorrect number of responses"));
        emitResult();
        return;
    }

    /* In general EWS guarantees that the order of response items will match the order of request items.
     * It is therefore safe to iterate these in parallel. */
    auto it = mItems.begin();
    for (const auto &resp : mChunkedJob.responses()) {
        const EwsItem &ewsItem = resp.item();
        auto id = ewsItem[EwsItemFieldItemId].value<EwsId>();
        if (it->remoteId() != id.id()) {
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
            /* Leave the item out instead of failing the whole request, which would block the
             * retrieval of the other items requested along with it. The item stays without
             * payload. */
            qCWarningNC(EWSRES_AGENTIF_LOG) << QStringLiteral("retrieveItems: Failed to set the payload of item %1 - skipping").arg(ewsHash(id.id()));
            it = mItems.erase(it);
            continue;
        }
        ++it;
    }

    emitResult();
}

#include "moc_ewsfetchitempayloadjob.cpp"
