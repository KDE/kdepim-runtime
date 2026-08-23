/*
    SPDX-FileCopyrightText: 2026 Malte Zilinski <malte@zilinski.eu>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "graphmailhandler.h"

#include <Akonadi/MessageFlags>
#include <KMime/Message>

namespace GraphMailHandler
{
QString mimeType()
{
    return KMime::Message::mimeType();
}

QJsonObject flagPatchBody(const Akonadi::Item &item)
{
    QJsonObject body;
    body.insert(QStringLiteral("isRead"), item.hasFlag(Akonadi::MessageFlags::Seen));

    QJsonObject flag;
    flag.insert(QStringLiteral("flagStatus"), item.hasFlag(Akonadi::MessageFlags::Flagged) ? QStringLiteral("flagged") : QStringLiteral("notFlagged"));
    body.insert(QStringLiteral("flag"), flag);
    return body;
}

QPair<QByteArray, QByteArray> createFromMime(const Akonadi::Item &item)
{
    if (!item.hasPayload<std::shared_ptr<KMime::Message>>()) {
        return {};
    }
    const auto msg = item.payload<std::shared_ptr<KMime::Message>>();
    // Graph accepts a full MIME message on POST /me/messages when the body is the
    // *base64-encoded* MIME with Content-Type: text/plain. It creates a draft;
    // caller then POSTs .../send.
    return {msg->encodedContent(KMime::NewlineType::CRLF).toBase64(), QByteArray("text/plain")};
}
}
