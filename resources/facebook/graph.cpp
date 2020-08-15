/*
 *    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>
 *
 *    SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "graph.h"
#include "resource_debug.h"

#include <QUrl>
#include <QUrlQuery>

QString Graph::appId()
{
    return QStringLiteral("1723356724632790");
}

QString Graph::scopes()
{
    return QStringLiteral("user_birthday,user_friends,user_events");
}

QUrl Graph::url(const QString &endpoint, const QString &accessToken, const QStringList &fields, const QMap<QString, QString> &queries)
{
    QUrl url(QStringLiteral("https://graph.facebook.com/v2.9/%1").arg(endpoint));
    QUrlQuery query(url);
    query.addQueryItem(QStringLiteral("access_token"), accessToken);
    if (!fields.isEmpty()) {
        query.addQueryItem(QStringLiteral("fields"), fields.join(QLatin1Char(',')));
    }
    for (auto it = queries.cbegin(), end = queries.cend(); it != end; ++it) {
        query.addQueryItem(it.key(), it.value());
    }
    url.setQuery(query);
    return url;
}

KJob *Graph::job(const QString &endpoint, const QString &accessToken, const QStringList &fields, const QMap<QString, QString> &queries)
{
    return job(url(endpoint, accessToken, fields, queries));
}

KJob *Graph::job(const QUrl &url)
{
    return KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
}

Graph::RSVP Graph::rsvpFromString(const QString &rsvp)
{
    if (rsvp == QLatin1String("attending")) {
        return Attending;
    } else if (rsvp == QLatin1String("maybe")) {
        return MaybeAttending;
    } else if (rsvp == QLatin1String("declined")) {
        return Declined;
    } else if (rsvp == QLatin1String("not_replied")) {
        return NotResponded;
    } else if (rsvp == QLatin1String("birthday")) {
        return Birthday;
    } else {
        qCDebug(FBRESOURCE_LOG) << "Unknown RSVP value" << rsvp;
        return NotResponded;
    }
}

QString Graph::rsvpToString(Graph::RSVP rsvp)
{
    switch (rsvp) {
    case Attending:
        return QStringLiteral("attending");
    case MaybeAttending:
        return QStringLiteral("maybe");
    case Declined:
        return QStringLiteral("declined");
    case NotResponded:
        return QStringLiteral("not_replied");
    case Birthday:
        return QStringLiteral("birthday");
    }

    Q_UNREACHABLE();
}
