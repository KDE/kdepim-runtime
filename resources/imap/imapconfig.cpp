// SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>
// SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL

#include <Akonadi/AgentConfigurationBase>

#include "settings.h"
#include "setupserver.h"

class ImapConfig : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    ImapConfig(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
        : Akonadi::AgentConfigurationBase(config, parent, args)
        , mSettings(config, Settings::Option::NoOption)
        , mWidget(mSettings, identifier(), parent)
    {
        connect(&mWidget, &SetupServer::okEnabled, this, &Akonadi::AgentConfigurationBase::enableOkButton);
    }

    void load() override
    {
        Akonadi::AgentConfigurationBase::load();
        mWidget.loadSettings();
    }

    [[nodiscard]] bool save() const override
    {
        const_cast<ImapConfig *>(this)->mWidget.saveSettings();
        return Akonadi::AgentConfigurationBase::save();
    }

    Settings mSettings;
    SetupServer mWidget;
};

AKONADI_AGENTCONFIG_FACTORY(ImapConfigFactory, "imapconfig.json", ImapConfig)

#include "imapconfig.moc"
