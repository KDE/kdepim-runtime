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
#include "dialog.h"
#include "transport.h"
#include "ispdb/ispdb.h"

#include <kpimutils/email.h>

#include <KDebug>

PersonalDataPage::PersonalDataPage(Dialog* parent) :
  Page( parent ), mSetupManager( parent->setupManager() )
{
  ui.setupUi( this );
  connect( ui.emailEdit, SIGNAL( textChanged(QString) ), SLOT( slotTextChanged() ) );
  connect( ui.nameEdit, SIGNAL( textChanged(QString) ), SLOT( slotTextChanged() ) );
}

void PersonalDataPage::slotTextChanged() 
{
  // Ignore the password field, as that can be empty when auth is based on ip-address.
  setValid( !ui.emailEdit->text().isEmpty() &&
            !ui.nameEdit->text().isEmpty()  && 
            KPIMUtils::isValidAddress( ui.emailEdit->text() ) == KPIMUtils::AddressOk);
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
    if ( mIspdb->smtpServers().count() > 0 ) {
      server s = mIspdb->smtpServers().first(); // should be ok.
      kDebug() << "Configuring transport for" << s.hostname;

      QObject* object = mSetupManager->createTransport("smtp");
      Transport* t = qobject_cast<Transport*>( object ); 
      t->setName( mIspdb->name( Ispdb::Long ) );
      t->setHost( s.hostname );
      t->setPort( s.port );
      t->setUsername( s.username );
      t->setPassword( ui.passwordEdit->text() );
      switch (s.authentication) {
        case Ispdb::Cleartext: t->setEncryption( "clear" ); break;
        case Ispdb::Secure: t->setEncryption( "plain" ); break;
        case Ispdb::NTLM: t->setEncryption( "ntlm" ); break;
        case Ispdb::GSSAPI: t->setEncryption( "gssapi" ); break;
        case Ispdb::ClientIP: break;
        case Ispdb::None: break;
        default: break;
      }
      switch (s.socketType) {
        case Ispdb::Plain: t->setAuthenticationType( "none" );break;
        case Ispdb::SSL: t->setAuthenticationType( "ssl" );break;
        case Ispdb::StartTLS: t->setAuthenticationType( "tls" );break;
        default: break;
      }
    } else
      kDebug() << "No transport to be created....";

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

