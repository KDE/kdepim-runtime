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

#include <kconfigdialogmanager.h>
#include <kdebug.h>
#include <kmessagebox.h>

ConfigDialog::ConfigDialog(QWidget * parent) :
    KDialog( parent )
{
  m_comm = new Communication( this );
  connect( m_comm, SIGNAL( authOk() ), SLOT( slotAuthOk() ) );
  connect( m_comm, SIGNAL( authFailed( const QString& ) ), SLOT( slotAuthFailed( const QString& ) ) );
  ui.setupUi( mainWidget() );
  mManager = new KConfigDialogManager( this, Settings::self() );
  mManager->updateWidgets();
  ui.password->setText( Settings::self()->password() );
  setButtons( KDialog::User1 | KDialog::Cancel );
  setButtonText( KDialog::User1, i18n("Test settings && Ok") );
  setDefaultButton( KDialog::User1 );
  connect( this, SIGNAL(user1Clicked()), SLOT(slotTestClicked()) );
}

ConfigDialog::~ConfigDialog() 
{
  delete m_comm;
}

void ConfigDialog::slotTestClicked()
{
  kDebug() << "Test request" << ui.kcfg_Service->currentIndex() 
          << ui.kcfg_UserName->text() << ui.password->text();

  enableButton( KDialog::User1, false );
  m_comm->checkAuth( ui.kcfg_Service->currentIndex(), ui.kcfg_UserName->text(), ui.password->text() );
}

void ConfigDialog::slotAuthOk()
{
  enableButton( KDialog::User1, true );
  Settings::self()->setPassword( ui.password->text() );
  mManager->updateSettings();
  accept();
}

void ConfigDialog::slotAuthFailed( const QString& error )
{
  KMessageBox::error( this, error, i18n("Failed to log in") );
  enableButton( KDialog::User1, true );
}

#include "configdialog.moc"
