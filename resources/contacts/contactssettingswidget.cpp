/*
    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2018-2025 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "contactssettingswidget.h"
#include "settings.h"

#include <KConfigDialogManager>

#include <KSharedConfig>
#include <QPushButton>
#include <QTimer>
#include <QUrl>
#if HAVE_ACTIVITY_SUPPORT
#include <PimCommonActivities/ActivitiesBaseManager>
#include <PimCommonActivities/ConfigureActivitiesWidget>
#endif
namespace
{
static const char myConfigGroupName[] = "ContactsSettingsDialog";
}

ContactsSettingsWidget::ContactsSettingsWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
#if HAVE_ACTIVITY_SUPPORT
    , mConfigureActivitiesWidget(new PimCommonActivities::ConfigureActivitiesWidget(parent))
#endif
{
    ContactsResourceSettings::instance(config);

    auto mainWidget = new QWidget(parent);

    ui.setupUi(mainWidget);
    parent->layout()->addWidget(mainWidget);
    ui.kcfg_Path->setMode(KFile::LocalOnly | KFile::Directory);
#if HAVE_ACTIVITY_SUPPORT
    ui.tabWidget->addTab(mConfigureActivitiesWidget, i18n("Activities"));
#endif

    ui.label_3->setMinimumSize(ui.label_3->sizeHint());
    ui.label_2->setMinimumSize(ui.label_2->sizeHint());
    mManager = new KConfigDialogManager(mainWidget, ContactsResourceSettings::self());

    connect(ui.kcfg_Path, &KUrlRequester::textChanged, this, &ContactsSettingsWidget::validate);
    connect(ui.kcfg_ReadOnly, &QCheckBox::toggled, this, &ContactsSettingsWidget::validate);
}

ContactsSettingsWidget::~ContactsSettingsWidget() = default;

void ContactsSettingsWidget::validate()
{
    const QUrl currentUrl = ui.kcfg_Path->url();
    if (currentUrl.isEmpty()) {
        Q_EMIT enableOkButton(false);
        return;
    }

    const QFileInfo file(currentUrl.toLocalFile());
    if (file.exists() && !file.isWritable()) {
        ui.kcfg_ReadOnly->setEnabled(false);
        ui.kcfg_ReadOnly->setChecked(true);
    } else {
        ui.kcfg_ReadOnly->setEnabled(true);
    }
    Q_EMIT enableOkButton(true);
}

void ContactsSettingsWidget::load()
{
    QTimer::singleShot(0, this, &ContactsSettingsWidget::validate);

    ui.kcfg_Path->setUrl(QUrl::fromLocalFile(ContactsResourceSettings::self()->path()));
#if HAVE_ACTIVITY_SUPPORT
    Akonadi::AgentConfigurationBase::ActivitySettings settingsBase = restoreActivitiesSettings();
    PimCommonActivities::ActivitiesBaseManager::ActivitySettings settings;
    settings.enabled = settingsBase.enabled;
    settings.activities = settingsBase.activities;
    qDebug() << "read activities settings " << settings;
    mConfigureActivitiesWidget->setActivitiesSettings(settings);
#endif
    mManager->updateWidgets();
}

bool ContactsSettingsWidget::save() const
{
    mManager->updateSettings();
#if HAVE_ACTIVITY_SUPPORT
    const PimCommonActivities::ActivitiesBaseManager::ActivitySettings activitiesSettings = mConfigureActivitiesWidget->activitiesSettings();
    saveActivitiesSettings(Akonadi::AgentConfigurationBase::ActivitySettings{activitiesSettings.enabled, activitiesSettings.activities});
#endif
    ContactsResourceSettings::self()->setPath(ui.kcfg_Path->url().toLocalFile());
    ContactsResourceSettings::self()->save();
    return true;
}

QSize ContactsSettingsWidget::restoreDialogSize() const
{
    auto group = config()->group(QLatin1StringView(myConfigGroupName));
    const QSize size = group.readEntry("Size", QSize(600, 400));
    return size;
}

void ContactsSettingsWidget::saveDialogSize(const QSize &size)
{
    auto group = config()->group(QLatin1StringView(myConfigGroupName));
    group.writeEntry("Size", size);
}

#include "moc_contactssettingswidget.cpp"
