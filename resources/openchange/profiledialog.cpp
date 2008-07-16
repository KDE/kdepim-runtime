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

#include "profiledialog.h"

#include <klocale.h>

#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>

extern "C" {
#include <libmapi/libmapi.h>
#include <talloc.h>
}

#include "profileeditdialog.h"

ProfileDialog::ProfileDialog( OCResource *resource, QWidget *parent )
  : QDialog( parent ), m_resource( resource )
{
  setWindowTitle( i18n( "Profile Configuration" ) );

  QVBoxLayout *mainLayout = new QVBoxLayout;

  QLabel *label = new QLabel( i18n( "OpenChange Profiles" ) );
  mainLayout->addWidget( label );


  QHBoxLayout *lowerLayout = new QHBoxLayout;

  QVBoxLayout *leftLayout = new QVBoxLayout;
  m_listOfProfiles = new QListWidget;
  leftLayout->addWidget( m_listOfProfiles );

  leftLayout->addStretch();


  QVBoxLayout *rightLayout = new QVBoxLayout;

  QPushButton *addButton = new QPushButton( i18n( "Add Profile" ) );
  connect( addButton, SIGNAL( clicked() ),
           this, SLOT( addNewProfile() ) );
  rightLayout->addWidget( addButton );

  m_editButton = new QPushButton( i18n( "Edit Profile" ) );
  connect( m_editButton, SIGNAL( clicked() ),
           this, SLOT( editExistingProfile() ) );
  rightLayout->addWidget( m_editButton );

  m_setAsDefaultButton = new QPushButton( i18n( "Make default" ) );
  connect( m_setAsDefaultButton, SIGNAL( clicked() ),
           this, SLOT( setAsDefaultProfile() ) );
  rightLayout->addWidget( m_setAsDefaultButton );

  m_removeButton = new QPushButton( i18n( "Remove Profile" ) );
  connect( m_removeButton, SIGNAL( clicked() ),
           this, SLOT( deleteSelectedProfile() ) );
  rightLayout->addWidget( m_removeButton );

  QPushButton *closeButton = new QPushButton( i18n( "Close" ) );
  connect( closeButton, SIGNAL( clicked() ),
           this, SLOT( close() ) );
  rightLayout->addWidget( closeButton );

  rightLayout->addStretch();


  lowerLayout->addLayout( leftLayout );
  lowerLayout->addLayout( rightLayout );
  mainLayout->addLayout( lowerLayout );
  setLayout( mainLayout );

  fillProfileList();
}

void ProfileDialog::fillProfileList()
{
  enum MAPISTATUS retval;
  struct SRowSet  proftable;
  uint32_t        count = 0;

  memset(&proftable, 0, sizeof (struct SRowSet));
  if ((retval = GetProfileTable(&proftable)) != MAPI_E_SUCCESS) {
    mapi_errstr("GetProfileTable", GetLastError());
    exit (1);
  }

  // qDebug() << "Profiles in the database:" << proftable.cRows;

  for (count = 0; count != proftable.cRows; count++) {
    const char      *name = NULL;
    uint32_t        dflt = 0;

    name = proftable.aRow[count].lpProps[0].value.lpszA;
    dflt = proftable.aRow[count].lpProps[1].value.l;

    if (dflt) {
      QString profileName( QString( name ) + QString( " [default]" ) );
      QListWidgetItem *item = new QListWidgetItem( profileName, m_listOfProfiles );
      m_listOfProfiles->setCurrentItem( item );
    } else {
      new QListWidgetItem( QString( name ), m_listOfProfiles );
    }

  }

  if ( count > 0 ) {
    m_editButton->setEnabled( true );
    m_setAsDefaultButton->setEnabled( true );
    m_removeButton->setEnabled( true );
  } else {
    m_editButton->setEnabled(false);
    m_setAsDefaultButton->setEnabled( false );
    m_removeButton->setEnabled( false );
  }
}

QString ProfileDialog::selectedProfileName()
{
  qDebug() << "selected profile: " << m_listOfProfiles->currentItem()->text();

  QListWidgetItem *selectedEntry = m_listOfProfiles->currentItem();
  QString selectedProfileName( selectedEntry->text() );
  // the profile name might have " [default]" on the end - check for that
  if ( selectedProfileName.endsWith ( " [default]" ) )
  {
    int endOfName = selectedProfileName.lastIndexOf( " [default]" );
    return selectedProfileName.mid( 0, endOfName );
  } else {
    return selectedProfileName;
  }
}

void ProfileDialog::deleteSelectedProfile()
{
  enum MAPISTATUS retval;
  QString profileName = selectedProfileName();
  if ((retval = DeleteProfile(profileName.toUtf8().constData()) ) != MAPI_E_SUCCESS) {
    mapi_errstr("DeleteProfile", GetLastError());
    exit (1);
  }
  qDebug() << profileName << "deleted";

  m_listOfProfiles->clear();
  fillProfileList();
}

void ProfileDialog::addNewProfile()
{
  ProfileEditDialog *addDialog = new ProfileEditDialog( this );
  addDialog->exec();
  delete addDialog;

  m_listOfProfiles->clear();
  fillProfileList();
}


void ProfileDialog::editExistingProfile()
{
  qDebug() << "Profile editing not yet implemented";

  enum MAPISTATUS retval;
  struct mapi_profile profile;

  QString profileName = selectedProfileName();

  if ((retval = OpenProfile(&profile, profileName.toUtf8().constData(), 0)) != MAPI_E_SUCCESS) {
    mapi_errstr("OpenProfile", GetLastError());
    exit (1);
  }

  ProfileEditDialog *editDialog = new ProfileEditDialog( this,
                                                         QString( profile.profname ),
                                                         QString( profile.username ),
                                                         QString( profile.password ),
                                                         QString( profile.server ),
                                                         QString( profile.workstation ),
                                                         QString( profile.domain ) );

  editDialog->exec();
  delete editDialog;

  m_listOfProfiles->clear();
  fillProfileList();
}

void ProfileDialog::setAsDefaultProfile()
{
  qDebug() << "selected profile: " << m_listOfProfiles->currentItem()->text();

  QListWidgetItem *selectedEntry = m_listOfProfiles->currentItem();
  QString selectedProfileName( selectedEntry->text() );

  if ( selectedProfileName.endsWith ( " [default]" ) )
    return; // since this is already the default profile

  if ( SetDefaultProfile(selectedProfileName.toUtf8().constData()) != MAPI_E_SUCCESS) {
    mapi_errstr("SetDefaultProfile", GetLastError());
  }
  m_listOfProfiles->clear();
  fillProfileList();
}


