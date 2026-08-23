/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    One-time migration of stored item remoteIds to Graph immutable ids
    (POST /me/translateExchangeIds), run before the first sync that requests
    ids with Prefer: IdType="ImmutableId". Without this, a delta emitting the
    new id form would not match any local item and duplicate the mailbox.
*/

#pragma once

#include <Akonadi/Collection>
#include <Akonadi/Item>
#include <KJob>

class GraphClient;

class GraphMigrateIdsJob : public KJob
{
    Q_OBJECT
public:
    GraphMigrateIdsJob(GraphClient &client, const QString &resourceId, QObject *parent = nullptr);

    void start() override;

private:
    void fetchNextCollection();
    void translateNextChunk();
    void applyTranslations(const Akonadi::Item::List &chunk, const QHash<QString, QString> &targetIds);

    GraphClient &mClient;
    const QString mResourceId;
    Akonadi::Collection::List mCollections; // still to process
    QList<Akonadi::Item::List> mChunks; // translate batches for the current collection
    int mMigrated = 0;
    int mSkipped = 0;
};
