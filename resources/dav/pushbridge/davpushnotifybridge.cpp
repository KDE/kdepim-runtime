/*
    SPDX-FileCopyrightText: 2026 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "davpushnotifybridge.h"
#include "davpushnotifybridge_debug.h"
#include "managementadaptor.h"
#include <QString>
#include <qloggingcategory.h>

using namespace Qt::Literals;

DavPushNotifyBridge::DavPushNotifyBridge(QObject *parent) // TODO add resource bus address
    : QObject(parent)
{
    new ManagementAdaptor(this);
    qCWarning(DAVPUSHNOTIFYBRIDGE_LOG) << QDBusConnection::sessionBus().registerObject(QLatin1StringView("/Management"), this);
}

void DavPushNotifyBridge::registerResource(const QString &resourceServiceName, const QString &vapid)
{
    qCDebug(DAVPUSHNOTIFYBRIDGE_LOG) << "Registering" << resourceServiceName << vapid;
    if (m_resources.contains(resourceServiceName)) {
        auto resourceHandler = m_resources[resourceServiceName].get();
        if (resourceHandler->vapid() == vapid) {
            return; // Already registered
        }
        resourceHandler->setVapid(vapid);
    }

    m_resources.try_emplace(resourceServiceName, std::make_unique<ResourceHandler>(resourceServiceName, vapid));
}
