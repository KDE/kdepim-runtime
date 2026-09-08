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

private:
    std::unordered_map<QString, std::unique_ptr<ResourceHandler>> m_resources;
};
