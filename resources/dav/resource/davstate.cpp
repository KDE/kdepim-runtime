/*
    SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "davstate.h"

#include <KConfigGroup>
#include <ksharedconfig.h>

using namespace Qt::Literals;

constexpr auto DAV_PUSH_GROUP = QLatin1StringView("DavPush");

DavState::DavState(const KSharedConfigPtr &config)
    : mConfig(config)
{
}

QString DavState::getToken() const
{
    auto state = KConfigGroup(mConfig, DAV_PUSH_GROUP);
    if (state.hasKey(u"token"_s)) {
        return state.readEntry(u"token"_s, QString());
    }
    return QString();
}

void DavState::setToken(const QString &token)
{
    auto state = KConfigGroup(mConfig, DAV_PUSH_GROUP);
    state.writeEntry(u"token"_s, token);
    state.sync();
}

void DavState::clearToken()
{
    auto state = KConfigGroup(mConfig, DAV_PUSH_GROUP);
    state.deleteEntry(u"token"_s);
}

QByteArray DavState::getVapidPublicKey() const
{
    auto state = KConfigGroup(mConfig, DAV_PUSH_GROUP);
    if (state.hasKey(u"vapidPublicKey"_s)) {
        return state.readEntry(u"vapidPublicKey"_s, QByteArray());
    }
    return QByteArray();
}

void DavState::setVapidPublicKey(const QByteArray &vapidPublicKey)
{
    auto state = KConfigGroup(mConfig, DAV_PUSH_GROUP);
    state.writeEntry(u"vapidPublicKey"_s, vapidPublicKey);
    state.sync();
}

void DavState::clearVapidPublicKey()
{
    auto state = KConfigGroup(mConfig, DAV_PUSH_GROUP);
    state.deleteEntry(u"vapidPublicKey"_s);
}

#include "moc_davstate.cpp"
