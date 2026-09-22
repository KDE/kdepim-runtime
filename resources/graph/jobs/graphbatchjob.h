/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    Runs a list of Graph calls through /$batch (20 sub-requests per round trip, the
    batches themselves one after the other — Graph throttles aggressively) and
    collects the per-call JSON responses. Used by the change-replay handlers (flags,
    move, delete, create) where one Akonadi change notification fans out into one
    call per item.

    Every call is issued even when others failed, as long as the server itself
    answers; the job reports the first error once all of them are done. Sub-requests
    that Graph throttles are re-issued after the announced delay.
*/

#pragma once

#include "graphclient/graphrequest.h"

#include <KJob>
#include <QHash>
#include <QJsonObject>
#include <QList>

class GraphClient;

class GraphBatchJob : public KJob
{
    Q_OBJECT
public:
    struct Call {
        GraphRequest::Method method;
        QString path;
        QJsonObject body; // ignored for Get/Delete
    };

    GraphBatchJob(GraphClient &client, const QList<Call> &calls, QObject *parent = nullptr);

    /// Treat HTTP 404 as success (delete/move of an item already gone on the server).
    void setIgnoreNotFound(bool ignore);

    void start() override;

    /// One entry per call, in order; empty object for responses without a body (204)
    /// and for calls that failed.
    [[nodiscard]] QList<QJsonObject> responses() const;

private:
    void issueNextBatch();
    void finishCall(int index, int status, const QJsonObject &body);
    void failCall(int error, const QString &errorText);
    void finish();

    GraphClient &mClient;
    const QList<Call> mCalls;
    QList<QJsonObject> mResponses;
    QList<int> mPending; // indexes not yet answered, in issue order
    QString mFirstErrorText;
    int mFailed = 0;
    int mFirstError = 0;
    QHash<int, int> mThrottleRetries; // per call index
    bool mIgnoreNotFound = false;
};
