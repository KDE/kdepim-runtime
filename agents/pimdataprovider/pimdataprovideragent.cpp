/*
    SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pimdataprovideragent.h"
#include "managementadaptor.h"
#include "pimdataproviderserver.h"

#include <QDBusConnection>

namespace Akonadi
{
PimDataProviderAgent::PimDataProviderAgent(const QString &id)
    : AgentWidgetBase(id)
{
    registerPimDataProviderDBusTypes();

    new ManagementAdaptor(this);

    QDBusConnection::sessionBus().registerObject(QStringLiteral("/org/pim/dataprovider/Calendar/Management"), this, QDBusConnection::ExportAdaptors);

    const QString service = QStringLiteral("org.pim.dataprovider.Calendar1.") + id;
    QDBusConnection::sessionBus().registerService(service);
}

PimDataProviderAgent::~PimDataProviderAgent() = default;

QList<QVariantMap> PimDataProviderAgent::getServers() const
{
    return {};
}

PimDataProviderItemList PimDataProviderAgent::getCalendars() const
{
    return {};
}

} // namespace Akonadi

AKONADI_AGENT_MAIN(Akonadi::PimDataProviderAgent)

#include "moc_pimdataprovideragent.cpp"
