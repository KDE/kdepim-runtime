/*
 * SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 *
 */

#include "pimdataprovideragent.h"

namespace Akonadi
{
PimDataProviderAgent::PimDataProviderAgent(const QString &id)
    : Akonadi::AgentBase(id)
{
}

PimDataProviderAgent::~PimDataProviderAgent() = default;
}

AKONADI_AGENT_CORE_MAIN(Akonadi::PimDataProviderAgent)

#include "moc_pimdataprovideragent.cpp"
