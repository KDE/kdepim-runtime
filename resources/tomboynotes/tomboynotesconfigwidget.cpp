/*
    Copyright (c) 2016 Stefan Stäglich <sstaeglich@kdemail.net>
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

#include "tomboynotesconfigwidget.h"
#include "ui_tomboynotesagentconfigwidget.h"
#include "settings.h"
#include <KConfigDialogManager>
#include <KLocalizedString>

TomboyNotesConfigWidget::TomboyNotesConfigWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
    , ui(new Ui::TomboyNotesAgentConfigWidget)
{
    Settings::instance(config);

    QWidget *mainWidget = new QWidget(parent);
    ui->setupUi(mainWidget);
    parent->layout()->addWidget(mainWidget);

    // KSettings stuff. Works not in the initialization list!
    mManager = new KConfigDialogManager(mainWidget, Settings::self());
    mManager->updateWidgets();

    ui->kcfg_ServerURL->setReadOnly(!Settings::self()->requestToken().isEmpty());
}

TomboyNotesConfigWidget::~TomboyNotesConfigWidget()
{
    delete ui;
}

void TomboyNotesConfigWidget::load()
{
}

bool TomboyNotesConfigWidget::save() const
{
    if (ui->kcfg_ServerURL->text() != Settings::self()->serverURL()) {
        Settings::self()->setRequestToken(QString());
        Settings::self()->setRequestTokenSecret(QString());
    }

    if (ui->kcfg_collectionName->text().isEmpty()) {
        ui->kcfg_collectionName->setText(i18n("Tomboy Notes %1", Settings::serverURL()));
    }

    mManager->updateSettings();
    Settings::self()->save();
    return true;
}

QSize TomboyNotesConfigWidget::restoreDialogSize() const
{
    auto group = config()->group("TomboyNotesConfigWidget");
    const QSize size = group.readEntry("Size", QSize(600, 400));
    return size;
}

void TomboyNotesConfigWidget::saveDialogSize(const QSize &size)
{
    auto group = config()->group("TomboyNotesConfigWidget");
    group.writeEntry("Size", size);
}
