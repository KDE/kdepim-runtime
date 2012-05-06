/*
    Copyright (C) 2011, 2012  Dan Vratil <dan@progdan.cz>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "settings.h"

#include <KMessageBox>
#include <KWindowSystem>

#include <libkgoogle/auth.h>
#include <libkgoogle/services/contacts.h>

using namespace KGoogle;

enum {
  KGoogleObjectRole = Qt::UserRole,
  ObjectUIDRole
};

SettingsDialog::SettingsDialog( WId windowId, QWidget *parent ):
  KDialog( parent ),
  m_windowId( windowId )
{
  KWindowSystem::setMainWindow( this, windowId );

  qRegisterMetaType<KGoogle::Services::Contacts>( "Contacts" );

  this->setButtons( Ok | Cancel );

  m_mainWidget = new QWidget();
  m_ui = new ::Ui::SettingsDialog();
  m_ui->setupUi( m_mainWidget );
  setMainWidget( m_mainWidget );

  m_ui->addAccountBtn->setIcon( QIcon::fromTheme( "list-add-user" ) );
  m_ui->removeAccountBtn->setIcon( QIcon::fromTheme( "list-remove-user" ) );

  connect( this, SIGNAL(accepted()),
           this, SLOT(saveSettings()) );

  connect( m_ui->addAccountBtn, SIGNAL(clicked(bool)),
           this, SLOT(addAccountClicked()) );
  connect( m_ui->removeAccountBtn, SIGNAL(clicked(bool)),
           this, SLOT(removeAccountClicked()) );

  KGoogle::Auth *auth = KGoogle::Auth::instance();
  connect( auth, SIGNAL(authenticated(KGoogle::Account::Ptr&)),
           this, SLOT(reloadAccounts()) );

  reloadAccounts();
  updateButtons();
}

SettingsDialog::~SettingsDialog()
{
  delete m_ui;
  delete m_mainWidget;
}

void SettingsDialog::saveSettings()
{
  Settings::self()->setAccount( m_ui->accountsCombo->currentText() );
  Settings::self()->writeConfig();
}

void SettingsDialog::error( KGoogle::Error errCode, const QString &msg )
{
  if ( errCode == KGoogle::OK ) {
    return;
  }

  KMessageBox::error( this, msg, i18n( "An error occurred" ) );

  m_ui->accountsBox->setEnabled( true );
}

void SettingsDialog::reloadAccounts()
{
  m_ui->accountsCombo->reload();

  QString accName = Settings::self()->account();
  int index = -1;

  if ( !accName.isEmpty() ) {
    index = m_ui->accountsCombo->findText( accName );
  }

  if ( index > -1 ) {
    m_ui->accountsCombo->setCurrentIndex( index );
  }
}

void SettingsDialog::addAccountClicked()
{
  KGoogle::Auth *auth = KGoogle::Auth::instance();

  KGoogle::Account::Ptr account( new KGoogle::Account() );
  account->addScope( Services::Contacts::ScopeUrl );

  try {
    auth->authenticate( account, true );
    updateButtons();
  } catch ( KGoogle::Exception::BaseException &e ) {
    KMessageBox::error( this, e.what() );
  }
}

void SettingsDialog::removeAccountClicked()
{
  KGoogle::Account::Ptr account = m_ui->accountsCombo->currentAccount();

  if ( account.isNull() ) {
    return;
  }

  if ( KMessageBox::warningYesNo(
         this,
         i18n( "Do you really want to revoke access to account <b>%1</b>?"
               "<p>This will revoke access to all resources using this account!</p>",
               account->accountName() ),
         i18n( "Revoke Access?" ),
         KStandardGuiItem::yes(),
         KStandardGuiItem::no(),
         QString(),
         KMessageBox::Dangerous ) != KMessageBox::Yes ) {

    return;
  }

  KGoogle::Auth *auth = KGoogle::Auth::instance();

  try {
    auth->revoke( account );
    updateButtons();
  } catch ( KGoogle::Exception::BaseException &e ) {
    KMessageBox::error( this, e.what() );
  }

  reloadAccounts();
}

void SettingsDialog::updateButtons()
{
  bool enableRemoveButton = (m_ui->accountsCombo->count()>0);
  m_ui->removeAccountBtn->setEnabled(enableRemoveButton);
}
