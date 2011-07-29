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

#include "oxa/connectiontestjob.h"
#include "settings.h"
#include "ui_configdialog.h"

#include <kaboutdata.h>
#include <kaboutapplicationdialog.h>
#include <kconfigdialogmanager.h>
#include <kmessagebox.h>
#include <kwindowsystem.h>

ConfigDialog::ConfigDialog( WId windowId )
  : KDialog()
{
  if ( windowId )
    KWindowSystem::setMainWindow( this, windowId );

  setButtons( Ok | Cancel | User1 );
  setButtonText( User1, i18n( "About..." ) );

  setCaption( i18n( "Open-Xchange Configuration" ) );

  Ui::ConfigDialog ui;
  ui.setupUi( mainWidget() );

  ui.kcfg_BaseUrl->setWhatsThis( i18n( "Enter the http or https URL to your Open-Xchange installation here." ) );
  ui.kcfg_Username->setWhatsThis( i18n( "Enter the username of your Open-Xchange account here." ) );
  ui.kcfg_Password->setWhatsThis( i18n( "Enter the password of your Open-Xchange account here." ) );

  mServerEdit = ui.kcfg_BaseUrl;
  mUserEdit = ui.kcfg_Username;
  mPasswordEdit = ui.kcfg_Password;
  mCheckConnectionButton = ui.checkConnectionButton;

  mManager = new KConfigDialogManager( this, Settings::self() );
  mManager->updateWidgets();

  connect( this, SIGNAL(okClicked()), SLOT(save()) );
  connect( this, SIGNAL(user1Clicked()), this, SLOT(showAboutDialog()) );
  connect( mServerEdit, SIGNAL(textChanged(QString)), SLOT(updateButtonState()) );
  connect( mUserEdit, SIGNAL(textChanged(QString)), SLOT(updateButtonState()) );
  connect( mCheckConnectionButton, SIGNAL(clicked()), SLOT(checkConnection()) );

  setInitialSize( QSize( 410, 200 ) );
}

void ConfigDialog::save()
{
  mManager->updateSettings();
  Settings::self()->writeConfig();
}

void ConfigDialog::showAboutDialog()
{
  KAboutData aboutData( "ox", "", ki18n( "Open-Xchange" ), "0.1",
                        ki18n( "Akonadi Open-Xchange Resource" ),
                        KAboutData::License_LGPL,
                        ki18n("(c) 2009 by Tobias Koenig (credativ GmbH)") );
  aboutData.addAuthor( ki18n( "Tobias Koenig" ), ki18n( "Current maintainer" ), "tokoe@kde.org" );
  aboutData.addCredit( ki18n( "credativ GmbH" ), ki18n( "Funded and supported" ), 0, "http://www.credativ.com" );

  KAboutApplicationDialog dlg( &aboutData, this );
  dlg.exec();
}

void ConfigDialog::updateButtonState()
{
  mCheckConnectionButton->setEnabled( !mServerEdit->text().isEmpty() && !mUserEdit->text().isEmpty() );
}

void ConfigDialog::checkConnection()
{
  OXA::ConnectionTestJob *job = new OXA::ConnectionTestJob( mServerEdit->text(), mUserEdit->text(),
                                                            mPasswordEdit->text(), this );
  connect( job, SIGNAL(result(KJob*)), SLOT(checkConnectionJobFinished(KJob*)) );
  job->start();

  QApplication::setOverrideCursor( Qt::WaitCursor );
}

void ConfigDialog::checkConnectionJobFinished( KJob *job )
{
  QApplication::restoreOverrideCursor();

  if ( job->error() ) {
    KMessageBox::error( this, i18n( "Unable to connect: %1", job->errorText() ), i18n( "Connection error" ) );
  } else {
    KMessageBox::information( this, i18n( "Tested connection successfully." ), i18n( "Connection success" ) );
  }
}
