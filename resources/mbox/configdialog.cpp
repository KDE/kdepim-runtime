/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>
    Copyright (c) 2009 Bertjan Broeksema <b.broeksema@kdemail.net>

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

#include <kconfigdialogmanager.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>

ConfigDialog::ConfigDialog(QWidget * parent) :
    KDialog( parent )
{
  ui.setupUi( mainWidget() );
  mManager = new KConfigDialogManager( this, Settings::self() );
  mManager->updateWidgets();
  ui.kcfg_File->setUrl( KUrl( Settings::self()->file() ) );

  checkAvailableLockMethods();
  
  connect( this, SIGNAL(okClicked()), SLOT(save()) );
}

void ConfigDialog::checkAvailableLockMethods()
{
  // FIXME: I guess this whole checking makes only sense on linux machines.
  
  // Check for procmail lock method.
  if (KStandardDirs::findExe("lockfile") == QString()) {
    ui.procmail->setEnabled(false);
    if (ui.procmail->isChecked()) // Select another lock method if necessary
      ui.mutt_dotlock->setChecked(true);
  }

  // Check for mutt lock method.
  if (KStandardDirs::findExe("mutt_dotlock") == QString()) {
    ui.mutt_dotlock->setEnabled(false);
    ui.mutt_dotlock_privileged->setEnabled(false);
    if (ui.mutt_dotlock->isChecked() || ui.mutt_dotlock_privileged->isChecked())
    {
      if (ui.procmail->isEnabled())
        ui.procmail->setChecked(true);
      else
        ui.fcntl->setChecked(true);
    }
  }

  // FIXME: I assume fcntl is available on *nix machines. Maybe some kind of
  //       (compile time) check is needed here.
}

void ConfigDialog::save()
{
  mManager->updateSettings();
  Settings::self()->setFile( ui.kcfg_File->url().path() );
  Settings::self()->writeConfig();
}

#include "configdialog.moc"
