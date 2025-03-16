// SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>
// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include <Akonadi/AgentConfigurationBase>

#include "configwidget.h"
#include "settings.h"

class DavGroupwareConfig : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    DavGroupwareConfig(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
        : Akonadi::AgentConfigurationBase(config, parent, args)
        , mSettings(config, Settings::Option::NoOption)
        , mWidget(mSettings, identifier(), parent)
    {
        connect(&mWidget, &ConfigWidget::okEnabled, this, &Akonadi::AgentConfigurationBase::enableOkButton);
    }

    void load() override
    {
        Akonadi::AgentConfigurationBase::load();
        mWidget.loadSettings();
    }

    [[nodiscard]] bool save() const override
    {
        mWidget.saveSettings();
        return Akonadi::AgentConfigurationBase::save();
    }

    Settings mSettings;
    ConfigWidget mWidget;
};

AKONADI_AGENTCONFIG_FACTORY(DavGroupwareConfigFactory, "davgroupwareconfig.json", DavGroupwareConfig)

#include "davgroupwareconfig.moc"
