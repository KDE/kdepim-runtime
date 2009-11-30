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

#include "configdialog.h"

#include "settings.h"
#include "ui_configdialog.h"

#include <kaboutdata.h>
#include <kaboutapplicationdialog.h>
#include <kconfigdialogmanager.h>
#include <kwindowsystem.h>

ConfigDialog::ConfigDialog( WId windowId )
  : KDialog()
{
  if ( windowId )
    KWindowSystem::setMainWindow( this, windowId );

  Ui::ConfigDialog ui;
  ui.setupUi( mainWidget() );

  mManager = new KConfigDialogManager( this, Settings::self() );
  mManager->updateWidgets();

  connect( ui.kurllabel_about, SIGNAL( leftClickedUrl() ), this, SLOT( showAboutDialog() ) );
  connect( this, SIGNAL( okClicked() ), SLOT( save() ) );
}

void ConfigDialog::save()
{
  mManager->updateSettings();
  Settings::self()->writeConfig();
}

void ConfigDialog::showAboutDialog()
{
  KAboutData aboutData( "openxchange", "", ki18n( "Open-Xchange" ), "0.1",
                        ki18n( "Akonadi Open-Xchange Resource" ),
                        KAboutData::License_LGPL,
                        ki18n("(c) 2009 by Tobias Koenig (credativ GmbH)") );
  aboutData.addAuthor( ki18n( "Tobias Koenig" ), ki18n( "Current maintainer" ), "tokoe@kde.org" );
  aboutData.addCredit( ki18n( "credativ GmbH" ), ki18n( "Funded and supported" ), 0, "http://www.credativ.com" );

  KAboutApplicationDialog dlg( &aboutData, this );
  dlg.exec();
}
