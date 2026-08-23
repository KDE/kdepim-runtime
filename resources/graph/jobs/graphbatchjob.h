/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    Runs a list of Graph calls sequentially (deliberately not in parallel — Graph
    throttles aggressively) and collects the per-call JSON responses. Used by the
    change-replay handlers (flags, move, delete, create) where one Akonadi change
    notification fans out into one call per item.
*/

#pragma once

#include "graphclient/graphrequest.h"

#include <KJob>
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

    /// One entry per call, in order; empty object for responses without a body (204).
    [[nodiscard]] QList<QJsonObject> responses() const;

private:
    void next();

    GraphClient &mClient;
    QList<Call> mCalls;
    QList<QJsonObject> mResponses;
    int mIndex = 0;
    bool mIgnoreNotFound = false;
};
