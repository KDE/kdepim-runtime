/*
    SPDX-FileCopyrightText: 2026 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "resourcehandler.h"

#include <KUnifiedPush/Connector>
#include <QDBusContext>

class DavPushNotifyBridge : public QObject, public QDBusContext
{
    Q_OBJECT
public:
    explicit DavPushNotifyBridge(QObject *parent = nullptr);
    void registerResource(const QString &resourceServiceName, const QString &vapid);
Q_SIGNALS:
    void contentUpdate(const QString &resourceName, const QString &topic, const QString &syncToken);
    void endpointChanged(const QString &resourceName,
                         const QString &endpoint,
                         const QByteArray &contentEncryptionAuthSecret,
                         const QByteArray &contentEncryptionPublicKey);

private:
    std::unordered_map<QString, std::unique_ptr<ResourceHandler>> m_resources;
};
