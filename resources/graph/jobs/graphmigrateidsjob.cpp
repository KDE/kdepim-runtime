/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "graphmigrateidsjob.h"

#include "calendar/grapheventhandler.h"
#include "contact/graphcontacthandler.h"
#include "graph_debug.h"
#include "graphclient/graphrequest.h"

#include <Akonadi/CollectionFetchJob>
#include <Akonadi/CollectionFetchScope>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/TransactionSequence>

#include <KLocalizedString>
#include <KMime/Message>
#include <QJsonArray>
#include <QJsonObject>

using namespace Akonadi;

// translateExchangeIds accepts at most 1000 ids per call.
static constexpr int kTranslateChunkSize = 1000;

GraphMigrateIdsJob::GraphMigrateIdsJob(GraphClient &client, const QString &resourceId, QObject *parent)
    : KJob(parent)
    , mClient(client)
    , mResourceId(resourceId)
{
}

void GraphMigrateIdsJob::start()
{
    auto fetch = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive, this);
    fetch->fetchScope().setResource(mResourceId);
    connect(fetch, &CollectionFetchJob::result, this, [this, fetch](KJob *job) {
        if (job->error()) {
            setError(job->error());
            setErrorText(job->errorText());
            emitResult();
            return;
        }
        for (const Collection &col : fetch->collections()) {
            const QStringList mimes = col.contentMimeTypes();
            // Mail, events and contacts carry Exchange item ids that translate; To Do
            // tasks live in a separate id space and keep their ids (no IdType there).
            if (mimes.contains(KMime::Message::mimeType()) || mimes.contains(GraphEventHandler::mimeType())
                || mimes.contains(GraphContactHandler::mimeType())) {
                mCollections.append(col);
            }
        }
        fetchNextCollection();
    });
}

void GraphMigrateIdsJob::fetchNextCollection()
{
    if (mCollections.isEmpty()) {
        qCDebug(GRAPH_LOG) << "id migration done, migrated:" << mMigrated << "skipped:" << mSkipped;
        emitResult();
        return;
    }
    const Collection col = mCollections.takeFirst();

    auto fetch = new ItemFetchJob(col, this);
    fetch->fetchScope().setFetchModificationTime(false);
    connect(fetch, &ItemFetchJob::result, this, [this, col, fetch](KJob *job) {
        if (job->error()) {
            setError(job->error());
            setErrorText(job->errorText());
            emitResult();
            return;
        }
        Item::List pending;
        for (const Item &item : fetch->items()) {
            if (!item.remoteId().isEmpty()) {
                pending.append(item);
            }
        }
        qCDebug(GRAPH_LOG) << "id migration:" << col.name() << pending.size() << "items";
        for (int i = 0; i < pending.size(); i += kTranslateChunkSize) {
            mChunks.append(pending.mid(i, kTranslateChunkSize));
        }
        translateNextChunk();
    });
}

void GraphMigrateIdsJob::translateNextChunk()
{
    if (mChunks.isEmpty()) {
        fetchNextCollection();
        return;
    }
    const Item::List chunk = mChunks.takeFirst();

    QJsonArray inputIds;
    for (const Item &item : chunk) {
        inputIds.append(item.remoteId());
    }
    auto req = new GraphRequest(mClient, this);
    req->setMethod(GraphRequest::Method::Post);
    req->setPath(QStringLiteral("/me/translateExchangeIds"));
    req->setBody({{QStringLiteral("inputIds"), inputIds},
                  {QStringLiteral("sourceIdType"), QStringLiteral("restId")},
                  {QStringLiteral("targetIdType"), QStringLiteral("restImmutableEntryId")}});
    connect(req, &KJob::result, this, [this, req, chunk](KJob *job) {
        if (job->error()) {
            // A 400 means at least one id is not a mutable REST id (typically already
            // migrated by an interrupted earlier run — the whole call fails, there is
            // no per-id error). Bisect until the offenders are isolated and skipped.
            if (req->httpStatus() == 400) {
                if (chunk.size() == 1) {
                    ++mSkipped;
                    translateNextChunk();
                    return;
                }
                const int half = chunk.size() / 2;
                mChunks.prepend(chunk.mid(half));
                mChunks.prepend(chunk.mid(0, half));
                translateNextChunk();
                return;
            }
            setError(job->error());
            setErrorText(job->errorText());
            emitResult();
            return;
        }
        QHash<QString, QString> targetIds;
        const auto values = req->aggregatedValue();
        for (const auto &v : values) {
            const QJsonObject entry = v.toObject();
            targetIds.insert(entry.value(QLatin1String("sourceId")).toString(), entry.value(QLatin1String("targetId")).toString());
        }
        applyTranslations(chunk, targetIds);
    });
    req->start();
}

void GraphMigrateIdsJob::applyTranslations(const Item::List &chunk, const QHash<QString, QString> &targetIds)
{
    Item::List renamed;
    for (Item item : chunk) {
        const QString target = targetIds.value(item.remoteId());
        if (target.isEmpty() || target == item.remoteId()) {
            continue;
        }
        item.setRemoteId(target);
        renamed.append(item);
    }
    if (renamed.isEmpty()) {
        translateNextChunk();
        return;
    }
    auto trx = new TransactionSequence(this);
    for (const Item &item : std::as_const(renamed)) {
        // Only the remoteId is dirty, so only it is transmitted; remoteIds are owned
        // by the resource, nothing else writes them — skip the revision conflict check.
        auto mod = new ItemModifyJob(item, trx);
        mod->setIgnorePayload(true);
        mod->disableRevisionCheck();
    }
    mMigrated += renamed.size();
    connect(trx, &TransactionSequence::result, this, [this](KJob *job) {
        if (job->error()) {
            setError(job->error());
            setErrorText(job->errorText());
            emitResult();
            return;
        }
        translateNextChunk();
    });
    trx->commit();
}

#include "moc_graphmigrateidsjob.cpp"
