/*
    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>
    SPDX-FileCopyrightText: 2018-2024 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "icaldirsettingswidget.h"

#include "settings.h"

#include <KConfigDialogManager>
#include <KLocalization>
#include <KLocalizedString>
#include <QUrl>

#include <QFontDatabase>
#include <QPushButton>
#include <QTimer>
#if HAVE_ACTIVITY_SUPPORT
#include <PimCommonActivities/ConfigureActivitiesWidget>
#endif

IcalDirSettingsWidget::IcalDirSettingsWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
#if HAVE_ACTIVITY_SUPPORT
    , mConfigureActivitiesWidget(new PimCommonActivities::ConfigureActivitiesWidget(parent))
#endif
{
    IcalDirResourceSettings::instance(config);

    auto mainWidget = new QWidget(parent);

    ui.setupUi(mainWidget);
    parent->layout()->addWidget(mainWidget);

    ui.kcfg_Path->setMode(KFile::LocalOnly | KFile::Directory);

    connect(ui.kcfg_Path, &KUrlRequester::textChanged, this, &IcalDirSettingsWidget::validate);
    connect(ui.kcfg_ReadOnly, &QCheckBox::toggled, this, &IcalDirSettingsWidget::validate);

    ui.kcfg_Path->setUrl(QUrl::fromLocalFile(IcalDirResourceSettings::self()->path()));
#if KI18N_VERSION > QT_VERSION_CHECK(6, 5, 0)
    KLocalization::setupSpinBoxFormatString(ui.kcfg_AutosaveInterval, ki18np("%v minute", "%v minutes"));
#endif
    mManager = new KConfigDialogManager(mainWidget, IcalDirResourceSettings::self());
    ui.readOnlyLabel->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    ui.runingLabel->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    ui.pathLabel->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
#if HAVE_ACTIVITY_SUPPORT
    ui.ktabwidget->addTab(mConfigureActivitiesWidget, i18n("Activities"));
#endif
}

void IcalDirSettingsWidget::validate()
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

void IcalDirSettingsWidget::load()
{
    mManager->updateWidgets();
#if HAVE_ACTIVITY_SUPPORT
    Akonadi::AgentConfigurationBase::ActivitySettings settingsBase = restoreActivitiesSettings();
    PimCommonActivities::ActivitiesBaseManager::ActivitySettings settings;
    settings.enabled = settingsBase.enabled;
    settings.activities = settingsBase.activities;
    qDebug() << "read activities settings " << settings;
    mConfigureActivitiesWidget->setActivitiesSettings(settings);
#endif

    QTimer::singleShot(0, this, &IcalDirSettingsWidget::validate);
}

bool IcalDirSettingsWidget::save() const
{
    mManager->updateSettings();
#if HAVE_ACTIVITY_SUPPORT
    const PimCommonActivities::ActivitiesBaseManager::ActivitySettings activitiesSettings = mConfigureActivitiesWidget->activitiesSettings();
    saveActivitiesSettings(Akonadi::AgentConfigurationBase::ActivitySettings{activitiesSettings.enabled, activitiesSettings.activities});
#endif
    IcalDirResourceSettings::self()->setPath(ui.kcfg_Path->url().toLocalFile());
    IcalDirResourceSettings::self()->save();
    return true;
}

#include "moc_icaldirsettingswidget.cpp"
