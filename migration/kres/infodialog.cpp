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

#include "infodialog.h"

#include <KGlobal>

#include <QListWidget>

InfoDialog::InfoDialog() :
    mMigratorCount( 0 )
{
  KGlobal::ref();
  setButtons( Close );
  enableButton( Close, false );
  mList = new QListWidget( this );
  mList->setMinimumWidth( 640 );
  setMainWidget( mList );

  connect( this, SIGNAL(closeClicked()), SLOT(deleteLater()) );
}

InfoDialog::~InfoDialog()
{
  KGlobal::deref();
}

void InfoDialog::successMessage(const QString & msg)
{
  new QListWidgetItem( KIcon( "dialog-ok" ), msg, mList );
}

void InfoDialog::infoMessage(const QString & msg)
{
  new QListWidgetItem( KIcon( "dialog-information" ), msg, mList );
}

void InfoDialog::errorMessage(const QString & msg)
{
  new QListWidgetItem( KIcon( "dialog-errror" ), msg, mList );
}

void InfoDialog::migratorAdded()
{
  ++mMigratorCount;
}

void InfoDialog::migratorDone()
{
  --mMigratorCount;
  if ( mMigratorCount == 0 )
    enableButton( Close, true );
}
