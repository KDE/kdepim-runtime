/*
    Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "birthdaysconfigagentwidget.h"
#include "settings.h"
#include <AkonadiCore/Tag>
#include <kconfigdialogmanager.h>
#include <QIcon>
#include <KLocalizedString>
#include <QPushButton>
#include <KConfigGroup>
#include <KSharedConfig>

namespace {
static const char myConfigGroupName[] = "BirthdaysSettingsDialog";
}

BirthdaysConfigAgentWidget::BirthdaysConfigAgentWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
{
    //setWindowIcon(QIcon::fromTheme(QStringLiteral("view-calendar-birthday")));

    Settings::instance(config);

    QWidget *mainWidget = new QWidget(parent);
    ui.setupUi(mainWidget);
    parent->layout()->addWidget(mainWidget);

    mManager = new KConfigDialogManager(mainWidget, Settings::self());
    mManager->updateWidgets();
    ui.kcfg_AlarmDays->setSuffix(ki18np(" day", " days"));
}

BirthdaysConfigAgentWidget::~BirthdaysConfigAgentWidget()
{
}

void BirthdaysConfigAgentWidget::load()
{
    const QStringList categories = Settings::self()->filterCategories();
    ui.FilterCategories->setSelectionFromStringList(categories);
}

bool BirthdaysConfigAgentWidget::save() const
{
    mManager->updateSettings();

    Settings::self()->setFilterCategories(ui.FilterCategories->tagToStringList());
    Settings::self()->save();
    return true;
}

QSize BirthdaysConfigAgentWidget::restoreDialogSize() const
{
    auto group = config()->group(myConfigGroupName);
    const QSize size = group.readEntry("Size", QSize(600, 400));
    return size;
}

void BirthdaysConfigAgentWidget::saveDialogSize(const QSize &size)
{
    auto group = config()->group(myConfigGroupName);
    group.writeEntry("Size", size);
}
