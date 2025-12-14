/*
    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2018-2025 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once
#include "config-kdepim-runtime.h"
#include "ui_contactsagentsettingswidget.h"
#include <Akonadi/AgentConfigurationBase>

class KConfigDialogManager;
#if HAVE_ACTIVITY_SUPPORT
namespace PimCommonActivities
{
class ConfigureActivitiesWidget;
}
#endif

class ContactsSettingsWidget : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    explicit ContactsSettingsWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args);
    ~ContactsSettingsWidget() override;

    void load() override;
    bool save() const override;

private:
    void validate();
    Ui::ContactAgentSettingsWidget ui;
    KConfigDialogManager *mManager = nullptr;
#if HAVE_ACTIVITY_SUPPORT
    PimCommonActivities::ConfigureActivitiesWidget *const mConfigureActivitiesWidget;
#endif
};
AKONADI_AGENTCONFIG_FACTORY(ContactsSettingsWidgetFactory, "contactsconfig.json", ContactsSettingsWidget)
