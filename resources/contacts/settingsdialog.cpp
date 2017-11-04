/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "settingsdialog.h"
#include "settings.h"

#include <KConfigDialogManager>
#include <KWindowSystem>

#include <QTimer>
#include <KSharedConfig>
#include <QUrl>
#include <QDialogButtonBox>
#include <KConfigGroup>
#include <QPushButton>
#include <QVBoxLayout>
using namespace Akonadi;
using namespace Akonadi_Contacts_Resource;

SettingsDialog::SettingsDialog(ContactsResourceSettings *settings, WId windowId)
    : QDialog()
    , mSettings(settings)
{
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainWidget);

    ui.setupUi(mainWidget);
    setWindowIcon(QIcon::fromTheme(QStringLiteral("text-directory")));
    ui.kcfg_Path->setMode(KFile::LocalOnly | KFile::Directory);

    ui.label_3->setMinimumSize(ui.label_3->sizeHint());
    ui.label_2->setMinimumSize(ui.label_2->sizeHint());

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mOkButton = buttonBox->button(QDialogButtonBox::Ok);
    mOkButton->setDefault(true);
    mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    mainLayout->addWidget(buttonBox);

    if (windowId) {
        KWindowSystem::setMainWindow(this, windowId);
    }

    connect(mOkButton, &QPushButton::clicked, this, &SettingsDialog::save);

    connect(ui.kcfg_Path, &KUrlRequester::textChanged, this, &SettingsDialog::validate);
    connect(ui.kcfg_ReadOnly, &QCheckBox::toggled, this, &SettingsDialog::validate);

    QTimer::singleShot(0, this, &SettingsDialog::validate);

    ui.kcfg_Path->setUrl(QUrl::fromLocalFile(mSettings->path()));
    mManager = new KConfigDialogManager(this, mSettings);
    mManager->updateWidgets();
    readConfig();
}

SettingsDialog::~SettingsDialog()
{
    writeConfig();
}

void SettingsDialog::save()
{
    mManager->updateSettings();
    mSettings->setPath(ui.kcfg_Path->url().toLocalFile());
    mSettings->save();
}

void SettingsDialog::validate()
{
    const QUrl currentUrl = ui.kcfg_Path->url();
    if (currentUrl.isEmpty()) {
        mOkButton->setEnabled(false);
        return;
    }

    const QFileInfo file(currentUrl.toLocalFile());
    if (file.exists() && !file.isWritable()) {
        ui.kcfg_ReadOnly->setEnabled(false);
        ui.kcfg_ReadOnly->setChecked(true);
    } else {
        ui.kcfg_ReadOnly->setEnabled(true);
    }
    mOkButton->setEnabled(true);
}

void SettingsDialog::readConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "SettingsDialog");
    const QSize size = group.readEntry("Size", QSize(600, 400));
    if (size.isValid()) {
        resize(size);
    }
}

void SettingsDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "SettingsDialog");
    group.writeEntry("Size", size());
    group.sync();
}
