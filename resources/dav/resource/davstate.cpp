/*
    SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "davstate.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <QUrl>

using namespace Qt::Literals;

constexpr auto PUSH_GROUP = "DavPush"_L1;
constexpr auto PUSH_PROP_VAPID_PUBLIC_KEY = "vapidPublicKey"_L1;
constexpr auto PUSH_PROP_ENDPOINT = "endpoint"_L1;
constexpr auto PUSH_PROP_AUTH_SECRET = "authSecret"_L1;
constexpr auto PUSH_PROP_ENCRYPTION_PUBLIC_KEY = "encryptionPublicKey"_L1;

DavState::DavState(const KSharedConfigPtr &config)
    : mConfig(config)
{
}

KConfigGroup DavState::pushConfigGroup() const
{
    return KConfigGroup(mConfig, PUSH_GROUP);
}

void DavState::clearPush()
{
    clearPushVapidPublicKey();
    clearPushEndpoint();
    clearPushAuthSecret();
    clearPushEncryptionPublicKey();
}

QByteArray DavState::getPushVapidPublicKey() const
{
    return pushConfigGroup().readEntry(PUSH_PROP_VAPID_PUBLIC_KEY, QByteArray());
}

void DavState::setPushVapidPublicKey(const QByteArray &vapidPublicKey)
{
    auto state = pushConfigGroup();
    state.writeEntry(PUSH_PROP_VAPID_PUBLIC_KEY, vapidPublicKey);
    state.sync();
}

void DavState::clearPushVapidPublicKey()
{
    auto state = pushConfigGroup();
    state.deleteEntry(PUSH_PROP_VAPID_PUBLIC_KEY);
    state.sync();
}

QUrl DavState::getPushEndpoint() const
{
    return pushConfigGroup().readEntry(PUSH_PROP_ENDPOINT, QUrl());
}

void DavState::setPushEndpoint(const QUrl &pushEndpoint)
{
    auto state = pushConfigGroup();
    state.writeEntry(PUSH_PROP_ENDPOINT, pushEndpoint);
    state.sync();
}

void DavState::clearPushEndpoint()
{
    auto state = pushConfigGroup();
    state.deleteEntry(PUSH_PROP_ENDPOINT);
    state.sync();
}

QByteArray DavState::getPushAuthSecret() const
{
    return pushConfigGroup().readEntry(PUSH_PROP_AUTH_SECRET, QByteArray());
}

void DavState::setPushAuthSecret(const QByteArray &pushAuthSecret)
{
    auto state = pushConfigGroup();
    state.writeEntry(PUSH_PROP_AUTH_SECRET, pushAuthSecret);
    state.sync();
}

void DavState::clearPushAuthSecret()
{
    auto state = pushConfigGroup();
    state.deleteEntry(PUSH_PROP_AUTH_SECRET);
    state.sync();
}

QByteArray DavState::getPushEncryptionPublicKey() const
{
    return pushConfigGroup().readEntry(PUSH_PROP_ENCRYPTION_PUBLIC_KEY, QByteArray());
}

void DavState::setPushEncryptionPublicKey(const QByteArray &pushEncryptionPublicKey)
{
    auto state = pushConfigGroup();
    state.writeEntry(PUSH_PROP_ENCRYPTION_PUBLIC_KEY, pushEncryptionPublicKey);
    state.sync();
}

void DavState::clearPushEncryptionPublicKey()
{
    auto state = pushConfigGroup();
    state.deleteEntry(PUSH_PROP_ENCRYPTION_PUBLIC_KEY);
    state.sync();
}

#include "moc_davstate.cpp"
