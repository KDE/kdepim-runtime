/*
    Copyright (c) 2016 Stefan Stäglich <sstaeglich@kdemail.net>
    Copyright (c) 2018 Laurent Montel <montel@kde.org>

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

#ifndef TomboyNotesConfigWidget_H
#define TomboyNotesConfigWidget_H

#include <AkonadiCore/AgentConfigurationBase>
class KConfigDialogManager;

namespace Ui {
class TomboyNotesAgentConfigWidget;
}

class TomboyNotesConfigWidget  : public Akonadi::AgentConfigurationBase
{
    Q_OBJECT
public:
    explicit TomboyNotesConfigWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args);
    ~TomboyNotesConfigWidget() override;

    void load() override;
    bool save() const override;
    QSize restoreDialogSize() const override;
    void saveDialogSize(const QSize &size) override;

private:
    Ui::TomboyNotesAgentConfigWidget *ui = nullptr;

    KConfigDialogManager *mManager = nullptr;
};
AKONADI_AGENTCONFIG_FACTORY(TomboyNotesConfigWidgetFactory, "tomboynotesconfig.json", TomboyNotesConfigWidget)
#endif // TomboyNotesConfigWidget_H
