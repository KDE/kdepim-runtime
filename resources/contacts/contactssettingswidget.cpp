/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>
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

#include "contactssettingswidget.h"
#include "settings.h"

#include <KConfigDialogManager>

#include <QTimer>
#include <KSharedConfig>
#include <QUrl>
#include <QPushButton>

namespace {
static const char myConfigGroupName[] = "ContactsSettingsDialog";
}

ContactsSettingsWidget::ContactsSettingsWidget(const KSharedConfigPtr &config, QWidget *parent, const QVariantList &args)
    : Akonadi::AgentConfigurationBase(config, parent, args)
{
    ContactsResourceSettings::instance(config);

    QWidget *mainWidget = new QWidget(parent);

    ui.setupUi(mainWidget);
    parent->layout()->addWidget(mainWidget);
    ui.kcfg_Path->setMode(KFile::LocalOnly | KFile::Directory);

    ui.label_3->setMinimumSize(ui.label_3->sizeHint());
    ui.label_2->setMinimumSize(ui.label_2->sizeHint());
    mManager = new KConfigDialogManager(mainWidget, ContactsResourceSettings::self());

    connect(ui.kcfg_Path, &KUrlRequester::textChanged, this, &ContactsSettingsWidget::validate);
    connect(ui.kcfg_ReadOnly, &QCheckBox::toggled, this, &ContactsSettingsWidget::validate);
}

ContactsSettingsWidget::~ContactsSettingsWidget()
{
}

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
    mManager->updateWidgets();
}

bool ContactsSettingsWidget::save() const
{
    mManager->updateSettings();
    ContactsResourceSettings::self()->setPath(ui.kcfg_Path->url().toLocalFile());
    ContactsResourceSettings::self()->save();
    return true;
}

QSize ContactsSettingsWidget::restoreDialogSize() const
{
    auto group = config()->group(myConfigGroupName);
    const QSize size = group.readEntry("Size", QSize(600, 400));
    return size;
}

void ContactsSettingsWidget::saveDialogSize(const QSize &size)
{
    auto group = config()->group(myConfigGroupName);
    group.writeEntry("Size", size);
}
