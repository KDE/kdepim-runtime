/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "graphfetchitemsjob.h"

#include "graphclient/graphrequest.h"

#include <Akonadi/MessageFlags>
#include <KMime/Message>
#include <QJsonArray>
#include <QJsonObject>

using namespace Akonadi;

GraphFetchItemsJob::GraphFetchItemsJob(GraphClient &client, const Collection &collection, const QString &deltaLink, QObject *parent)
    : KJob(parent)
    , mClient(client)
    , mCollection(collection)
    , mDeltaLink(deltaLink)
{
}

void GraphFetchItemsJob::start()
{
    auto req = new GraphRequest(mClient, this);
    if (!mDeltaLink.isEmpty()) {
        req->setAbsoluteUrl(QUrl(mDeltaLink));
    } else {
        // Fetch the envelope fields so the message list works without touching the
        // server again (mirrors EwsFetchMailDetailJob); the body comes via /$value.
        // $top=200: delta defaults to ~10 items per page, which turns a big mailbox
        // into thousands of round trips. 200 is Graph's maximum for messages.
        req->setPath(QStringLiteral("/me/mailFolders/%1/messages/delta"
                                    "?$top=200&$select=id,isRead,flag,subject,from,toRecipients,ccRecipients,receivedDateTime,internetMessageId")
                         .arg(mCollection.remoteId()));
    }
    connect(req, &KJob::result, this, &GraphFetchItemsJob::onRequestFinished);
    req->start();
}

static void addAddresses(KMime::Headers::Generics::AddressList *header, const QJsonArray &recipients)
{
    for (const auto &r : recipients) {
        const QJsonObject addr = r.toObject().value(QLatin1String("emailAddress")).toObject();
        const QString address = addr.value(QLatin1String("address")).toString();
        if (!address.isEmpty()) {
            header->addAddress(address.toUtf8(), addr.value(QLatin1String("name")).toString());
        }
    }
}

Item GraphFetchItemsJob::itemStubFromJson(const QJsonObject &message) const
{
    Item item(KMime::Message::mimeType());
    item.setRemoteId(message.value(QLatin1String("id")).toString());
    item.setParentCollection(mCollection);

    if (message.value(QLatin1String("isRead")).toBool()) {
        item.setFlag(Akonadi::MessageFlags::Seen);
    }
    const QJsonObject flag = message.value(QLatin1String("flag")).toObject();
    if (flag.value(QLatin1String("flagStatus")).toString() == QLatin1String("flagged")) {
        item.setFlag(Akonadi::MessageFlags::Flagged);
    }

    // Header-only payload (no body): the Akonadi MIME serializer registers this as
    // ENVELOPE/HEAD parts only, so the message list has subject/sender/date while a
    // full-body request still reaches retrieveItems() -> /$value.
    auto msg = std::make_shared<KMime::Message>();
    msg->subject()->fromUnicodeString(message.value(QLatin1String("subject")).toString());
    const QJsonObject from = message.value(QLatin1String("from")).toObject().value(QLatin1String("emailAddress")).toObject();
    const QString fromAddress = from.value(QLatin1String("address")).toString();
    if (!fromAddress.isEmpty()) {
        msg->from()->addAddress(fromAddress.toUtf8(), from.value(QLatin1String("name")).toString());
    }
    addAddresses(msg->to(), message.value(QLatin1String("toRecipients")).toArray());
    addAddresses(msg->cc(), message.value(QLatin1String("ccRecipients")).toArray());
    const auto received = QDateTime::fromString(message.value(QLatin1String("receivedDateTime")).toString(), Qt::ISODate);
    if (received.isValid()) {
        msg->date()->setDateTime(received);
    }
    const QString messageId = message.value(QLatin1String("internetMessageId")).toString();
    if (!messageId.isEmpty()) {
        msg->messageID()->from7BitString(messageId.toLatin1());
    }
    msg->assemble();
    item.setPayload(msg);

    return item;
}

void GraphFetchItemsJob::onRequestFinished(KJob *job)
{
    auto *req = qobject_cast<GraphRequest *>(job);
    if (job->error()) {
        if ((req->httpStatus() == 410 || req->graphErrorCode() == QLatin1String("ErrorInvalidSyncStateData")) && !mDeltaLink.isEmpty()) {
            // The stored delta token expired (HTTP 410 Gone) or the server no longer
            // accepts its state (HTTP 400 ErrorInvalidSyncStateData) — drop it and
            // rerun the delta from scratch. Delivery is incremental, so this only re-lists.
            mDeltaLink.clear();
            start();
            return;
        }
        setError(job->error());
        setErrorText(job->errorText());
        emitResult();
        return;
    }
    mDeltaLink = req->deltaLink();

    for (const auto &v : req->aggregatedValue()) {
        const QJsonObject message = v.toObject();
        if (message.contains(QLatin1String("@removed"))) {
            Item item;
            item.setRemoteId(message.value(QLatin1String("id")).toString());
            mRemoved.append(item);
        } else {
            mChanged.append(itemStubFromJson(message));
        }
    }
    emitResult();
}

Collection GraphFetchItemsJob::collection() const
{
    return mCollection;
}

Item::List GraphFetchItemsJob::changedItems() const
{
    return mChanged;
}

Item::List GraphFetchItemsJob::removedItems() const
{
    return mRemoved;
}
QString GraphFetchItemsJob::deltaLink() const
{
    return mDeltaLink;
}

#include "moc_graphfetchitemsjob.cpp"
