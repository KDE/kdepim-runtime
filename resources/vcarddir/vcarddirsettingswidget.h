/*
    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2018-2024 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once
#include "config-kdepim-runtime.h"
#include "ui_vcarddiragentsettingswidget.h"
#include <Akonadi/AgentConfigurationBase>

class KConfigDialogManager;
#if HAVE_ACTIVITY_SUPPORT
namespace PimCommonActivities
{
class ConfigureActivitiesWidget;
}
#endif
class VcardDirSettingsWidget : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    explicit VcardDirSettingsWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args);

    void load() override;
    bool save() const override;

private:
    void validate();
    Ui::VcardDirsAgentSettingsWidget ui;
    KConfigDialogManager *mManager = nullptr;
#if HAVE_ACTIVITY_SUPPORT
    PimCommonActivities::ConfigureActivitiesWidget *const mConfigureActivitiesWidget;
#endif
};

AKONADI_AGENTCONFIG_FACTORY(VcardDirSettingsWidgetFactory, "vcarddirconfig.json", VcardDirSettingsWidget)
