/*
    SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pimdataprovideragent.h"
#include "managementadaptor.h"
#include "pimdataproviderserver.h"

#include <Akonadi/AgentManager>

#include <KConfigGroup>
#include <KSharedConfig>

#include <QDBusConnection>

namespace Akonadi
{
static QString extractDavUrl(const AgentInstance &instance)
{
    const KSharedConfigPtr config = KSharedConfig::openConfig(instance.identifier() + QStringLiteral("rc"));
    const KConfigGroup generalGroup(config, QStringLiteral("General"));
    const QStringList remoteUrls = generalGroup.readEntry("remoteUrls", QStringList());
    auto davUrl = QString();

    for (const QString &serialized : remoteUrls) {
        const QStringList parts = serialized.split(u'|');
        if (parts.size() >= 3 && parts.at(1).compare(QLatin1StringView("CalDav"), Qt::CaseInsensitive) == 0 && !parts.at(2).isEmpty()) {
            Q_ASSERT(davUrl.isEmpty());
            davUrl = parts.at(2);
        }
    }
    return davUrl.isEmpty() ? instance.identifier() : davUrl;
}

PimDataProviderAgent::PimDataProviderAgent(const QString &id)
    : AgentWidgetBase(id)
{
    const QString service = QStringLiteral("org.pim.dataprovider.Calendar1.") + id;
    QDBusConnection::sessionBus().registerService(service);

    registerPimDataProviderDBusTypes();
    new ManagementAdaptor(this);

    // Servers management : we use the instance manager
    connect(AgentManager::self(), &AgentManager::instanceAdded, this, &PimDataProviderAgent::slotServerAdded);
    connect(AgentManager::self(), &AgentManager::instanceRemoved, this, &PimDataProviderAgent::slotServerRemoved);

    // Get the list of already registered Dav servers
    const AgentInstance::List instances = AgentManager::self()->instances();
    for (const AgentInstance &instance : instances) {
        if (instance.type().identifier() == QLatin1StringView("akonadi_davgroupware_resource")) {
            const QString serverUrl = extractDavUrl(instance);
            mServers.insert(serverUrl, std::make_shared<PimDataProviderServer>(serverUrl));
        }
    }
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/org/pim/dataprovider/Calendar/Management"), this, QDBusConnection::ExportAdaptors);
}

PimDataProviderAgent::~PimDataProviderAgent()
{
    QDBusConnection::sessionBus().unregisterObject(QStringLiteral("/org/pim/dataprovider/Calendar/Management/"));
}

QList<QVariantMap> PimDataProviderAgent::getServers() const
{
    QList<QVariantMap> result;
    result.reserve(mServers.size());
    for (const auto &server : mServers) {
        const QString path = QStringLiteral("/org/pim/dataprovider/Calendar/Servers/") + encodeForDBusPath(server->id());
        result.emplaceBack(QVariantMap{
            {QStringLiteral("path"), path},
            {QStringLiteral("id"), server->id()},
        });
    }
    return result;
}

PimDataProviderItemList PimDataProviderAgent::getCalendars() const
{
    PimDataProviderItemList result;
    for (const auto &server : mServers) {
        const auto calendarMaps = server->getCalendars();
        for (const auto &map : calendarMaps) {
            result.emplace_back(PimDataProviderItem{
                server->id(),
                map,
            });
        }
    }
    return result;
}

void PimDataProviderAgent::slotServerAdded(const AgentInstance &instance)
{
    if (instance.type().identifier() != QLatin1StringView("akonadi_davgroupware_resource")) {
        return;
    }
    const QString serverId = extractDavUrl(instance);
    if (mServers.contains(serverId)) {
        return;
    }
    mServers.insert(serverId, std::make_shared<PimDataProviderServer>(serverId));

    const QString path = QStringLiteral("/org/pim/dataprovider/Calendar/Servers/") + encodeForDBusPath(serverId);
    QVariantMap serverMap;
    serverMap.insert(QStringLiteral("path"), path);
    serverMap.insert(QStringLiteral("id"), serverId);

    PimDataProviderItem item;
    item.id = serverId;
    item.data = serverMap;
    Q_EMIT serverCreated(item);
}

void PimDataProviderAgent::slotServerRemoved(const AgentInstance &instance)
{
    const QString serverId = extractDavUrl(instance);
    if (auto server = mServers.take(serverId)) {
        Q_EMIT serverDeleted(serverId);
    }
}

} // namespace Akonadi

AKONADI_AGENT_MAIN(Akonadi::PimDataProviderAgent)

#include "moc_pimdataprovideragent.cpp"
