/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "graphbatchjob.h"

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
            setError(job->error());
            setErrorText(job->errorText());
            emitResult();
            return;
        }
        mResponses.append(req->responseObject());
        if (++mIndex < mCalls.size()) {
            next();
        } else {
            emitResult();
        }
    });
    req->start();
}

QList<QJsonObject> GraphBatchJob::responses() const
{
    return mResponses;
}

#include "moc_graphbatchjob.cpp"
