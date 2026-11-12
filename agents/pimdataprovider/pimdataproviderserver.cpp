/*
    SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pimdataproviderserver.h"
#include "serveradaptor.h"

#include <QDBusConnection>

namespace Akonadi
{
PimDataProviderServer::PimDataProviderServer(const QString &serverId, QObject *parent)
    : QObject(parent)
    , mServerId(serverId)
{
    new ServerAdaptor(this);

    const QString path = QStringLiteral("/org/pim/dataprovider/Calendar/Servers/") + serverId;
    QDBusConnection::sessionBus().registerObject(path, this, QDBusConnection::ExportAdaptors);
}

PimDataProviderServer::~PimDataProviderServer() = default;

QString PimDataProviderServer::id() const
{
    return mServerId;
}

QList<QVariantMap> PimDataProviderServer::getCalendars() const
{
    return {};
}

} // namespace Akonadi

#include "moc_pimdataproviderserver.cpp"
