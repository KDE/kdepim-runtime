/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "maildirconfig.h"

MaildirConfig::MaildirConfig(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentWidgetConfigurationBase(config, parent, args)
    , mSettings(new Akonadi_Maildir_Resource::MaildirSettings(config))
    , mWidget(new ConfigWidget(mSettings.data(), identifier(), parent))
{
    connect(mWidget.data(), &ConfigWidget::okEnabled, this, &Akonadi::AgentWidgetConfigurationBase::enableOkButton);
}

MaildirConfig::~MaildirConfig() = default;

void MaildirConfig::load()
{
    Akonadi::AgentWidgetConfigurationBase::load();
    mWidget->load();
}

bool MaildirConfig::save() const
{
    mWidget->save();
    return Akonadi::AgentWidgetConfigurationBase::save();
}
