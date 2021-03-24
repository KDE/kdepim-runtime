/*
 *    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>
 *
 *    SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <kio/storedtransferjob.h>

namespace Graph
{
QString appId();
QString scopes();

QUrl url(const QString &endpoint, const QString &accessToken, const QStringList &fields = {}, const QMap<QString, QString> &queries = {});

KJob *job(const QString &endpoint, const QString &accessToken, const QStringList &fields = {}, const QMap<QString, QString> &queries = {});
KJob *job(const QUrl &url);

enum RSVP { Attending, MaybeAttending, Declined, NotResponded, Birthday };

RSVP rsvpFromString(const QString &rsvp);
QString rsvpToString(RSVP rsvp);
}

