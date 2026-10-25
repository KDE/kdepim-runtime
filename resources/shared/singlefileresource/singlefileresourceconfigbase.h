/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "config-kdepim-runtime.h"
#include "singlefileresourceconfigwidget.h"
#include <Akonadi/AgentConfigurationBase>
#include <KLocalizedString>
#if HAVE_ACTIVITY_SUPPORT
#include <PimCommonActivities/ConfigureActivitiesWidget>
#endif

template<typename Settings>
class SingleFileResourceConfigBase : public Akonadi::AgentConfigurationBase
{
public:
    explicit SingleFileResourceConfigBase(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &list)
        : Akonadi::AgentConfigurationBase(config, parent, list)
        , mSettings(new Settings(config))
        , mWidget(new Akonadi::SingleFileResourceConfigWidget<Settings>(parent, mSettings.data()))
#if HAVE_ACTIVITY_SUPPORT
        , mConfigureActivitiesWidget(new PimCommonActivities::ConfigureActivitiesWidget(parent))
#endif
    {
#if HAVE_ACTIVITY_SUPPORT
        mWidget->addPage(i18n("Activities"), mConfigureActivitiesWidget.data());
#endif
    }

    ~SingleFileResourceConfigBase() override = default;

    void load() override
    {
        mWidget->load();
#if HAVE_ACTIVITY_SUPPORT
        PimCommonActivities::ActivitiesBaseManager::ActivitySettings settings;
        settings.enabled = mSettings->activitiesEnabled();
        settings.activities = mSettings->activities();
        mConfigureActivitiesWidget->setActivitiesSettings(settings);
#endif
        Akonadi::AgentConfigurationBase::load();
    }

    bool save() const override
    {
#if HAVE_ACTIVITY_SUPPORT
        const PimCommonActivities::ActivitiesBaseManager::ActivitySettings activitiesSettings = mConfigureActivitiesWidget->activitiesSettings();
        mSettings->setActivitiesEnabled(activitiesSettings.enabled);
        mSettings->setActivities(activitiesSettings.activities);
#endif
        if (!mWidget->save()) {
            return false;
        }
        return Akonadi::AgentConfigurationBase::save();
    }

protected:
    QScopedPointer<Settings> mSettings;
    QScopedPointer<Akonadi::SingleFileResourceConfigWidget<Settings>> mWidget;

private:
#if HAVE_ACTIVITY_SUPPORT
    QScopedPointer<PimCommonActivities::ConfigureActivitiesWidget> mConfigureActivitiesWidget;
#endif
};
