/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <Akonadi/AgentConfigurationBase>

#include "googleresource_debug.h"
#include "googlesettings.h"
#include "googlesettingswidget.h"

class GoogleConfig : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT

public:
    explicit GoogleConfig(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &list)
        : Akonadi::AgentConfigurationBase(config, parent, list)
        , mSettings(config, GoogleSettings::Option::NoOption)
        , mWidget(mSettings, identifier(), parent)
    {
        connect(&mWidget, &GoogleSettingsWidget::okEnabled, this, &Akonadi::AgentConfigurationBase::enableOkButton);
    }

    void load() override
    {
        Akonadi::AgentConfigurationBase::load();
        mSettings.init();
        connect(&mSettings, &GoogleSettings::accountReady, this, [this](bool ready) {
            if (ready) {
                mWidget.loadSettings();
            }
        });
    }

    [[nodiscard]] bool save() const override
    {
        const_cast<GoogleConfig *>(this)->mWidget.saveSettings();
        return Akonadi::AgentConfigurationBase::save();
    }

    GoogleSettings mSettings;
    GoogleSettingsWidget mWidget;
};

AKONADI_AGENTCONFIG_FACTORY(GoogleConfigFactory, "googleconfig.json", GoogleConfig)

#include "googleconfig.moc"
