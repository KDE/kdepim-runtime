/*
    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2018-2021 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
