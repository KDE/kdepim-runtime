/*
    SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "pimdataprovider_types.h"

#include <Akonadi/AgentWidgetBase>

namespace Akonadi
{
class PimDataProviderAgent : public AgentWidgetBase
{
    Q_OBJECT
public:
    explicit PimDataProviderAgent(const QString &id);
    ~PimDataProviderAgent() override;

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
};
}
