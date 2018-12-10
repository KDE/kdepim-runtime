/*
 *    Copyright (C) 2017 Daniel Vrátil <dvratil@kde.org>
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
