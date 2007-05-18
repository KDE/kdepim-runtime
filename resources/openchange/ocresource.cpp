/*
    Copyright (c) 2007 Brad Hards <bradh@frogmouth.net>

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

#include "ocresource.h"

#include <QtCore/QDebug>
#include <QtDBus/QDBusConnection>

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Akonadi;

ProfileDialog::ProfileDialog( QWidget *parent )
  : QDialog( parent )
{
  setWindowTitle( "Profile Configuration" );

  QVBoxLayout *mainLayout = new QVBoxLayout;

  QLabel *label = new QLabel( "OpenChange Profiles" );
  mainLayout->addWidget( label );


  QHBoxLayout *lowerLayout = new QHBoxLayout;

  QVBoxLayout *leftLayout = new QVBoxLayout;
  m_listOfProfiles = new QListWidget;
  leftLayout->addWidget( m_listOfProfiles );

  leftLayout->addStretch();


  QVBoxLayout *rightLayout = new QVBoxLayout;

  QPushButton *addButton = new QPushButton( "Add Profile" );
  connect( addButton, SIGNAL( clicked() ),
           this, SLOT( addNewProfile() ) );
  rightLayout->addWidget( addButton );

  QPushButton *editButton = new QPushButton( "Edit Profile" );
  connect( editButton, SIGNAL( clicked() ),
           this, SLOT( editExistingProfile() ) );
  rightLayout->addWidget( editButton );

  QPushButton *setAsDefaultButton = new QPushButton( "Make default" );
  connect( setAsDefaultButton, SIGNAL( clicked() ),
           this, SLOT( setAsDefaultProfile() ) );
  rightLayout->addWidget( setAsDefaultButton );

  QPushButton *removeButton = new QPushButton( "Remove Profile" );
  connect( removeButton, SIGNAL( clicked() ),
           this, SLOT( deleteSelectedProfile() ) );
  rightLayout->addWidget( removeButton );

  QPushButton *closeButton = new QPushButton( "Close" );
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
  if ((retval = GetProfileTable(&proftable, 0)) != MAPI_E_SUCCESS) {
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
}

void ProfileDialog::deleteSelectedProfile()
{
  qDebug() << "selected profile: " << m_listOfProfiles->currentItem()->text();

  QListWidgetItem *selectedEntry = m_listOfProfiles->currentItem();
  QString selectedProfileName( selectedEntry->text() );
  // the profile name might have " [default]" on the end - check for that
  if ( selectedProfileName.endsWith ( " [default]" ) )
  {
    int endOfName = selectedProfileName.lastIndexOf( " [default]" );
    QString realProfileName = selectedProfileName.mid( 0, endOfName );
    enum MAPISTATUS retval;
    if ((retval = DeleteProfile(realProfileName.toLatin1().data(), 0)) != MAPI_E_SUCCESS) {
      mapi_errstr("DeleteProfile", GetLastError());
      exit (1);
    }
    qDebug() << realProfileName << "deleted (default)";
  } else {
    enum MAPISTATUS retval;
    if ((retval = DeleteProfile(selectedProfileName.toLatin1().data(), 0)) != MAPI_E_SUCCESS) {
      mapi_errstr("DeleteProfile", GetLastError());
      exit (1);
    }
    qDebug() << selectedProfileName << "deleted";
  }
  m_listOfProfiles->clear();
  fillProfileList();
}

void ProfileDialog::addNewProfile()
{
  qDebug() << "Adding profiles not yet implemented";
}


void ProfileDialog::editExistingProfile()
{
  qDebug() << "Profile editing not yet implemented";
}

void ProfileDialog::setAsDefaultProfile()
{
  qDebug() << "selected profile: " << m_listOfProfiles->currentItem()->text();

  QListWidgetItem *selectedEntry = m_listOfProfiles->currentItem();
  QString selectedProfileName( selectedEntry->text() );

  if ( selectedProfileName.endsWith ( " [default]" ) )
    return; // since this is already the default profile

  if ( SetDefaultProfile(selectedProfileName.toLatin1().data(), 0) != MAPI_E_SUCCESS) {
    mapi_errstr("SetDefaultProfile", GetLastError());
  }
  m_listOfProfiles->clear();
  fillProfileList();
}


OCResource::OCResource( const QString &id )
  :ResourceBase( id )
{
  m_mapiMemoryContext = talloc_init("openchangeclient");

  // Make this a setting somehow
  m_profileDatabase = QString( "/home/bradh/.openchange/profiles.ldb" );

  int retval = MAPIInitialize( m_profileDatabase.toAscii().data() );
  if (retval != MAPI_E_SUCCESS) {
    mapi_errstr("MAPIInitialize", GetLastError());
    exit (1);
  }

  mapi_object_init(&m_mapiStore);
}

OCResource::~ OCResource()
{
  mapi_object_release(&m_mapiStore);

  MAPIUninitialize();

  talloc_free(m_mapiMemoryContext);
}

bool OCResource::requestItemDelivery( const Akonadi::DataReference &ref, int, const QDBusMessage &msg )
{
  return true;
}


void OCResource::aboutToQuit()
{
}

void OCResource::configure()
{
  ProfileDialog configDialog;
  configDialog.exec();
}

void OCResource::itemAdded( const Akonadi::Item & item, const Akonadi::Collection& )
{
}

void OCResource::itemChanged( const Akonadi::Item&, const QStringList& )
{
}

void OCResource::itemRemoved(const Akonadi::DataReference & ref)
{
}

void OCResource::retrieveCollections()
{
}

void OCResource::synchronizeCollection(const Akonadi::Collection & col)
{
}

#include "ocresource.moc"
