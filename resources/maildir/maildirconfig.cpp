/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "maildirconfig.h"
MaildirConfig::MaildirConfig(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
    , mSettings(new Akonadi_Maildir_Resource::MaildirSettings(config))
    , mWidget(new ConfigWidget(mSettings.data(), identifier(), parent))
{
    connect(mWidget.data(), &ConfigWidget::okEnabled, this, &Akonadi::AgentConfigurationBase::enableOkButton);
}

MaildirConfig::~MaildirConfig() = default;

void MaildirConfig::load()
{
    Akonadi::AgentConfigurationBase::load();

    mWidget->load();
#if HAVE_ACTIVITY_SUPPORT
    mWidget->setActivitiesSettings(restoreActivitiesSettings());
#endif
}

bool MaildirConfig::save() const
{
    mWidget->save();
#if HAVE_ACTIVITY_SUPPORT
    saveActivitiesSettings(mWidget->activitiesSettings());
#endif

    return Akonadi::AgentConfigurationBase::save();
}

#include "moc_maildirconfig.cpp"
