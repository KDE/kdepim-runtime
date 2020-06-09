/*
 * Copyright (C) 2020 by Shashwat Jolly <shashwat.jolly@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "configwidget.h"

#include <KConfigDialogManager>
#include <KLocalizedString>
#include <KMessageBox>
#include <QPushButton>

#include "etesyncconfig_debug.h"
#include "settings.h"
#include "ui_configwidget.h"

ConfigWidget::ConfigWidget(Settings *settings, QWidget *parent)
    : QWidget(parent)
{
    Ui::ConfigWidget ui;
    ui.setupUi(this);

    ui.kcfg_BaseUrl->setWhatsThis(i18n("Enter the http or https URL of your EteSync server here."));
    ui.kcfg_Username->setWhatsThis(i18n("Enter the username of your EteSync account here."));
    ui.kcfg_Password->setWhatsThis(i18n("Enter the password of your EteSync account here."));
    ui.kcfg_EncryptionPassword->setWhatsThis(i18n("Enter the encryption password for your EteSync account here."));

    ui.kcfg_BaseUrl->setText(QStringLiteral("https://api.etesync.com"));

    mServerEdit = ui.kcfg_BaseUrl;
    mUserEdit = ui.kcfg_Username;
    mPasswordEdit = ui.kcfg_Password;
    mEncryptionPasswordEdit = ui.kcfg_EncryptionPassword;

    mManager = new KConfigDialogManager(this, settings);

    setMinimumSize(QSize(380, 180));
}

void ConfigWidget::load()
{
    mManager->updateWidgets();
}

void ConfigWidget::save() const
{
    mManager->updateSettings();
}