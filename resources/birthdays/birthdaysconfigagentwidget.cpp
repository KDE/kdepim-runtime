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
#include <kbirthdaysconfigwidgetmanager.h>
#include <QIcon>
#include <KLocalizedString>
#include <QPushButton>
#include <KConfigGroup>
#include <KSharedConfig>

BirthdaysConfigAgentWidget::BirthdaysConfigAgentWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
{
    //setWindowIcon(QIcon::fromTheme(QStringLiteral("view-calendar-birthday")));

    QWidget *mainWidget = new QWidget(this);
    ui.setupUi(mainWidget);
    mainLayout->addWidget(mainWidget);

    mainLayout->addWidget(buttonBox);

    mManager = new KBirthdaysConfigWidgetManager(this, Settings::self());
    mManager->updateWidgets();
    ui.kcfg_AlarmDays->setSuffix(ki18np(" day", " days"));

    connect(okButton, &QPushButton::clicked, this, &BirthdaysConfigWidget::save);
    loadTags();
}

BirthdaysConfigAgentWidget::~BirthdaysConfigAgentWidget()
{
}

void BirthdaysConfigAgentWidget::load()
{

}

bool BirthdaysConfigAgentWidget::save() const
{

}

void BirthdaysConfigAgentWidget::loadTags()
{
    const QStringList categories = Settings::self()->filterCategories();
    ui.FilterCategories->setSelectionFromStringList(categories);
}

void BirthdaysConfigWidget::save()
{
    mManager->updateSettings();

    Settings::self()->setFilterCategories(ui.FilterCategories->tagToStringList());
    Settings::self()->save();
}

//void BirthdaysConfigWidget::readConfig()
//{
//    KConfigGroup group(KSharedConfig::openConfig(), "BirthdaysConfigWidget");
//    const QSize size = group.readEntry("Size", QSize(600, 400));
//    if (size.isValid()) {
//        resize(size);
//    }
//}

//void BirthdaysConfigWidget::writeConfig()
//{
//    KConfigGroup group(KSharedConfig::openConfig(), "BirthdaysConfigWidget");
//    group.writeEntry("Size", size());
//    group.sync();
//}


QSize BirthdaysConfigAgentWidget::restoreDialogSize() const
{
}

void BirthdaysConfigAgentWidget::saveDialogSize(const QSize &size)
{
}
