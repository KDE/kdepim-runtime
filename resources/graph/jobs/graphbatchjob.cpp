/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "graphbatchjob.h"

#include <KLocalizedString>

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
    if (mCalls.isEmpty()) {
        emitResult();
        return;
    }
    next();
}

void GraphBatchJob::next()
{
    const Call &call = mCalls.at(mIndex);
    auto req = new GraphRequest(mClient, this);
    req->setMethod(call.method);
    req->setPath(call.path);
    if (call.method == GraphRequest::Method::Post || call.method == GraphRequest::Method::Patch) {
        req->setBody(call.body);
    }
    connect(req, &KJob::result, this, [this, req](KJob *job) {
        if (job->error() && !(mIgnoreNotFound && req->httpStatus() == 404)) {
            // Carry on with the remaining calls instead of abandoning them. One
            // Akonadi change notification fans out into one call per item, and a
            // failed replay is not retried: ResourceBase::cancelTask() marks it as
            // processed. Whatever is skipped here would therefore stay unapplied on
            // the server until the next full resync, without the user being told.
            if (++mFailed == 1) {
                mFirstError = job->error();
                mFirstErrorText = job->errorText();
            }
            mResponses.append(QJsonObject()); // keep responses aligned with the calls
        } else {
            mResponses.append(req->responseObject());
        }
        if (++mIndex < mCalls.size()) {
            next();
            return;
        }
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
    });
    req->start();
}

QList<QJsonObject> GraphBatchJob::responses() const
{
    return mResponses;
}

#include "moc_graphbatchjob.cpp"
