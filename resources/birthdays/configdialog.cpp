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

#include "configdialog.h"
#include "settings.h"
#include <AkonadiCore/Tag>
#include <kconfigdialogmanager.h>
#include <QIcon>
#include <KLocalizedString>
#include <QPushButton>
#include <QDialogButtonBox>

ConfigDialog::ConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    QWidget *mainWidget = new QWidget(this);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConfigDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ConfigDialog::reject);
    okButton->setDefault(true);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(mainWidget);
    mainLayout->addWidget(buttonBox);
    ui.setupUi(mainWidget);
    setWindowIcon(QIcon::fromTheme(QLatin1String("view-calendar-birthday")));
    mManager = new KConfigDialogManager(this, Settings::self());
    mManager->updateWidgets();
    ui.kcfg_AlarmDays->setSuffix(ki18np(" day", " days"));

    connect(okButton, &QPushButton::clicked, this, &ConfigDialog::save);
    loadTags();
    readConfig();
}

ConfigDialog::~ConfigDialog()
{
    writeConfig();
}

void ConfigDialog::loadTags()
{
    const QStringList categories = Settings::self()->filterCategories();
    ui.FilterCategories->setSelectionFromStringList(categories);
}

void ConfigDialog::save()
{
  mManager->updateSettings();

  Settings::self()->setFilterCategories(ui.FilterCategories->tagToStringList());
  Settings::self()->save();
}

void ConfigDialog::readConfig()
{
    KConfigGroup group( KSharedConfig::openConfig(), "ConfigDialog" );
    const QSize size = group.readEntry( "Size", QSize(600, 400) );
    if ( size.isValid() ) {
        resize( size );
    }
}

void ConfigDialog::writeConfig()
{
    KConfigGroup group( KSharedConfig::openConfig(), "ConfigDialog" );
    group.writeEntry( "Size", size() );
    group.sync();
}

