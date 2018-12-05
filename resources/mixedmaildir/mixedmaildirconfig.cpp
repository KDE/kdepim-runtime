/*  Copyright (C) 2018  Daniel Vrátil <dvratil@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <AkonadiCore/AgentConfigurationBase>

#include "configwidget.h"
#include "settings.h"

class MixedMaildirConfig : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    MixedMaildirConfig(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
        : Akonadi::AgentConfigurationBase(config, parent, args)
    {
        Settings::instance(config);
        mSettings.reset(Settings::self());
        mWidget.reset(new ConfigWidget(mSettings.data(), parent));
        connect(mWidget.data(), &ConfigWidget::okEnabled, this, &Akonadi::AgentConfigurationBase::enableOkButton);
    }

    void load() override
    {
        Akonadi::AgentConfigurationBase::load();
        mWidget->load(mSettings.data());
    }

    bool save() const override
    {
        mWidget->save(mSettings.data());
        mSettings->save();
        return Akonadi::AgentConfigurationBase::save();
    }

private:
    QScopedPointer<Settings> mSettings;
    QScopedPointer<ConfigWidget> mWidget;
};

AKONADI_AGENTCONFIG_FACTORY(MixedMaildirConfigFactory, "mixedmaildirconfig.json", MixedMaildirConfig)

#include "mixedmaildirconfig.moc"
