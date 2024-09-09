/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <Akonadi/AgentConfigurationBase>

#include "outlooksettings.h"
#include "outlooksettingswidget.h"

class OutlookConfig : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT

public:
    explicit OutlookConfig(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &list)
        : Akonadi::AgentConfigurationBase(config, parent, list)
        , mSettings(config)
        , mWidget(mSettings, identifier(), parent)
    {
        connect(&mWidget, &OutlookSettingsWidget::okEnabled, this, &Akonadi::AgentConfigurationBase::enableOkButton);
    }

    void load() override
    {
        Akonadi::AgentConfigurationBase::load();
        mSettings.init();
    }

    [[nodiscard]] bool save() const override
    {
        const_cast<OutlookConfig *>(this)->mWidget.saveSettings();
        return Akonadi::AgentConfigurationBase::save();
    }

    OutlookSettings mSettings;
    OutlookSettingsWidget mWidget;
};

AKONADI_AGENTCONFIG_FACTORY(OutlookConfigFactory, "outlookconfig.json", OutlookConfig)

#include "outlookconfig.moc"
