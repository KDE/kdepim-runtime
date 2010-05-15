/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>
    Copyright (c) 2010 Tom Albers <toma@kde.org>

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

#include "personaldatapage.h"
#include "global.h"
#include "ispdb/ispdb.h"

#include <KDebug>

PersonalDataPage::PersonalDataPage(KAssistantDialog* parent) :
  Page( parent )
{
  ui.setupUi( this );
  setValid( true );
}

void PersonalDataPage::leavePageNext()
{
  if ( ui.checkOnlineGroupBox->isChecked() ) {
    // since the user can go back and forth, explicitly disable the man page
    emit manualWanted( false );

    kDebug() << "Searching on internet";
    mIspdb = new Ispdb();
    mIspdb->setEmail( ui.emailEdit->text() );
    mIspdb->start();

    connect( mIspdb, SIGNAL( finished( bool ) ),
             SLOT( ispdbSearchFinished( bool ) ) );
  } else {
    emit manualWanted( true );     // enable the manual page
    emit leavePageNextOk();  // go to the next page
  }
}

void PersonalDataPage::ispdbSearchFinished( bool ok )
{
  kDebug() << ok;

  if ( ok ) {
    // configure the stuff 
    emit leavePageNextOk();  // go to the next page
  } else {
    emit manualWanted( true );     // enable the manual page
    emit leavePageNextOk();
  }
}

void PersonalDataPage::leavePageNextRequested()
{
  // override base class with doing nothing...
}


#include "personaldatapage.moc"

