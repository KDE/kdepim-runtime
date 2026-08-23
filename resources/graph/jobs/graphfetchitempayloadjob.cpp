/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "graphfetchitempayloadjob.h"

#include "graphclient/graphrequest.h"

#include <KLocalizedString>
#include <KMime/Message>
#include <QJsonArray>
#include <QJsonObject>

using namespace Akonadi;

// Graph's /$batch accepts at most 20 sub-requests per call.
static constexpr int kBatchSize = 20;

GraphFetchItemPayloadJob::GraphFetchItemPayloadJob(GraphClient &client, const Item::List &items, QObject *parent)
    : KJob(parent)
    , mClient(client)
    , mItems(items)
{
}

void GraphFetchItemPayloadJob::start()
{
    if (mItems.isEmpty()) {
        emitResult();
        return;
    }
    fetchBatch();
}

void GraphFetchItemPayloadJob::fetchBatch()
{
    // Build one /$batch call for up to kBatchSize messages. The sub-request "id" is the
    // index into mItems so we can route each response back to the right item.
    const int first = mIndex;
    const int last = qMin(mIndex + kBatchSize, mItems.size());

    QJsonArray requests;
    for (int i = first; i < last; ++i) {
        QJsonObject sub;
        sub.insert(QStringLiteral("id"), QString::number(i));
        sub.insert(QStringLiteral("method"), QStringLiteral("GET"));
        sub.insert(QStringLiteral("url"), QStringLiteral("/me/messages/%1/$value").arg(mItems[i].remoteId()));
        requests.append(sub);
    }
    QJsonObject body;
    body.insert(QStringLiteral("requests"), requests);

    auto req = new GraphRequest(mClient, this);
    req->setMethod(GraphRequest::Method::Post);
    req->setPath(QStringLiteral("/$batch"));
    req->setBody(body);
    connect(req, &KJob::result, this, [this, req, last](KJob *job) {
        if (job->error()) {
            setError(job->error());
            setErrorText(job->errorText());
            emitResult();
            return;
        }
        const QJsonArray responses = req->responseObject().value(QLatin1String("responses")).toArray();
        for (const auto &r : responses) {
            const QJsonObject resp = r.toObject();
            const int idx = resp.value(QLatin1String("id")).toString().toInt();
            if (idx < 0 || idx >= mItems.size()) {
                continue;
            }
            const int status = resp.value(QLatin1String("status")).toInt();
            if (status < 200 || status >= 300) {
                setError(KJob::UserDefinedError);
                setErrorText(i18n("Failed to fetch the content of message %1 (HTTP %2)", mItems[idx].remoteId(), status));
                emitResult();
                return;
            }
            // /$value in a batch comes back as a base64-encoded string body.
            const QByteArray mime = QByteArray::fromBase64(resp.value(QLatin1String("body")).toString().toLatin1());
            applyMime(mItems[idx], mime);
        }

        mIndex = last;
        if (mIndex < mItems.size()) {
            fetchBatch();
        } else {
            emitResult();
        }
    });
    req->start();
}

void GraphFetchItemPayloadJob::applyMime(Item &item, const QByteArray &mime)
{
    auto msg = std::make_shared<KMime::Message>();
    // KMime expects LF line endings; Graph hands out raw CRLF MIME. Without this the
    // head/body split fails, the whole message lands in the head and KMail refetches
    // the "missing" body forever (cf. EwsMailHandler::setItemPayload).
    msg->setContent(KMime::CRLFtoLF(mime));
    msg->parse();
    // An empty body reads as "body not yet loaded" to Akonadi and would also loop.
    if (msg->body().isEmpty() && msg->contents().isEmpty()) {
        msg->setBody("\n");
    }
    item.setPayload(msg);
}

Item::List GraphFetchItemPayloadJob::items() const
{
    return mItems;
}

#include "moc_graphfetchitempayloadjob.cpp"
