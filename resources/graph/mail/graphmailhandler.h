/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later

    Maps between Akonadi mail items (KMime) and Graph `message` resources.
    In Phase 1 the read path needs almost nothing here because Graph gives us MIME via
    /$value (see GraphFetchItemPayloadJob). This handler carries the *write* mappings
    (create draft, flag changes) and exists so calendar/contact handlers can follow the
    same shape later (cf. ews mail/calendar/contact/task handlers).
*/

#pragma once

#include <Akonadi/Item>
#include <QJsonObject>

namespace GraphMailHandler
{
[[nodiscard]] QString mimeType();

/// Seen/Flagged -> PATCH body for /me/messages/{id}.
[[nodiscard]] QJsonObject flagPatchBody(const Akonadi::Item &item);

/// KMime payload -> body for creating a message from MIME:
///   POST /me/messages  with Content-Type: text/plain and base64(MIME)
/// Returns (rawBody, contentType).
[[nodiscard]] QPair<QByteArray, QByteArray> createFromMime(const Akonadi::Item &item);
}
