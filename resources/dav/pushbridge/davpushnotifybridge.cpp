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
        it = m_resources.insert({resourceServiceName, std::make_unique<ResourceHandler>(resourceServiceName, vapid)}).first;
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

QString DavPushNotifyBridge::stateFile() const
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + "/davpushnotifybridge"_L1;
}

void DavPushNotifyBridge::storeState() const
{
    QSettings settings(stateFile(), QSettings::IniFormat);
    for (const auto &[resourceName, handler] : m_resources) {
        settings.beginGroup(resourceName);
        settings.setValue("vapid", handler->vapid());
        settings.endGroup();
    }
}

void DavPushNotifyBridge::loadState()
{
    QSettings settings(stateFile(), QSettings::IniFormat);
    for (const QString &resourceName : settings.childGroups()) {
        settings.beginGroup(resourceName);
        const QString vapid = settings.value("vapid").toString();
        settings.endGroup();
        m_resources.insert({resourceName, std::make_unique<ResourceHandler>(resourceName, vapid)});
        connectResourceToBridge(resourceName);
    }
}
