/*
    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2018-2021 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CONTACTS_SETTINGSWIDGET_H
#define CONTACTS_SETTINGSWIDGET_H

#include "ui_contactsagentsettingswidget.h"
#include <AkonadiCore/AgentConfigurationBase>

class KConfigDialogManager;

class ContactsSettingsWidget : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    explicit ContactsSettingsWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args);
    ~ContactsSettingsWidget() override;

    void load() override;
    bool save() const override;
    QSize restoreDialogSize() const override;
    void saveDialogSize(const QSize &size) override;
private:
    void validate();
    Ui::ContactAgentSettingsWidget ui;
    KConfigDialogManager *mManager = nullptr;
};
AKONADI_AGENTCONFIG_FACTORY(ContactsSettingsWidgetFactory, "contactsconfig.json", ContactsSettingsWidget)

#endif
