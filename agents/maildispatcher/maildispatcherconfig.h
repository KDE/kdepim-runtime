/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef MAILDISPATCHERCONFIG_H_
#define MAILDISPATCHERCONFIG_H_

#include <AkonadiCore/AgentConfigurationBase>

class KNotifyConfigWidget;
class MailDispatcherConfig : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    explicit MailDispatcherConfig(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &cfg);

    bool save() const override;

private:
    KNotifyConfigWidget *mWidget = nullptr;
};

AKONADI_AGENTCONFIG_FACTORY(MailDispatcherConfigFactory, "maildispatcherconfig.json", MailDispatcherConfig)

#endif
