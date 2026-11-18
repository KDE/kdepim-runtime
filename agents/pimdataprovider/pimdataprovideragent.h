/*
SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "pimdataprovider_types.h"
#include "pimdataproviderserver.h"

#include <Akonadi/AgentInstance>
#include <Akonadi/AgentWidgetBase>

#include <QMap>

class KJob;

namespace Akonadi
{
class PimDataProviderAgent : public AgentWidgetBase
{
    Q_OBJECT
public:
    explicit PimDataProviderAgent(const QString &id);
    ~PimDataProviderAgent() override;
    PimDataProviderAgent(const PimDataProviderAgent &) = delete;
    PimDataProviderAgent &operator=(const PimDataProviderAgent &) = delete;
    PimDataProviderAgent(PimDataProviderAgent &&) = delete;
    PimDataProviderAgent &operator=(PimDataProviderAgent &&) = delete;

    [[nodiscard]] QList<QVariantMap> getServers() const;
    [[nodiscard]] PimDataProviderItemList getCalendars() const;

Q_SIGNALS:
    /**
     * Emitted when a new server is created
     */
    void serverCreated(PimDataProviderItem server);

    /**
     * Emitted when a server is deleted
     */
    void serverDeleted(const QString &serverId);

private:
    void slotServerAdded(const Akonadi::AgentInstance &instance);
    void slotServerRemoved(const Akonadi::AgentInstance &instance);

    QMap<QString, std::shared_ptr<PimDataProviderServer>> mServers;
};
}
