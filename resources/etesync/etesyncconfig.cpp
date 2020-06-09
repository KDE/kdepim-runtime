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

#include <AkonadiCore/AgentConfigurationBase>

#include "configwidget.h"
#include "etesyncconfig_debug.h"
#include "settings.h"

class EteSyncConfig : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    EteSyncConfig(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
        : Akonadi::AgentConfigurationBase(config, parent, args)
    {
        Settings::instance(config);
        mSettings.reset(Settings::self());
        mWidget.reset(new ConfigWidget(mSettings.data(), parent));
    }

    void load() override
    {
        Akonadi::AgentConfigurationBase::load();
        mWidget->load();
    }

    bool save() const override
    {
        mWidget->save();
        return Akonadi::AgentConfigurationBase::save();
    }

    QScopedPointer<Settings> mSettings;
    QScopedPointer<ConfigWidget> mWidget;
};

AKONADI_AGENTCONFIG_FACTORY(EteSyncConfigFactory, "etesyncconfig.json", EteSyncConfig)

#include "etesyncconfig.moc"
