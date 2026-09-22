/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "graphbatchjob.h"

#include "graph_debug.h"

#include <KLocalizedString>
#include <QJsonArray>
#include <QSet>
#include <QTimer>

// Graph's /$batch accepts at most 20 sub-requests per call.
static constexpr int kBatchSize = 20;
// A sub-request answered with 429/503 is re-issued this often before it counts as
// failed (the top-level request has its own retry loop in GraphRequest). Counted per
// call: Graph runs only a few requests per mailbox at a time and throttles the rest
// of a batch, so a large change needs many rounds while every call still progresses.
static constexpr int kMaxThrottleRetries = 5;

static QString methodName(GraphRequest::Method method)
{
    switch (method) {
    case GraphRequest::Method::Get:
        return QStringLiteral("GET");
    case GraphRequest::Method::Post:
        return QStringLiteral("POST");
    case GraphRequest::Method::Put:
        return QStringLiteral("PUT");
    case GraphRequest::Method::Patch:
        return QStringLiteral("PATCH");
    case GraphRequest::Method::Delete:
        return QStringLiteral("DELETE");
    }
    return {};
}

GraphBatchJob::GraphBatchJob(GraphClient &client, const QList<Call> &calls, QObject *parent)
    : KJob(parent)
    , mClient(client)
    , mCalls(calls)
{
}

void GraphBatchJob::setIgnoreNotFound(bool ignore)
{
    mIgnoreNotFound = ignore;
}

void GraphBatchJob::start()
{
    mResponses = QList<QJsonObject>(mCalls.size());
    if (mCalls.isEmpty()) {
        // Stay asynchronous like every other exit of this job, so a caller that
        // connects after start() cannot miss the result (same as GraphRequest).
        QTimer::singleShot(0, this, [this] {
            emitResult();
        });
        return;
    }
    mPending.reserve(mCalls.size());
    for (int i = 0; i < mCalls.size(); ++i) {
        mPending.append(i);
    }
    issueNextBatch();
}

void GraphBatchJob::issueNextBatch()
{
    const QList<int> indexes = mPending.mid(0, kBatchSize);
    mPending.remove(0, indexes.size());

    // The sub-request "id" is the index into mCalls, so each response routes back to
    // its call. Headers are not inherited from the outer request, so the immutable-id
    // preference GraphRequest sets on every single call is repeated here.
    QJsonArray requests;
    for (const int i : indexes) {
        const Call &call = mCalls.at(i);
        QJsonObject sub;
        sub.insert(QStringLiteral("id"), QString::number(i));
        sub.insert(QStringLiteral("method"), methodName(call.method));
        sub.insert(QStringLiteral("url"), call.path);
        QJsonObject headers;
        if (GraphRequest::usesImmutableIds(call.path)) {
            headers.insert(QStringLiteral("Prefer"), QStringLiteral("IdType=\"ImmutableId\""));
        }
        if (call.method == GraphRequest::Method::Post || call.method == GraphRequest::Method::Patch) {
            headers.insert(QStringLiteral("Content-Type"), QStringLiteral("application/json"));
            sub.insert(QStringLiteral("body"), call.body);
        }
        sub.insert(QStringLiteral("headers"), headers);
        requests.append(sub);
    }
    QJsonObject body;
    body.insert(QStringLiteral("requests"), requests);

    auto req = new GraphRequest(mClient, this);
    req->setMethod(GraphRequest::Method::Post);
    req->setPath(QStringLiteral("/$batch"));
    req->setBody(body);
    connect(req, &KJob::result, this, [this, req, indexes](KJob *job) {
        if (job->error()) {
            // The server itself did not answer (no token yet, network gone, or the
            // batch was rejected as a whole): nothing further will get through either,
            // so account for every remaining call and stop.
            for (int i = 0; i < indexes.size() + mPending.size(); ++i) {
                failCall(job->error(), job->errorText());
            }
            mPending.clear();
            finish();
            return;
        }

        QList<int> throttled;
        int retryAfter = 0;
        QSet<int> answered;
        const QJsonArray responses = req->responseObject().value(QLatin1String("responses")).toArray();
        for (const auto &r : responses) {
            const QJsonObject resp = r.toObject();
            const int index = resp.value(QLatin1String("id")).toString().toInt();
            if (!indexes.contains(index) || answered.contains(index)) {
                continue;
            }
            answered.insert(index);
            const int status = resp.value(QLatin1String("status")).toInt();
            if ((status == 429 || status == 503) && mThrottleRetries.value(index) < kMaxThrottleRetries) {
                ++mThrottleRetries[index];
                throttled.append(index);
                retryAfter = qMax(retryAfter, resp.value(QLatin1String("headers")).toObject().value(QLatin1String("Retry-After")).toString().toInt());
                continue;
            }
            finishCall(index, status, resp.value(QLatin1String("body")).toObject());
        }
        for (const int i : indexes) {
            if (!answered.contains(i)) {
                failCall(KJob::UserDefinedError, i18n("The server did not answer this request"));
            }
        }

        if (!throttled.isEmpty()) {
            // Re-issue the throttled ones first, after the delay the server asked for.
            mPending = throttled + mPending;
            const int seconds = retryAfter > 0 ? retryAfter : (1 << mThrottleRetries.value(throttled.constFirst()));
            QTimer::singleShot(seconds * 1000, this, &GraphBatchJob::issueNextBatch);
            return;
        }
        if (mPending.isEmpty()) {
            finish();
        } else {
            issueNextBatch();
        }
    });
    req->start();
}

void GraphBatchJob::finishCall(int index, int status, const QJsonObject &body)
{
    if (status >= 200 && status < 300) {
        mResponses[index] = body;
        return;
    }
    if (mIgnoreNotFound && status == 404) {
        // Expected when the item is already gone on the server; logged so that a
        // systematic cause (such as a wrong id) does not go unnoticed.
        qCInfo(GRAPH_LOG) << "batch: treating 404 as done for" << mCalls.at(index).path;
        return; // success without a response body
    }
    const QJsonObject err = body.value(QLatin1String("error")).toObject();
    failCall(KJob::UserDefinedError, err.isEmpty() ? i18n("HTTP %1", status) : GraphRequest::formatError(err, status));
}

void GraphBatchJob::failCall(int error, const QString &errorText)
{
    // Carry on with the remaining calls instead of abandoning them. One Akonadi
    // change notification fans out into one call per item, and a failed replay is
    // not retried: ResourceBase::cancelTask() marks it as processed. Whatever is
    // skipped here would therefore stay unapplied on the server until the next
    // full resync, without the user being told.
    if (++mFailed == 1) {
        mFirstError = error;
        mFirstErrorText = errorText;
    }
}

void GraphBatchJob::finish()
{
    if (mFailed > 0) {
        setError(mFirstError);
        setErrorText(mCalls.size() == 1 ? mFirstErrorText
                                        : i18nc("%1 and %2 are counts, %3 the server error message",
                                                "%1 of %2 requests failed, first error: %3",
                                                mFailed,
                                                mCalls.size(),
                                                mFirstErrorText));
    }
    emitResult();
}

QList<QJsonObject> GraphBatchJob::responses() const
{
    return mResponses;
}

#include "moc_graphbatchjob.cpp"
