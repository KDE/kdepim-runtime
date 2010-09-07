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
#include "resource.h"
#include "ispdb/ispdb.h"

#include <kpimutils/emailvalidator.h>
#include <kpimutils/email.h>

#include <mailtransport/transport.h>

#include <KDebug>

#include <QScrollArea>

PersonalDataPage::PersonalDataPage(Dialog* parent) :
  Page( parent ), mSetupManager( parent->setupManager() )
{
#ifdef KDEPIM_MOBILE_UI
  // for mobile ui we put the page into a scroll area in case it's too big
  QVBoxLayout *layout = new QVBoxLayout;
  layout->setMargin( 0 );
  setLayout( layout );

  QScrollArea *scrollArea = new QScrollArea( this );
  scrollArea->setFrameShape( QFrame::NoFrame );
  scrollArea->setWidgetResizable( true );
  layout->addWidget( scrollArea );

  QWidget *pageParent = new QWidget;
  scrollArea->setWidget( pageParent );
#else
  QWidget *pageParent = this;
#endif

  ui.setupUi( pageParent );

  KPIMUtils::EmailValidator* emailValidator = new KPIMUtils::EmailValidator( this );
  ui.emailEdit->setValidator( emailValidator );

  // KEmailSettings defaults
  ui.nameEdit->setText( mSetupManager->name() );
  ui.emailEdit->setText( mSetupManager->email() );
  slotTextChanged();
  connect( ui.emailEdit, SIGNAL( textChanged(QString) ), SLOT( slotTextChanged() ) );
  connect( ui.nameEdit, SIGNAL( textChanged(QString) ), SLOT( slotTextChanged() ) );
}

void PersonalDataPage::setHideOptionInternetSearch( bool hide )
{
  ui.checkOnlineGroupBox->setChecked( !hide );
  ui.checkOnlineGroupBox->setVisible( !hide );
}

void PersonalDataPage::slotTextChanged() 
{
  // Ignore the password field, as that can be empty when auth is based on ip-address.
  setValid( !ui.emailEdit->text().isEmpty() &&
            !ui.nameEdit->text().isEmpty()  && 
            KPIMUtils::isValidSimpleAddress( ui.emailEdit->text() ) );
}

void PersonalDataPage::leavePageNext()
{
  mSetupManager->setPersonalDataAvailable( true );
  mSetupManager->setName( ui.nameEdit->text() );
  mSetupManager->setPassword( ui.passwordEdit->text() );
  mSetupManager->setEmail( ui.emailEdit->text().trimmed() );

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
        case Ispdb::Plain: t->setAuthenticationType( "plain" ); break;
        case Ispdb::CramMD5: t->setAuthenticationType( "cram-md5" ); break;
        case Ispdb::NTLM: t->setAuthenticationType( "ntlm" ); break;
        case Ispdb::GSSAPI: t->setAuthenticationType( "gssapi" ); break;
        case Ispdb::ClientIP: break;
        case Ispdb::NoAuth: break;
        default: break;
      }
      switch (s.socketType) {
        case Ispdb::Plain: t->setEncryption( "none" );break;
        case Ispdb::SSL: t->setEncryption( "ssl" );break;
        case Ispdb::StartTLS: t->setEncryption( "tls" );break;
        default: break;
      }
    } else
      kDebug() << "No transport to be created....";


    // configure incoming


    if ( mIspdb->imapServers().count() > 0 ) {
      server s = mIspdb->imapServers().first(); // should be ok.
      kDebug() << "Configuring imap for" << s.hostname;

      QObject* object = mSetupManager->createResource("akonadi_imap_resource");
      Resource* t = qobject_cast<Resource*>( object ); 
      t->setName( mIspdb->name( Ispdb::Long ) );
      t->setOption( "ImapServer", s.hostname );
      t->setOption( "ImapPort", s.port );
      t->setOption( "UserName", s.username );
      t->setOption( "Password", ui.passwordEdit->text() );
      switch (s.authentication) {
        case Ispdb::Plain: t->setOption("Authentication", MailTransport::Transport::EnumAuthenticationType::CLEAR ); break;
        case Ispdb::CramMD5: t->setOption("Authentication", MailTransport::Transport::EnumAuthenticationType::CRAM_MD5 ); break;
        case Ispdb::NTLM: t->setOption("Authentication", MailTransport::Transport::EnumAuthenticationType::NTLM ); break;
        case Ispdb::GSSAPI: t->setOption("Authentication", MailTransport::Transport::EnumAuthenticationType::GSSAPI ); break;
        case Ispdb::ClientIP: break;
        case Ispdb::NoAuth: break;
        default: break;
      }
      switch (s.socketType) {
        case Ispdb::None: t->setOption( "Safety", "None" );break;
        case Ispdb::SSL: t->setOption( "Safety", "SSL" );break;
        case Ispdb::StartTLS: t->setOption( "Safety", "STARTTLS" );break;
        default: break;
      }
    } else if ( mIspdb->pop3Servers().count() > 0 ) {
      server s = mIspdb->pop3Servers().first(); // should be ok.
      kDebug() << "No Imap to be created, configuring pop3 for" << s.hostname;

      QObject* object = mSetupManager->createResource("akonadi_pop3_resource");
      Resource* t = qobject_cast<Resource*>( object ); 
      t->setName( mIspdb->name( Ispdb::Long ) );
      t->setOption( "Host", s.hostname );
      t->setOption( "Port", s.port );
      t->setOption( "Login", s.username );
      t->setOption( "Password", ui.passwordEdit->text() );
      switch (s.authentication) {
        case Ispdb::Plain: t->setOption("AuthenticationMethod", MailTransport::Transport::EnumAuthenticationType::PLAIN ); break;
        case Ispdb::CramMD5: t->setOption("AuthenticationMethod", MailTransport::Transport::EnumAuthenticationType::CRAM_MD5 ); break;
        case Ispdb::NTLM: t->setOption("AuthenticationMethod", MailTransport::Transport::EnumAuthenticationType::NTLM ); break;
        case Ispdb::GSSAPI: t->setOption("AuthenticationMethod", MailTransport::Transport::EnumAuthenticationType::GSSAPI ); break;
        case Ispdb::ClientIP:
        case Ispdb::NoAuth:
        default: t->setOption("AuthenticationMethod", MailTransport::Transport::EnumAuthenticationType::CLEAR ); break;
      }
      switch (s.socketType) {
        case Ispdb::SSL: t->setOption( "UseSSL", 1 );break;
        case Ispdb::StartTLS: t->setOption( "UseTLS", 1 );break;
        case Ispdb::None:
        default: t->setOption( "UseTLS", 1 ); break;
      }
    } else
      kDebug() << "No Imap or pop3 to be created....";

    emit leavePageNextOk();  // go to the next page
    mSetupManager->execute();
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

