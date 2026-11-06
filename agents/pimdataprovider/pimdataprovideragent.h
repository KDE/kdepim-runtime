/*
 * SPDX-FileCopyrightText: 2026 Noham Devillers <noham.devillers@enioka.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 *
 */

#pragma once

#include <Akonadi/AgentBase>

namespace Akonadi
{
class PimDataProviderAgent : public AgentBase
{
    Q_OBJECT
public:
    explicit PimDataProviderAgent(const QString &id);
    ~PimDataProviderAgent() override;
};
}
