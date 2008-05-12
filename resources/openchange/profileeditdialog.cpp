/*
    Copyright (c) 2007 Brad Hards <bradh@frogmouth.net>

    Significant amounts of this code adapted from the openchange client utility,
    which is Copyright (C) Julien Kerihuel 2007 <j.kerihuel@openchange.org>.

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "profileeditdialog.h"

#include <QHostInfo>
#include <QLabel>
#include <QVBoxLayout>

extern "C" {
#include <libmapi/libmapi.h>
#include <talloc.h>
}

ProfileEditDialog::ProfileEditDialog( QWidget *parent,
                                      const QString &profileName,
                                      const QString &userName,
                                      const QString &password,
                                      const QString &serverAddress,
                                      const QString &workstation,
                                      const QString &domain )
  : QDialog( parent )
{
  setWindowTitle( "Add / Edit Profile" );

  QVBoxLayout *mainLayout = new QVBoxLayout;

  QLabel *nameLabel = new QLabel( "Profile name" );
  mainLayout->addWidget( nameLabel );
  m_profileNameEdit = new QLineEdit();
  m_profileNameEdit->setText( profileName );
  connect( m_profileNameEdit, SIGNAL( textEdited(QString) ),
           this, SLOT( checkIfComplete() ) );
  mainLayout->addWidget( m_profileNameEdit );

  QLabel *usernameLabel = new QLabel( "User name" );
  mainLayout->addWidget( usernameLabel );
  m_usernameEdit = new QLineEdit();
  m_usernameEdit->setText( userName );
  connect( m_usernameEdit, SIGNAL( textEdited(QString) ),
           this, SLOT( checkIfComplete() ) );
  mainLayout->addWidget( m_usernameEdit );

  QLabel *passwordLabel = new QLabel( "Password" );
  mainLayout->addWidget( passwordLabel );
  m_passwordEdit = new QLineEdit();
  m_passwordEdit->setEchoMode( QLineEdit::Password );
  m_passwordEdit->setText( password );
  connect( m_passwordEdit, SIGNAL( textEdited(QString) ),
           this, SLOT( checkIfComplete() ) );
  mainLayout->addWidget( m_passwordEdit );

  QLabel *addressLabel = new QLabel( "Server name or address" );
  mainLayout->addWidget( addressLabel );
  m_addressEdit = new QLineEdit();
  m_addressEdit->setText( serverAddress );
  connect( m_addressEdit, SIGNAL( textEdited(QString) ),
           this, SLOT( checkIfComplete() ) );
  mainLayout->addWidget( m_addressEdit );

  QLabel *workstationLabel = new QLabel( "Local machine name or address" );
  mainLayout->addWidget( workstationLabel );
  m_workstationEdit = new QLineEdit();
  if ( workstation.isEmpty() )
    m_workstationEdit->setText( QHostInfo::localHostName() );
  else
    m_workstationEdit->setText( workstation );
  connect( m_workstationEdit, SIGNAL( textEdited(QString) ),
           this, SLOT( checkIfComplete() ) );
  mainLayout->addWidget( m_workstationEdit );

  QLabel *domainLabel = new QLabel( "Authentication domain" );
  mainLayout->addWidget( domainLabel );
  m_domainEdit = new QLineEdit();
  m_domainEdit->setText( domain );
  connect( m_domainEdit, SIGNAL( textEdited(QString) ),
           this, SLOT( checkIfComplete() ) );
  m_domainEdit->setToolTip( "The authentication domain (also known as realm) to use for this account. Ask your exchange server administrator if you are do not know about this." );
  mainLayout->addWidget( m_domainEdit );

  mainLayout->addStretch();

  QHBoxLayout *buttonLayout = new QHBoxLayout;

  m_okButton = new QPushButton( "OK" );
  connect( m_okButton, SIGNAL( clicked() ),
           this, SLOT( commitProfile() ) );
  buttonLayout->addWidget( m_okButton );

  QPushButton *cancelButton = new QPushButton( "Cancel" );
  connect( cancelButton, SIGNAL( clicked() ),
           this, SLOT( close() ) );
  buttonLayout->addWidget( cancelButton );

  mainLayout->addLayout( buttonLayout );

  checkIfComplete();

  setLayout( mainLayout );
}

void ProfileEditDialog::checkIfComplete()
{
  if ( m_profileNameEdit->text().isEmpty()
       || m_usernameEdit->text().isEmpty()
       || m_passwordEdit->text().isEmpty()
       || m_addressEdit->text().isEmpty()
       || m_workstationEdit->text().isEmpty()
       || m_domainEdit->text().isEmpty() )
    m_okButton->setDisabled(true);
  else
    m_okButton->setEnabled(true);
}

uint32_t ProfileEditDialog::callback(struct SRowSet *rowset, void *private_var)
{
  qDebug() << "ProfileEditDialog::callback() Found more than 1 match";

  // TODO: Handle this. Need to find a way to produce this.
  // Cancel user creation for now.
  return rowset->cRows;
}

void ProfileEditDialog::commitProfile()
{
  qDebug() << "committing profile: " << m_profileNameEdit->text();

  enum MAPISTATUS         retval;
  struct mapi_profile     testProfile;

  // if the profile already exists, we overwrite it....
  // maybe we should have a "do you really want to do this?", but that will be pretty annoying on edits.
  retval = OpenProfile( &testProfile, m_profileNameEdit->text().toUtf8().constData(), 0 );
  qDebug() << "openprofile result: " << retval;
  if ( GetLastError() != MAPI_E_NOT_FOUND ) {
    // then this one exists, and we should kill it
    if ((retval = DeleteProfile(m_profileNameEdit->text().toUtf8().constData()) ) != MAPI_E_SUCCESS) {
      mapi_errstr("DeleteProfile", GetLastError());
      return;
    }
  }

  retval = CreateProfile(m_profileNameEdit->text().toUtf8().constData(),
                         m_usernameEdit->text().toUtf8().constData(),
                         m_passwordEdit->text().toUtf8().constData(), 0);

  if (retval != MAPI_E_SUCCESS) {
    mapi_errstr("CreateProfile", GetLastError());
    return;
  }

  mapi_profile_add_string_attr(m_profileNameEdit->text().toUtf8().constData(),
                               "binding",
                               m_addressEdit->text().toUtf8().constData() );

  mapi_profile_add_string_attr(m_profileNameEdit->text().toUtf8().constData(),
                               "workstation",
                               m_workstationEdit->text().toUtf8().constData() );

  mapi_profile_add_string_attr(m_profileNameEdit->text().toUtf8().constData(),
                               "domain",
                               m_domainEdit->text().toUtf8().constData() );

  /* This is only convenient here and should be replaced at some point */
  mapi_profile_add_string_attr(m_profileNameEdit->text().toUtf8().constData(),
                               "codepage", "0x4e4");
  mapi_profile_add_string_attr(m_profileNameEdit->text().toUtf8().constData(),
                               "language", "0x40c");
  mapi_profile_add_string_attr(m_profileNameEdit->text().toUtf8().constData(),
                               "method", "0x409");


  // there needs to be some network magic in here.
  struct mapi_session     *session = NULL;
  retval = MapiLogonProvider(&session, m_profileNameEdit->text().toUtf8().constData(),
                             m_passwordEdit->text().toUtf8().constData(), PROVIDER_ID_NSPI);
  if (retval != MAPI_E_SUCCESS) {
    mapi_errstr("MapiLogonProvider", GetLastError());
    // TODO - we need to do something more creative here
  }


  retval = ProcessNetworkProfile(session, m_usernameEdit->text().toUtf8().constData(), (mapi_profile_callback_t) ProfileEditDialog::callback, 
                                 "Select a user id");

  if (retval != MAPI_E_SUCCESS && retval != 0x1) {
    mapi_errstr("ProcessNetworkProfile", GetLastError());
    printf("Deleting profile\n");
    if ((retval = DeleteProfile(m_profileNameEdit->text().toUtf8().constData())) != MAPI_E_SUCCESS) {
      mapi_errstr("DeleteProfile", GetLastError());
    }
    exit (1);
  }


  // TODO: add in KWallet support
  close(); // only if we added correctly.
}

