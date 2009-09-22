/* This file is part of the KDE project
   Copyright (C) 2009 Kevin Ottens <ervin@kde.org>
   Copyright (C) 2006-2008 Omat Holding B.V. <info@omat.nl>
   Copyright (C) 2006 Frode M. Døving <frode@lnix.net>

   Original copied from showfoto:
    Copyright 2005 by Gilles Caulier <caulier.gilles@free.fr>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "setupserver.h"
#include "settings.h"

#include <QButtonGroup>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QRadioButton>

#include <mailtransport/transport.h>
#include <mailtransport/servertest.h>

#include <kemailsettings.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kmessagebox.h>
#include <kuser.h>
#include <solid/networking.h>

#include "imapaccount.h"
#include "subscriptiondialog.h"

#include "ui_setupserverview.h"

SetupServer::SetupServer( WId parent )
  : KDialog(), m_ui(new Ui::SetupServerView), m_serverTest(0),
    m_subscriptionsChanged(false), m_shouldClearCache(false)
{
  Settings::self()->setWinId( parent );
  m_ui->setupUi( mainWidget() );

  m_ui->safeImapGroup->setId( m_ui->noRadio, 1 );
  m_ui->safeImapGroup->setId( m_ui->sslRadio, 2 );
  m_ui->safeImapGroup->setId( m_ui->tlsRadio, 3 );

  connect( m_ui->noRadio, SIGNAL( toggled(bool) ),
           this, SLOT( slotSafetyChanged() ) );
  connect( m_ui->sslRadio, SIGNAL( toggled(bool) ),
           this, SLOT( slotSafetyChanged() ) );
  connect( m_ui->tlsRadio, SIGNAL( toggled(bool) ),
           this, SLOT( slotSafetyChanged() ) );

  m_ui->authImapGroup->setId( m_ui->clearRadio, 1 );
  m_ui->authImapGroup->setId( m_ui->loginRadio, 2 );
  m_ui->authImapGroup->setId( m_ui->plainRadio, 3 );
  m_ui->authImapGroup->setId( m_ui->cramMd5Radio, 4 );
  m_ui->authImapGroup->setId( m_ui->digestMd5Radio, 5 );
  m_ui->authImapGroup->setId( m_ui->ntlmRadio, 6 );
  m_ui->authImapGroup->setId( m_ui->gssapiRadio, 7 );
  m_ui->authImapGroup->setId( m_ui->anonymousRadio, 8 );

  m_ui->testInfo->hide();
  m_ui->testProgress->hide();
  connect( m_ui->testButton, SIGNAL( pressed() ), SLOT( slotTest() ) );

  connect( m_ui->imapServer, SIGNAL( textChanged( const QString & ) ),
           SLOT( slotTestChanged() ) );
  connect( m_ui->imapServer, SIGNAL( textChanged( const QString & ) ),
           SLOT( slotComplete() ) );
  connect( m_ui->userName, SIGNAL( textChanged( const QString & ) ),
           SLOT( slotComplete() ) );

  connect( m_ui->subscriptionButton, SIGNAL( pressed() ), SLOT( slotManageSubscriptions() ) );

  readSettings();
  slotTestChanged();
  slotComplete();
  connect( Solid::Networking::notifier(),
           SIGNAL( statusChanged( Solid::Networking::Status ) ),
           SLOT( slotTestChanged() ) );
  connect( this, SIGNAL( applyClicked() ),
           SLOT( applySettings() ) );
  connect( this, SIGNAL( okClicked() ),
           SLOT( applySettings() ) );
}

SetupServer::~SetupServer()
{
}

bool SetupServer::shouldClearCache() const
{
  return m_shouldClearCache;
}

void SetupServer::applySettings()
{
  m_shouldClearCache = ( Settings::self()->imapServer() != m_ui->imapServer->text() )
                    || ( Settings::self()->userName() != m_ui->userName->text() );

  Settings::self()->setImapServer( m_ui->imapServer->text() );
  Settings::self()->setUserName( m_ui->userName->text() );
  Settings::self()->setSafety( m_ui->safeImapGroup->checkedId() );
  Settings::self()->setAuthentication( m_ui->authImapGroup->checkedId() );
  Settings::self()->setPassword( m_ui->password->text() );
  Settings::self()->setSubscriptionEnabled( m_ui->subscriptionEnabled->isChecked() );
  Settings::self()->writeConfig();
  kDebug() << "wrote" << m_ui->imapServer->text() << m_ui->userName->text() << m_ui->safeImapGroup->checkedId();
}

void SetupServer::readSettings()
{
  KUser* currentUser = new KUser();
  KEMailSettings esetting;

  m_ui->imapServer->setText(
    !Settings::self()->imapServer().isEmpty() ? Settings::self()->imapServer() :
    esetting.getSetting( KEMailSettings::InServer ) );

  m_ui->userName->setText(
    !Settings::self()->userName().isEmpty() ? Settings::self()->userName() :
    currentUser->loginName() );

  int i = Settings::self()->safety();
  if ( i == 0 )
    i = 1; // it crashes when 0, shouldn't happen, but you never know.
  m_ui->safeImapGroup->button( i )->setChecked( true );

  i = Settings::self()->authentication();
  if ( i == 0 )
    i = 1; // it crashes when 0, shouldn't happen, but you never know.
  m_ui->authImapGroup->button( i )->setChecked( true );

  if ( !Settings::self()->passwordPossible() ) {
    m_ui->password->setEnabled( false );
    KMessageBox::information( 0, i18n( "Could not access KWallet. "
                                       "If you want to store the password permanently then you have to "
                                       "activate it. If you do not "
                                       "want to use KWallet, check the box below, but note that you will be "
                                       "prompted for your password when needed." ),
                              i18n( "Do not use KWallet" ), "warning_kwallet_disabled" );
  } else {
    m_ui->password->insert( Settings::self()->password() );
  }

  m_ui->subscriptionEnabled->setChecked( Settings::self()->subscriptionEnabled() );

  delete currentUser;
}

void SetupServer::slotTest()
{
  kDebug() << m_ui->imapServer->text();

  m_ui->testButton->setEnabled( false );
  m_ui->safeImap->setEnabled( false );
  m_ui->authImap->setEnabled( false );

  m_ui->testInfo->clear();
  m_ui->testInfo->hide();

  delete m_serverTest;
  m_serverTest = new MailTransport::ServerTest( this );

  QString server = m_ui->imapServer->text().section( ':', 0, 0 );
  int port = m_ui->imapServer->text().section( ':', 1, 1 ).toInt();

  m_serverTest->setServer( server );

  if ( port>0 ) {
    m_serverTest->setPort( MailTransport::Transport::EnumEncryption::None, port );
    m_serverTest->setPort( MailTransport::Transport::EnumEncryption::SSL, port );
  }

  m_serverTest->setProtocol( "imap" );
  m_serverTest->setProgressBar( m_ui->testProgress );
  connect( m_serverTest, SIGNAL( finished( QList<int> ) ),
           SLOT( slotFinished( QList<int> ) ) );
  m_serverTest->start();
}

void SetupServer::slotFinished( QList<int> testResult )
{
  kDebug() << testResult;

  using namespace MailTransport;

  m_ui->testInfo->show();

  m_ui->sslRadio->setEnabled( testResult.contains( Transport::EnumEncryption::SSL ) );
  m_ui->tlsRadio->setEnabled( testResult.contains( Transport::EnumEncryption::TLS ) );
  m_ui->noRadio->setEnabled( testResult.contains( Transport::EnumEncryption::None ) );

  QString text;
  if ( testResult.contains( Transport::EnumEncryption::TLS ) ) {
    m_ui->tlsRadio->setChecked( true );
    text = i18n( "<qt><b>TLS is supported and recommended.</b></qt>" );
  } else if ( testResult.contains( Transport::EnumEncryption::SSL ) ) {
    m_ui->sslRadio->setChecked( true );
    text = i18n( "<qt><b>SSL is supported and recommended.</b></qt>" );
  } else if ( testResult.contains( Transport::EnumEncryption::None ) ) {
    m_ui->noRadio->setChecked( true );
    text = i18n( "<qt><b>No security is supported. It is not "
                 "recommended to connect to this server.</b></qt>" );
  } else {
    text = i18n( "<qt><b>It is not possible to use this server.</b></qt>" );
  }
  m_ui->testInfo->setText( text );

  m_ui->testButton->setEnabled( true );
  m_ui->safeImap->setEnabled( true );
  m_ui->authImap->setEnabled( true );
}

void SetupServer::slotTestChanged()
{
  delete m_serverTest;
  m_serverTest = 0;
  slotSafetyChanged();

  // do not use imapConnectionPossible, as the data is not yet saved.
  m_ui->testButton->setEnabled( true /* TODO Global::connectionPossible() ||
                                        m_ui->imapServer->text() == "localhost"*/ );
}

void SetupServer::slotComplete()
{
  bool ok =  !m_ui->imapServer->text().isEmpty() && !m_ui->userName->text().isEmpty();
  button( KDialog::Ok )->setEnabled( ok );
}

void SetupServer::slotSafetyChanged()
{
  if ( m_serverTest==0 ) {
    m_ui->noRadio->setEnabled( true );
    m_ui->sslRadio->setEnabled( true );
    m_ui->tlsRadio->setEnabled( true );

    m_ui->clearRadio->setEnabled( true );
    m_ui->loginRadio->setEnabled( true );
    m_ui->plainRadio->setEnabled( true );
    m_ui->cramMd5Radio->setEnabled( true );
    m_ui->digestMd5Radio->setEnabled( true );
    m_ui->ntlmRadio->setEnabled( true );
    m_ui->gssapiRadio->setEnabled( true );
    m_ui->anonymousRadio->setEnabled( true );
    return;
  }

  QList<int> protocols;

  switch ( m_ui->safeImapGroup->checkedId() ) {
  case 1:
    protocols = m_serverTest->normalProtocols();
    break;
  case 2:
    protocols = m_serverTest->secureProtocols();
    break;
  case 3:
    protocols = m_serverTest->tlsProtocols();
    break;
  default:
    kFatal() << "Shouldn't happen";
  }

  using namespace MailTransport;

  m_ui->clearRadio->setEnabled( true );
  m_ui->loginRadio->setEnabled( protocols.contains( Transport::EnumAuthenticationType::LOGIN ) );
  m_ui->plainRadio->setEnabled( protocols.contains( Transport::EnumAuthenticationType::PLAIN ) );
  m_ui->cramMd5Radio->setEnabled( protocols.contains( Transport::EnumAuthenticationType::CRAM_MD5 ) );
  m_ui->digestMd5Radio->setEnabled( protocols.contains( Transport::EnumAuthenticationType::DIGEST_MD5 ) );
  m_ui->ntlmRadio->setEnabled( protocols.contains( Transport::EnumAuthenticationType::NTLM ) );
  m_ui->gssapiRadio->setEnabled( protocols.contains( Transport::EnumAuthenticationType::GSSAPI ) );
  m_ui->anonymousRadio->setEnabled( protocols.contains( Transport::EnumAuthenticationType::ANONYMOUS ) );

  if ( !m_ui->authImapGroup->checkedButton()->isEnabled() ) {
    m_ui->clearRadio->setChecked( true );
  }
}

void SetupServer::slotManageSubscriptions()
{
  ImapAccount account;
  account.setServer( m_ui->imapServer->text() );
  account.setUserName( m_ui->userName->text() );
  account.setSubscriptionEnabled( m_ui->subscriptionEnabled->isChecked() );

  switch ( m_ui->safeImapGroup->checkedId() ) {
  case 1:
    account.setEncryptionMode( KIMAP::LoginJob::Unencrypted );
    break;
  case 2:
    account.setEncryptionMode( KIMAP::LoginJob::AnySslVersion );
    break;
  case 3:
    account.setEncryptionMode( KIMAP::LoginJob::TlsV1 );
    break;
  default:
    kFatal("Shouldn't happen...");
  }

  switch ( m_ui->authImapGroup->checkedId() ) {
  case 1:
    account.setAuthenticationMode( KIMAP::LoginJob::ClearText );
    break;
  case 2:
    account.setAuthenticationMode( KIMAP::LoginJob::Login );
    break;
  case 3:
    account.setAuthenticationMode( KIMAP::LoginJob::Plain );
    break;
  case 4:
    account.setAuthenticationMode( KIMAP::LoginJob::CramMD5 );
    break;
  case 5:
    account.setAuthenticationMode( KIMAP::LoginJob::DigestMD5 );
    break;
  case 6:
    account.setAuthenticationMode( KIMAP::LoginJob::NTLM );
    break;
  case 7:
    account.setAuthenticationMode( KIMAP::LoginJob::GSSAPI );
    break;
  case 8:
    account.setAuthenticationMode( KIMAP::LoginJob::Anonymous );
    break;
  default:
    kFatal("Shouldn't happen...");
  }

  m_subscriptionsChanged = false;
  SubscriptionDialog *subscriptions = new SubscriptionDialog( this, i18n("Serverside Subscription..."), &account, m_subscriptionsChanged );

  connect( &account, SIGNAL( success() ),
           subscriptions, SLOT( slotConnectionSuccess() ) );

  account.connect( m_ui->password->text() );
  subscriptions->exec();

  m_ui->subscriptionEnabled->setChecked( account.isSubscriptionEnabled() );
}

#include "setupserver.moc"
