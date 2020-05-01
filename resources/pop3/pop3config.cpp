/* Copyright 2018 Daniel Vrátil <dvratil@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <AkonadiCore/AgentConfigurationBase>

#include "accountwidget.h"
#include "settings.h"

class Pop3Config : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    Pop3Config(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
        : Akonadi::AgentConfigurationBase(config, parent, args)
        , mSettings(config, Settings::Option::NoOption)
        , mWidget(mSettings, identifier(), parent)
    {
        connect(&mWidget, &AccountWidget::okEnabled, this, &Akonadi::AgentConfigurationBase::enableOkButton);
    }

    void load() override
    {
        Akonadi::AgentConfigurationBase::load();
        mWidget.loadSettings();
    }

    bool save() const override
    {
        const_cast<Pop3Config *>(this)->mWidget.saveSettings();
        return Akonadi::AgentConfigurationBase::save();
    }

    Settings mSettings;
    AccountWidget mWidget;
};

AKONADI_AGENTCONFIG_FACTORY(Pop3ConfigFactory, "pop3config.json", Pop3Config)

#include "pop3config.moc"
