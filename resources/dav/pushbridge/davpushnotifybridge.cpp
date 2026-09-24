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
    ResourceHandler *resourceHandler;
    auto it = m_resources.find(resourceServiceName);
    if (it == m_resources.end()) {
        resourceHandler = it->second.get();
        resourceHandler->setVapid(vapid);
    } else {
        it = m_resources.emplace(resourceServiceName, std::make_unique<ResourceHandler>(resourceServiceName, vapid)).first;
        resourceHandler = it->second.get();
    }

    connect(resourceHandler, &ResourceHandler::endpointChanged, this, &DavPushNotifyBridge::endpointChanged);
    connect(resourceHandler, &ResourceHandler::contentUpdate, this, &DavPushNotifyBridge::contentUpdate);
    connect(resourceHandler, &ResourceHandler::propertyUpdate, this, &DavPushNotifyBridge::propertyUpdate);
    connect(resourceHandler, &ResourceHandler::vapidKeyUpdated, this, &DavPushNotifyBridge::vapidKeyUpdated);
}

void DavPushNotifyBridge::unregisterResource(const QString &resourceServiceName)
{
    // TODO delete on kunifiedpush
    qCDebug(DAVPUSHNOTIFYBRIDGE_LOG) << "Unregistering" << resourceServiceName;
    if (m_resources.contains(resourceServiceName)) {
        m_resources.erase(resourceServiceName);
    }
}
