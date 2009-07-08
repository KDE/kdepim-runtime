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

using namespace Akonadi;

SettingsDialog::SettingsDialog( WId windowId )
  : KDialog()
{
  ui.setupUi( mainWidget() );
  ui.kcfg_Path->setMode( KFile::Directory );
  setButtons( Ok | Cancel );

  if ( windowId )
    KWindowSystem::setMainWindow( this, windowId );

  connect( this, SIGNAL(okClicked()), SLOT(save()) );

  connect( ui.kcfg_Path, SIGNAL( textChanged( QString ) ), SLOT( validate() ) );
  connect( ui.kcfg_ReadOnly, SIGNAL( toggled( bool ) ), SLOT( validate() ) );

  QTimer::singleShot( 0, this, SLOT( validate() ) );

  ui.kcfg_Path->setUrl( KUrl( Settings::self()->path() ) );
  mManager = new KConfigDialogManager( this, Settings::self() );
  mManager->updateWidgets();
}

void SettingsDialog::save()
{
  mManager->updateSettings();
  Settings::self()->setPath( ui.kcfg_Path->url().url() );
  Settings::self()->writeConfig();
}

void SettingsDialog::validate()
{
  const KUrl currentUrl = ui.kcfg_Path->url();
  if ( currentUrl.isEmpty() ) {
    enableButton( Ok, false );
    return;
  }

  const QFileInfo file( currentUrl.toLocalFile() );
  if ( file.exists() && !file.isWritable() ) {
    ui.kcfg_ReadOnly->setEnabled( false );
    ui.kcfg_ReadOnly->setChecked( true );
  } else {
    ui.kcfg_ReadOnly->setEnabled( true );
  }
  enableButton( Ok, true );
}
