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

#include <kconfigdialogmanager.h>

#include <Akonadi/Tag>

ConfigDialog::ConfigDialog(QWidget* parent)
  : KDialog( parent )
{
  ui.setupUi( mainWidget() );
  setWindowIcon( KIcon( QLatin1String("view-calendar-birthday") ) );
  mManager = new KConfigDialogManager( this, Settings::self() );
  mManager->updateWidgets();
  ui.kcfg_AlarmDays->setSuffix( ki18np( " day", " days" ) );

  connect( this, SIGNAL(okClicked()), SLOT(save()) );
  loadTags();
  readConfig();
}

ConfigDialog::~ConfigDialog()
{
    writeConfig();
}

void ConfigDialog::loadTags()
{
    Akonadi::Tag::List tags;

    const QStringList categories = Settings::self()->filterCategories();
    foreach (const QString &category, categories) {
        tags.append(Akonadi::Tag::fromUrl(category));
    }
    ui.FilterCategories->setSelection(tags);
}

void ConfigDialog::save()
{
  mManager->updateSettings();

  QStringList list;
  const Akonadi::Tag::List tags = ui.FilterCategories->selection();
  foreach (const Akonadi::Tag &tag, tags) {
      list.append(tag.url().url());
  }
  Settings::self()->setFilterCategories(list);
  Settings::self()->writeConfig();
}

void ConfigDialog::readConfig()
{
    KConfigGroup group( KGlobal::config(), "ConfigDialog" );
    const QSize size = group.readEntry( "Size", QSize(600, 400) );
    if ( size.isValid() ) {
        resize( size );
    }
}

void ConfigDialog::writeConfig()
{
    KConfigGroup group( KGlobal::config(), "ConfigDialog" );
    group.writeEntry( "Size", size() );
    group.sync();
}
