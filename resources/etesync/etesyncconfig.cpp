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

#include "etesyncconfig.h"

#include <KConfigDialogManager>
#include <KSharedConfig>

#include "settings.h"

namespace {
    static const char myConfigGroupName[] = "EteSyncConfigDialog";
}

EteSyncConfig::EteSyncConfig(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
{
    Settings::instance(config);

    QWidget *mainWidget = new QWidget(parent);
    ui.setupUi(mainWidget);
    parent->layout()->addWidget(mainWidget);

    mManager = new KConfigDialogManager(mainWidget, Settings::self());
    mManager->updateWidgets();
}

bool EteSyncConfig::save() const
{
    mManager->updateSettings();
    return Akonadi::AgentConfigurationBase::save();
}

QSize EteSyncConfig::restoreDialogSize() const
{
    auto group = config()->group(myConfigGroupName);
    const QSize size = group.readEntry("Size", QSize(380, 180));
    return size;
}

void EteSyncConfig::saveDialogSize(const QSize &size)
{
    auto group = config()->group(myConfigGroupName);
    group.writeEntry("Size", size);
}
