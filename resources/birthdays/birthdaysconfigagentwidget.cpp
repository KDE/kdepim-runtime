/*
    SPDX-FileCopyrightText: 2003 Cornelius Schumacher <schumacher@kde.org>
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "birthdaysconfigagentwidget.h"
#include "settings.h"
#include <Akonadi/Tag>
#include <KConfigDialogManager>
#include <KLocalization>
#include <KLocalizedString>
#include <KSharedConfig>
#include <QIcon>
#include <QPushButton>
namespace
{
static const char myConfigGroupName[] = "BirthdaysSettingsDialog";
}

BirthdaysConfigAgentWidget::BirthdaysConfigAgentWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
{
    // setWindowIcon(QIcon::fromTheme(QStringLiteral("view-calendar-birthday")));

    Settings::instance(config);

    auto mainWidget = new QWidget(parent);
    ui.setupUi(mainWidget);
    parent->layout()->addWidget(mainWidget);

    mManager = new KConfigDialogManager(mainWidget, Settings::self());
    mManager->updateWidgets();
    KLocalization::setupSpinBoxFormatString(ui.kcfg_AlarmDays, ki18np("%v day", "%v days"));
}

BirthdaysConfigAgentWidget::~BirthdaysConfigAgentWidget() = default;

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

#include "moc_birthdaysconfigagentwidget.cpp"
