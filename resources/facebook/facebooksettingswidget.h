/*
 *    Copyright (C) 2017 Daniel Vrátil <dvratil@kde.org>
 *    Copyright (C) 2018 Laurent Montel <montel@kde.org>
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FACEBOOKSETTINGSWIDGET_H_
#define FACEBOOKSETTINGSWIDGET_H_

#include <QScopedPointer>
#include <AkonadiCore/AgentConfigurationBase>

class Ui_FacebookAgentSettingsWidget;
class FacebookSettingsWidget : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    explicit FacebookSettingsWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args);
    ~FacebookSettingsWidget() override;

    void load() override;
    bool save() const override;

private Q_SLOT:
    void checkToken();
    void login();
    void logout();

private:
    QScopedPointer<Ui_FacebookAgentSettingsWidget> ui;

};
AKONADI_AGENTCONFIG_FACTORY(FacebookSettingsWidgetFactory, "facebookconfig.json", FacebookSettingsWidget)
#endif
