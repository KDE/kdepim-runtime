/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2018-2019 Laurent Montel <montel@kde.org>

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

#ifndef ICALDIRSETTINGSWIDGET_H
#define ICALDIRSETTINGSWIDGET_H

#include "ui_icaldirsagentsettingswidget.h"
#include <AkonadiCore/AgentConfigurationBase>

class KConfigDialogManager;

class IcalDirSettingsWidget : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    explicit IcalDirSettingsWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args);

    void load() override;
    bool save() const override;
private:
    void validate();
    Ui::IcalDirsAgentSettingsWidget ui;
    KConfigDialogManager *mManager = nullptr;
};

AKONADI_AGENTCONFIG_FACTORY(IcalDirSettingsWidgetFactory, "icaldirconfig.json", IcalDirSettingsWidget)
#endif
