/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "maildirconfig.h"
#if HAVE_ACTIVITY_SUPPORT
#include <PimCommonActivities/ConfigureActivitiesWidget>
#endif
MaildirConfig::MaildirConfig(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
    , mSettings(new Akonadi_Maildir_Resource::MaildirSettings(config))
    , mWidget(new ConfigWidget(mSettings.data(), identifier(), parent))
#if HAVE_ACTIVITY_SUPPORT
    , mConfigureActivitiesWidget(new PimCommonActivities::ConfigureActivitiesWidget(parent))
#endif
{
    connect(mWidget.data(), &ConfigWidget::okEnabled, this, &Akonadi::AgentConfigurationBase::enableOkButton);
}

MaildirConfig::~MaildirConfig() = default;

void MaildirConfig::load()
{
    Akonadi::AgentConfigurationBase::load();
#if HAVE_ACTIVITY_SUPPORT
    Akonadi::AgentConfigurationBase::ActivitySettings settingsBase = restoreActivitiesSettings();
    PimCommonActivities::ActivitiesBaseManager::ActivitySettings settings;
    settings.enabled = settingsBase.enabled;
    settings.activities = settingsBase.activities;
    qDebug() << "read activities settings " << settings;
    mConfigureActivitiesWidget->setActivitiesSettings(settings);
#endif
    mWidget->load();
}

bool MaildirConfig::save() const
{
    mWidget->save();
#if HAVE_ACTIVITY_SUPPORT
    const PimCommonActivities::ActivitiesBaseManager::ActivitySettings activitiesSettings = mConfigureActivitiesWidget->activitiesSettings();
    saveActivitiesSettings(Akonadi::AgentConfigurationBase::ActivitySettings{activitiesSettings.enabled, activitiesSettings.activities});
#endif
    return Akonadi::AgentConfigurationBase::save();
}

#include "moc_maildirconfig.cpp"
