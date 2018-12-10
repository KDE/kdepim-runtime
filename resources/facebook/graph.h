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

#ifndef GRAPH_H_
#define GRAPH_H_

#include <kio/storedtransferjob.h>

namespace Graph {
QString appId();
QString scopes();

QUrl url(const QString &endpoint, const QString &accessToken, const QStringList &fields = {}, const QMap<QString, QString> &queries = {});

KJob *job(const QString &endpoint, const QString &accessToken, const QStringList &fields = {}, const QMap<QString, QString> &queries = {});
KJob *job(const QUrl &url);

enum RSVP {
    Attending,
    MaybeAttending,
    Declined,
    NotResponded,
    Birthday
};

RSVP rsvpFromString(const QString &rsvp);
QString rsvpToString(RSVP rsvp);
}

#endif // GRAPH_H_
