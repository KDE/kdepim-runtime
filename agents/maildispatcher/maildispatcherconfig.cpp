/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "maildispatcherconfig.h"

#include <KNotifyConfigWidget>
#include <QLayout>

MailDispatcherConfig::MailDispatcherConfig(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &cfg)
    : Akonadi::AgentConfigurationBase(config, parent, cfg)
    , mWidget(new KNotifyConfigWidget(parent))
{
    mWidget->setApplication(QStringLiteral("akonadi_maildispatcher_agent"));
    parent->layout()->addWidget(mWidget);
}

bool MailDispatcherConfig::save() const
{
    mWidget->save();
    return Akonadi::AgentConfigurationBase::save();
}

#include "moc_maildispatcherconfig.cpp"
