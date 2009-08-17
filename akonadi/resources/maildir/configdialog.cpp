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

#include <maildir.h>

#include <kconfigdialogmanager.h>
#include <kurlrequester.h>
#include <klineedit.h>

using KPIM::Maildir;

ConfigDialog::ConfigDialog(QWidget * parent) :
    KDialog( parent ),
    mToplevelIsContainer( false )
{
  ui.setupUi( mainWidget() );
  mManager = new KConfigDialogManager( this, Settings::self() );
  mManager->updateWidgets();
  ui.kcfg_Path->setMode( KFile::Directory | KFile::ExistingOnly );
  ui.kcfg_Path->setUrl( KUrl( Settings::self()->path() ) );

  connect( this, SIGNAL(okClicked()), SLOT(save()) );
  connect( ui.kcfg_Path->lineEdit(), SIGNAL(textChanged(QString)), SLOT(checkPath()) );

  checkPath();
}

void ConfigDialog::checkPath()
{
  bool ok = false;
  mToplevelIsContainer = false;
  QDir d( ui.kcfg_Path->url().toLocalFile() );
  if ( d.exists() ) {
    Maildir md( d.path() );
    QString error;
    if ( !md.isValid( error ) ) {
      Maildir md2( d.path(), true );
      if ( md2.isValid() ) {
        ui.statusLabel->setText( i18n( "The selected path contains valid Maildir folders." ) );
        mToplevelIsContainer = true;
        ok = true;
      } else {
        ui.statusLabel->setText( error );
      }
    } else {
      ui.statusLabel->setText( i18n( "The selected path is a valid Maildir." ) );
      ok = true;
    }
  } else {
    d.cdUp();
    if ( d.exists() ) {
      ui.statusLabel->setText( i18n( "The selected path does not exist yet, a new Maildir will be created." ) );
      ok = true;
    } else {
      ui.statusLabel->setText( i18n( "The selected path does not exist." ) );
    }
  }
  enableButton( Ok, ok );
}

void ConfigDialog::save()
{
  mManager->updateSettings();
  Settings::self()->setPath( ui.kcfg_Path->url().isLocalFile() ? ui.kcfg_Path->url().toLocalFile()  : ui.kcfg_Path->url().path() );
  Settings::self()->setTopLevelIsContainer( mToplevelIsContainer );
  Settings::self()->writeConfig();
}

#include "configdialog.moc"
