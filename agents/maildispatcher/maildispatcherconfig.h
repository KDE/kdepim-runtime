/*
    Copyright 2018 Daniel Vrátil <dvratil@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
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
