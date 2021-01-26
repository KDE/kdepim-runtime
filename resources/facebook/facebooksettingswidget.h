/*
 *    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.org>
 *    SPDX-FileCopyrightText: 2018-2021 Laurent Montel <montel@kde.org>
 *
 *    SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef FACEBOOKSETTINGSWIDGET_H_
#define FACEBOOKSETTINGSWIDGET_H_

#include <AkonadiCore/AgentConfigurationBase>
#include <QScopedPointer>

class Ui_FacebookAgentSettingsWidget;
class FacebookSettingsWidget : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    explicit FacebookSettingsWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args);
    ~FacebookSettingsWidget() override;

    void load() override;
    bool save() const override;

private
    Q_SLOT : void checkToken();
    void login();
    void logout();

private:
    QScopedPointer<Ui_FacebookAgentSettingsWidget> ui;
};
AKONADI_AGENTCONFIG_FACTORY(FacebookSettingsWidgetFactory, "facebookconfig.json", FacebookSettingsWidget)
#endif
