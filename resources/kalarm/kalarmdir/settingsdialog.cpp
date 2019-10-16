/*
 *  settingsdialog.cpp  -  Akonadi KAlarm directory resource configuration dialog
 *  Program:  kalarm
 *  Copyright © 2011 by David Jarvie <djarvie@kde.org>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */

#include "settingsdialog.h"
#include "settings.h"
#include "alarmtypewidget.h"

#include <KConfigDialogManager>
#include <KWindowSystem>

#include <QUrl>
#include <QTimer>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace Akonadi_KAlarm_Dir_Resource {
SettingsDialog::SettingsDialog(WId windowId, Settings *settings)
    : QDialog()
    , mSettings(settings)
    , mReadOnlySelected(false)
{
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainWidget);
    ui.setupUi(mainWidget);
    mTypeSelector = new AlarmTypeWidget(ui.tab, ui.tabLayout);
    ui.kcfg_Path->setMode(KFile::LocalOnly | KFile::Directory);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mOkButton = buttonBox->button(QDialogButtonBox::Ok);
    mOkButton->setDefault(true);
    mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    mainLayout->addWidget(buttonBox);
    setWindowTitle(i18nc("@title", "Configure Calendar"));

    if (windowId) {
        KWindowSystem::setMainWindow(windowHandle(), windowId);
    }

    // Make directory path read-only if the resource already exists
    QUrl path = QUrl::fromLocalFile(mSettings->path());
    ui.kcfg_Path->setUrl(path);
    if (!path.isEmpty()) {
        ui.kcfg_Path->setEnabled(false);
    }

    mTypeSelector->setAlarmTypes(CalEvent::types(mSettings->alarmTypes()));
    mManager = new KConfigDialogManager(this, mSettings);
    mManager->updateWidgets();

    connect(mOkButton, &QPushButton::clicked, this, &SettingsDialog::save);
    connect(ui.kcfg_Path, &KUrlRequester::textChanged, this, &SettingsDialog::textChanged);
    connect(ui.kcfg_ReadOnly, &QCheckBox::clicked, this, &SettingsDialog::readOnlyClicked);
    connect(mTypeSelector, &AlarmTypeWidget::changed, this, &SettingsDialog::validate);

    QTimer::singleShot(0, this, &SettingsDialog::validate);
}

void SettingsDialog::save()
{
    mManager->updateSettings();
    mSettings->setPath(ui.kcfg_Path->url().toLocalFile());
    mSettings->setAlarmTypes(CalEvent::mimeTypes(mTypeSelector->alarmTypes()));
    mSettings->save();
}

void SettingsDialog::readOnlyClicked(bool set)
{
    mReadOnlySelected = set;
}

void SettingsDialog::textChanged()
{
    bool oldReadOnly = ui.kcfg_ReadOnly->isEnabled();
    validate();
    if (ui.kcfg_ReadOnly->isEnabled() && !oldReadOnly) {
        // If read-only was only set earlier by validating the input path,
        // and the path is now ok to be read-write, clear the read-only status.
        ui.kcfg_ReadOnly->setChecked(mReadOnlySelected);
    }
}

void SettingsDialog::validate()
{
    bool enableOk = false;
    // At least one alarm type must be selected
    if (mTypeSelector->alarmTypes() != CalEvent::EMPTY) {
        // The entered URL must be valid and local
        const QUrl currentUrl = ui.kcfg_Path->url();
        if (currentUrl.isEmpty()) {
            ui.kcfg_ReadOnly->setEnabled(true);
        } else if (currentUrl.isLocalFile()) {
            QFileInfo file(currentUrl.toLocalFile());
            // It must either be an existing directory, or in a writable
            // directory, and it must not be an existing file.
            if (file.exists()) {
                if (file.isDir()) {
                    enableOk = true;    // it's an existing directory
                }
            } else {
                // Specified directory doesn't already exist.
                // Find the first level of parent directory which exists,
                // and check that it is writable.
                for (;;) {
                    file.setFile(file.dir().absolutePath());   // get parent dir's file info
                    if (file.exists()) {
                        if (file.isDir() && file.isWritable()) {
                            enableOk = true;    // it's possible to create the directory
                        }
                        break;
                    }
                }
            }
        }
    }
    mOkButton->setEnabled(enableOk);
}
}
