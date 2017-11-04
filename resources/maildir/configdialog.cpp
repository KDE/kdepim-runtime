/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "configdialog.h"
#include "settings.h"
#include "resources/folderarchivesettings/folderarchivesettingpage.h"

#include <maildir.h>

#include <kconfigdialogmanager.h>
#include <kurlrequester.h>
#include <klineedit.h>
#include <QVBoxLayout>
#include <QDialogButtonBox>

using KPIM::Maildir;
using namespace Akonadi_Maildir_Resource;

ConfigDialog::ConfigDialog(MaildirSettings *settings, const QString &identifier, QWidget *parent)
    : QDialog(parent)
    , mSettings(settings)
    , mToplevelIsContainer(false)
{
    setWindowTitle(i18n("Select a MailDir folder"));
    QWidget *mainWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainWidget);
    ui.setupUi(mainWidget);
    mFolderArchiveSettingPage = new FolderArchiveSettingPage(identifier, this);
    mFolderArchiveSettingPage->loadSettings();
    ui.tabWidget->addTab(mFolderArchiveSettingPage, i18n("Archive Folder"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mOkButton = buttonBox->button(QDialogButtonBox::Ok);
    mOkButton->setDefault(true);
    mOkButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConfigDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ConfigDialog::reject);
    mainLayout->addWidget(buttonBox);

    mManager = new KConfigDialogManager(this, mSettings);
    mManager->updateWidgets();
    ui.kcfg_Path->setMode(KFile::Directory | KFile::ExistingOnly);
    ui.kcfg_Path->setUrl(QUrl::fromLocalFile(mSettings->path()));

    connect(mOkButton, &QPushButton::clicked, this, &ConfigDialog::save);
    connect(ui.kcfg_Path->lineEdit(), &QLineEdit::textChanged, this, &ConfigDialog::checkPath);
    ui.kcfg_Path->lineEdit()->setFocus();
    checkPath();
    readConfig();
}

ConfigDialog::~ConfigDialog()
{
    writeConfig();
}

void ConfigDialog::readConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "ConfigDialog");
    const QSize size = group.readEntry("Size", QSize(600, 400));
    if (size.isValid()) {
        resize(size);
    }
}

void ConfigDialog::writeConfig()
{
    KConfigGroup group(KSharedConfig::openConfig(), "ConfigDialog");
    group.writeEntry("Size", size());
    group.sync();
}

void ConfigDialog::checkPath()
{
    if (ui.kcfg_Path->url().isEmpty()) {
        ui.statusLabel->setText(i18n("The selected path is empty."));
        mOkButton->setEnabled(false);
        return;
    }
    bool ok = false;
    mToplevelIsContainer = false;
    QDir d(ui.kcfg_Path->url().toLocalFile());

    if (d.exists()) {
        Maildir md(d.path());
        if (!md.isValid(false)) {
            Maildir md2(d.path(), true);
            if (md2.isValid(false)) {
                ui.statusLabel->setText(i18n("The selected path contains valid Maildir folders."));
                mToplevelIsContainer = true;
                ok = true;
            } else {
                ui.statusLabel->setText(md.lastError());
            }
        } else {
            ui.statusLabel->setText(i18n("The selected path is a valid Maildir."));
            ok = true;
        }
    } else {
        d.cdUp();
        if (d.exists()) {
            ui.statusLabel->setText(i18n("The selected path does not exist yet, a new Maildir will be created."));
            mToplevelIsContainer = true;
            ok = true;
        } else {
            ui.statusLabel->setText(i18n("The selected path does not exist."));
        }
    }
    mOkButton->setEnabled(ok);
}

void ConfigDialog::save()
{
    mFolderArchiveSettingPage->writeSettings();
    mManager->updateSettings();
    QString path = ui.kcfg_Path->url().isLocalFile() ? ui.kcfg_Path->url().toLocalFile() : ui.kcfg_Path->url().path();
    mSettings->setPath(path);
    mSettings->setTopLevelIsContainer(mToplevelIsContainer);
    mSettings->save();

    if (ui.kcfg_Path->url().isLocalFile()) {
        QDir d(path);
        if (!d.exists()) {
            d.mkpath(ui.kcfg_Path->url().toLocalFile());
        }
    }
}
