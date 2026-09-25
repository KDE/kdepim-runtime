/*
    SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KSharedConfig>
#include <QObject>

class QByteArray;
class QString;

class DavState : public QObject
{
    Q_OBJECT

public:
    explicit DavState(const KSharedConfigPtr &config);

    KConfigGroup pushConfigGroup() const;
    void clearPush();

    // Format is base64url
    [[nodiscard]] QByteArray getPushVapidPublicKey() const;
    // Format is base64url
    void setPushVapidPublicKey(const QByteArray &vapidPublicKey);
    void clearPushVapidPublicKey();

    [[nodiscard]] QUrl getPushEndpoint() const;
    void setPushEndpoint(const QUrl &pushEndpoint);
    void clearPushEndpoint();

    // Format is base64url
    [[nodiscard]] QByteArray getPushAuthSecret() const;
    // Format is base64url
    void setPushAuthSecret(const QByteArray &pushAuthSecret);
    void clearPushAuthSecret();

    // Format is base64url
    [[nodiscard]] QByteArray getPushEncryptionPublicKey() const;
    // Format is base64url
    void setPushEncryptionPublicKey(const QByteArray &pushEncryptionPublicKey);
    void clearPushEncryptionPublicKey();

private:
    KSharedConfigPtr mConfig;
};
