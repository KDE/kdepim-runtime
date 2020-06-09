/*
 * Copyright (C) 2020 by Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ETESYNCCONFIG_H
#define ETESYNCCONFIG_H

#include <AkonadiCore/AgentConfigurationBase>

#include "ui_etesyncconfigwidget.h"

class KConfigDialogManager;

class EteSyncConfig : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    explicit EteSyncConfig(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args);

    bool save() const override;
    QSize restoreDialogSize() const override;
    void saveDialogSize(const QSize &size) override;

private:
    Ui::EteSyncConfigWidget ui;
    KConfigDialogManager *mManager = nullptr;
};
AKONADI_AGENTCONFIG_FACTORY(EteSyncConfigFactory, "etesyncconfig.json", EteSyncConfig)

#endif
