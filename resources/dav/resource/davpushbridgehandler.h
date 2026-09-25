/*
 *  SPDX-FileCopyrightText: 2026 Dominique Michel <dominique.michel@enioka.com>
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QObject>
#include <QString>
#include <QUrl>

class DavPushBridgeHandler : public QObject
{
    Q_OBJECT

public:
    explicit DavPushBridgeHandler(const QString &resourceName);

    void connectHandler();

    void setResourceName(const QString &resourceName);
    [[nodiscard]] QString resourceName() const;

public:
    QDBusPendingCall registerResource(const QString &topic, const QString &resourceName = QString());

private Q_SLOTS:
    void onEndpointChange(const QString &resourceName, const QString &endpoint, const QByteArray &authSecret, const QByteArray &encryptionPublicKey);
    void onContentUpdate(const QString &resourceName, const QString &topic, const QString &syncToken);
    void onPropertyUpdate(const QString &resourceName, const QString &topic);
    void onVapidKeyChange(const QString &resourceName);

Q_SIGNALS:
    void endpointChanged(const QUrl &endpoint, const QByteArray &authSecret, const QByteArray &encryptionPublicKey);
    void contentUpdated(const QString &topic, const QString &syncToken);
    void propertyUpdated(const QString &topic);
    void vapidKeyChanged();

private:
    QString mResourceName;
    QDBusConnection mConnection;
};
