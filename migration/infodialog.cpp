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

#include <KDebug>
#include <KGlobal>

#include <QListWidget>

InfoDialog::InfoDialog( bool closeWhenDone ) :
    mMigratorCount( 0 ),
    mError( false ),
    mChange( false ),
    mCloseWhenDone( closeWhenDone )
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

void InfoDialog::message(KMigratorBase::MessageType type, const QString & msg)
{
  QListWidgetItem *item = new QListWidgetItem( msg, mList );
  switch ( type ) {
    case KMigratorBase::Success:
      item->setIcon( KIcon( "dialog-ok-apply" ) );
      mChange = true;
      kDebug() << msg;
      break;
    case KMigratorBase::Skip:
      item->setIcon( KIcon( "dialog-ok" ) );
      kDebug() << msg;
      break;
    case KMigratorBase::Info:
      item->setIcon( KIcon( "dialog-information" ) );
      kDebug() << msg;
      break;
    case KMigratorBase::Warning:
      item->setIcon( KIcon( "dialog-warning" ) );
      kDebug() << msg;
      break;
    case KMigratorBase::Error:
      item->setIcon( KIcon( "dialog-error" ) );
      mError = true;
      kError() << msg;
      break;
    default:
      kError() << "WTF?";
  }
}

void InfoDialog::migratorAdded()
{
  ++mMigratorCount;
}

void InfoDialog::migratorDone()
{
  --mMigratorCount;
  if ( mMigratorCount == 0 ) {
    enableButton( Close, true );
    if ( mCloseWhenDone && !hasError() && !hasChange() )
      emit closeClicked();
  }
}

#include "infodialog.moc"
