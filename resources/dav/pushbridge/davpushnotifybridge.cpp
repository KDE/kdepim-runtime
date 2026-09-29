/*
    SPDX-FileCopyrightText: 2026 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "davpushnotifybridge.h"
#include "davpushnotifybridge_debug.h"
#include "managementadaptor.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <QString>
#include <qloggingcategory.h>

using namespace Qt::Literals;

DavPushNotifyBridge::DavPushNotifyBridge(QObject *parent) // TODO add resource bus address
    : QObject(parent)
{
    loadState();
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
        connectResourceToBridge(resourceServiceName);
    }
    storeState();
}

void DavPushNotifyBridge::connectResourceToBridge(const QString &resourceName)
{
    if (m_resources.contains(resourceName)) {
        auto resourceHandler = m_resources[resourceName].get();
        connect(resourceHandler, &ResourceHandler::endpointChanged, this, &DavPushNotifyBridge::endpointChanged);
        connect(resourceHandler, &ResourceHandler::contentUpdate, this, &DavPushNotifyBridge::contentUpdate);
        connect(resourceHandler, &ResourceHandler::propertyUpdate, this, &DavPushNotifyBridge::propertyUpdate);
        connect(resourceHandler, &ResourceHandler::vapidKeyUpdated, this, &DavPushNotifyBridge::vapidKeyUpdated);
    }
}

void DavPushNotifyBridge::unregisterResource(const QString &resourceServiceName)
{
    qCDebug(DAVPUSHNOTIFYBRIDGE_LOG) << "Unregistering" << resourceServiceName;
    if (m_resources.contains(resourceServiceName)) {
        auto resourceHandler = m_resources[resourceServiceName].get();
        resourceHandler->unregister();
        m_resources.erase(resourceServiceName);
        storeState();
    }
}

void DavPushNotifyBridge::storeState() const
{
    const KSharedConfigPtr config = KSharedConfig::openStateConfig(u"davpushnotifybridge"_s);

    for (const auto &[resourceName, handler] : m_resources) {
        KConfigGroup group = config->group(resourceName);
        group.writeEntry("vapid", handler->vapid());
    }

    config->sync();
}

void DavPushNotifyBridge::loadState()
{
    const KSharedConfigPtr config = KSharedConfig::openStateConfig(u"davpushnotifybridge"_s);

    for (const QString &resourceName : config->groupList()) {
        const KConfigGroup group = config->group(resourceName);
        const QString vapid = group.readEntry("vapid", QString());

        m_resources.emplace(resourceName, std::make_unique<ResourceHandler>(resourceName, vapid));
        connectResourceToBridge(resourceName);
    }
}
