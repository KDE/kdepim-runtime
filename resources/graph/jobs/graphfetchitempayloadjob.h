/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    GET /me/messages/{id}/$value  ->  raw RFC822 MIME  ->  KMime::Message payload.
    This is the KMail-critical mapping and it is trivial because Graph hands us MIME
    directly (like EWS item:MimeContent). Mirrors EwsFetchItemPayloadJob.

    Payloads are fetched in batches of up to 20 via POST /$batch (Graph's JSON batch
    endpoint), so retrieving several selected messages costs one HTTP round trip instead
    of one per message. In a batch, /$value bodies come back base64-encoded and the
    sub-responses may be out of order, so we match them by request id.
*/

#pragma once

#include <Akonadi/Item>
#include <KJob>

class GraphClient;

class GraphFetchItemPayloadJob : public KJob
{
    Q_OBJECT
public:
    GraphFetchItemPayloadJob(GraphClient &client, const Akonadi::Item::List &items, QObject *parent = nullptr);

    void start() override;
    [[nodiscard]] Akonadi::Item::List items() const;

private:
    void fetchBatch();
    void applyMime(Akonadi::Item &item, const QByteArray &mime);

    GraphClient &mClient;
    Akonadi::Item::List mItems;
    int mIndex = 0; // next item not yet requested
};
