/*
    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2018-2025 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once
#include "config-kdepim-runtime.h"
#include "ui_icaldirsagentsettingswidget.h"
#include <Akonadi/AgentConfigurationBase>

class KConfigDialogManager;
#if HAVE_ACTIVITY_SUPPORT
namespace PimCommonActivities
{
class ConfigureActivitiesWidget;
}
#endif
class IcalDirSettingsWidget : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    explicit IcalDirSettingsWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args);

    void load() override;
    bool save() const override;

private:
    void validate();
    Ui::IcalDirsAgentSettingsWidget ui;
    KConfigDialogManager *mManager = nullptr;
#if HAVE_ACTIVITY_SUPPORT
    PimCommonActivities::ConfigureActivitiesWidget *const mConfigureActivitiesWidget;
#endif
};

AKONADI_AGENTCONFIG_FACTORY(IcalDirSettingsWidgetFactory, "icaldirconfig.json", IcalDirSettingsWidget)
