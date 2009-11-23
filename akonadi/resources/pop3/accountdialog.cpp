/*
 *   Copyright (C) 2000 Espen Sand, espen@kde.org
 *   Copyright 2009 Thomas McGuire <mcguire@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

// Local includes
#include "settings.h"
#include "settingsadaptor.h"
#include "accountdialog.h"
#include "pop3resource.h"

// KDEPIMLIBS includes
#include <Akonadi/Collection>
#include <Akonadi/CollectionFetchJob>
#include <akonadi/kmime/specialmailcollections.h>
#include <akonadi/kmime/specialmailcollectionsrequestjob.h>
#include <Mailtransport/ServerTest>

// KDELIBS includes
#include <KMessageBox>
#include <KProtocolInfo>
#include <KWindowSystem>
#include <kwallet.h>

using namespace MailTransport;
using namespace Akonadi;
using namespace KWallet;

namespace {

class BusyCursorHelper : public QObject
{
public:
  inline BusyCursorHelper( QObject *parent )
         : QObject( parent )
  {
    qApp->setOverrideCursor( Qt::BusyCursor );
  }

  inline ~BusyCursorHelper()
  {
    qApp->restoreOverrideCursor();
  }
};

}


AccountDialog::AccountDialog( POP3Resource *parentResource, WId parentWindow )
  : KDialog(),
    mParentResource( parentResource ),
    mServerTest( 0 ),
    mValidator( this ),
    mWallet( 0 )
{
  KWindowSystem::setMainWindow( this, parentWindow );
  setButtons( Ok|Cancel );
  mValidator.setRegExp( QRegExp( "[A-Za-z0-9-_:.]*" ) );

  setupWidgets();
  loadSettings();
}

AccountDialog::~AccountDialog()
{
  delete mWallet;
  mWallet = 0;
  delete mServerTest;
  mServerTest = 0;
}

void AccountDialog::setupWidgets()
{
  QWidget *page = new QWidget( this );
  setupUi( page );
  setMainWidget( page );

  connect( passwordEdit, SIGNAL( textEdited( const QString& ) ),
           this, SLOT( slotPopPasswordChanged( const QString& ) ) );

  // only letters, digits, '-', '.', ':' (IPv6) and '_' (for Windows
  // compatibility) are allowed
  hostEdit->setValidator( &mValidator );

  connect( leaveOnServerCheck, SIGNAL( clicked() ),
           this, SLOT( slotLeaveOnServerClicked() ) );
  connect( leaveOnServerDaysCheck, SIGNAL( toggled(bool) ),
           this, SLOT( slotEnableLeaveOnServerDays(bool)) );
  connect( leaveOnServerDaysSpin, SIGNAL(valueChanged(int)),
           SLOT(slotLeaveOnServerDaysChanged(int)));
  connect( leaveOnServerCountCheck, SIGNAL( toggled(bool) ),
           this, SLOT( slotEnableLeaveOnServerCount(bool)) );
  connect( leaveOnServerCountSpin, SIGNAL(valueChanged(int)),
           SLOT(slotLeaveOnServerCountChanged(int)));
  connect( leaveOnServerSizeCheck, SIGNAL( toggled(bool) ),
           this, SLOT( slotEnableLeaveOnServerSize(bool)) );

  connect(filterOnServerSizeSpin, SIGNAL(valueChanged(int)),
          SLOT(slotFilterOnServerSizeChanged(int)));
  connect( filterOnServerCheck, SIGNAL(toggled(bool)),
           filterOnServerSizeSpin, SLOT(setEnabled(bool)) );
  connect( filterOnServerCheck, SIGNAL( clicked() ),
           this, SLOT( slotFilterOnServerClicked() ) );

  connect( checkCapabilities, SIGNAL(clicked()),
           SLOT(slotCheckPopCapabilities()) );
  encryptionButtonGroup = new QButtonGroup();
  encryptionButtonGroup->addButton( encryptionNone,
                                    Transport::EnumEncryption::None );
  encryptionButtonGroup->addButton( encryptionSSL,
                                    Transport::EnumEncryption::SSL );
  encryptionButtonGroup->addButton( encryptionTLS,
                                    Transport::EnumEncryption::TLS );

  connect( encryptionButtonGroup, SIGNAL(buttonClicked(int)),
           SLOT(slotPopEncryptionChanged(int)) );
  connect( intervalCheck, SIGNAL(toggled(bool)),
           this, SLOT(slotEnablePopInterval(bool)) );

  // FIXME: Does this still work for non-ioslave POP3?
  if ( KProtocolInfo::capabilities("pop3").contains("SASL") == 0 )
  {
    authNTLM->hide();
    authGSSAPI->hide();
  }
  authButtonGroup = new QButtonGroup();
  authButtonGroup->addButton( authUser );
  authButtonGroup->addButton( authLogin );
  authButtonGroup->addButton( authPlain );
  authButtonGroup->addButton( authCRAM_MD5 );
  authButtonGroup->addButton( authDigestMd5 );
  authButtonGroup->addButton( authNTLM );
  authButtonGroup->addButton( authGSSAPI );
  authButtonGroup->addButton( authAPOP );

  folderRequester->setMimeTypeFilter(
      QStringList() << QLatin1String( "message/rfc822" ) );
  folderRequester->setFrameStyle( QFrame::NoFrame );

  connect( usePipeliningCheck, SIGNAL(clicked()),
           SLOT(slotPipeliningClicked()) );

  connect(KGlobalSettings::self(),SIGNAL(kdisplayFontChanged()),
          SLOT(slotFontChanged()));

  // FIXME: Hide widgets which are not supported yet
  includeInCheck->hide();
  filterOnServerCheck->hide();
  filterOnServerSizeSpin->hide();
}

void AccountDialog::loadSettings()
{
  if ( mParentResource->name() == mParentResource->identifier() )
    mParentResource->setName( i18n( "POP3 Account") );

  nameEdit->setText( mParentResource->name() );
  nameEdit->setFocus();
  loginEdit->setText( Settings::self()->login() );
  hostEdit->setText( Settings::self()->host() );
  portEdit->setValue( Settings::self()->port() );
  precommand->setText( Settings::self()->precommand() );
  usePipeliningCheck->setChecked( Settings::self()->pipelining() );
  storePasswordCheck->setChecked( Settings::self()->storePassword() );
  mInitallyStorePassword = Settings::self()->storePassword();
  leaveOnServerCheck->setChecked( Settings::self()->leaveOnServer() );
  leaveOnServerDaysCheck->setEnabled( Settings::self()->leaveOnServer() );
  leaveOnServerDaysCheck->setChecked( Settings::self()->leaveOnServerDays() >= 1 );
  leaveOnServerDaysSpin->setValue( Settings::self()->leaveOnServerDays() >= 1 ?
                                           Settings::self()->leaveOnServerDays() : 7 );
  leaveOnServerCountCheck->setEnabled( Settings::self()->leaveOnServer() );
  leaveOnServerCountCheck->setChecked( Settings::self()->leaveOnServerCount() >= 1 );
  leaveOnServerCountSpin->setValue( Settings::self()->leaveOnServerCount() >= 1 ?
                                            Settings::self()->leaveOnServerCount() : 100 );
  leaveOnServerSizeCheck->setEnabled( Settings::self()->leaveOnServer() );
  leaveOnServerSizeCheck->setChecked( Settings::self()->leaveOnServerSize() >= 1 );
  leaveOnServerSizeSpin->setValue( Settings::self()->leaveOnServerSize() >= 1 ?
                                           Settings::self()->leaveOnServerSize() : 10 );
  filterOnServerCheck->setChecked( Settings::self()->filterOnServer() );
  filterOnServerSizeSpin->setValue( Settings::self()->filterCheckSize() );
  intervalCheck->setChecked( Settings::self()->intervalCheckEnabled() );
  intervalSpin->setValue( Settings::self()->intervalCheckInterval() );
  intervalSpin->setEnabled( Settings::self()->intervalCheckEnabled() );
  if (Settings::self()->useSSL())
    encryptionSSL->setChecked( true );
  else if (Settings::self()->useTLS())
    encryptionTLS->setChecked( true );
  else encryptionNone->setChecked( true );
  if (Settings::self()->authenticationMethod() == "LOGIN")
    authLogin->setChecked( true );
  else if (Settings::self()->authenticationMethod() == "PLAIN")
    authPlain->setChecked( true );
  else if (Settings::self()->authenticationMethod() == "CRAM-MD5")
    authCRAM_MD5->setChecked( true );
  else if (Settings::self()->authenticationMethod() == "DIGEST-MD5")
    authDigestMd5->setChecked( true );
  else if (Settings::self()->authenticationMethod() == "NTLM")
    authNTLM->setChecked( true );
  else if (Settings::self()->authenticationMethod() == "GSSAPI")
    authGSSAPI->setChecked( true );
  else if (Settings::self()->authenticationMethod() == "APOP")
    authAPOP->setChecked( true );
  else authUser->setChecked( true );

  slotEnableLeaveOnServerDays( leaveOnServerDaysCheck->isEnabled() ?
                               Settings::self()->leaveOnServerDays() >= 1 : 0);
  slotEnableLeaveOnServerCount( leaveOnServerCountCheck->isEnabled() ?
                                Settings::self()->leaveOnServerCount() >= 1 : 0);
  slotEnableLeaveOnServerSize( leaveOnServerSizeCheck->isEnabled() ?
                               Settings::self()->leaveOnServerSize() >= 1 : 0);

  // We need to fetch the collection, as the CollectionRequester needs the name
  // of it to work correctly
  Collection targetCollection( Settings::self()->targetCollection() );
  if ( targetCollection.isValid() ) {
    CollectionFetchJob *fetchJob = new CollectionFetchJob( targetCollection,
                                                           CollectionFetchJob::Base,
                                                           this );
    connect( fetchJob, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
             this, SLOT(targetCollectionReceived(Akonadi::Collection::List)) );
  }
  else {
    // FIXME: This is a bit duplicated from POP3Resource...

    // No target collection set in the config? Try requesting a default inbox
    SpecialMailCollectionsRequestJob *requestJob = new SpecialMailCollectionsRequestJob( this );
    requestJob->requestDefaultCollection( SpecialMailCollections::Inbox );
    requestJob->start();
    connect ( requestJob, SIGNAL(result(KJob*)),
              this, SLOT(localFolderRequestJobFinished(KJob*)) );
  }
  folderRequester->setEnabled( false );

  if ( Settings::storePassword() ) {
    mWallet = Wallet::openWallet( Wallet::NetworkWallet(), winId(),
                                  Wallet::Asynchronous );
    connect( mWallet, SIGNAL(walletOpened(bool)),
             this, SLOT(walletOpenedForLoading(bool)) );
    passwordEdit->setEnabled( false );
  }
}

void AccountDialog::walletOpenedForLoading( bool success )
{
  if ( success ) {
    if ( mWallet && mWallet->isOpen() && mWallet->hasFolder( "pop3" ) ) {
      QString password;
      mWallet->setFolder( "pop3" );
      mWallet->readPassword( mParentResource->identifier(), password );
      passwordEdit->setText( password );
      mInitalPassword = password;
    }
    else {
      kWarning() << "Wallet not open or doesn't have pop3 folder.";
    }
  }
  else {
    storePasswordCheck->setChecked( false );
    kWarning() << "Failed to open wallet for loading the password.";
  }

  passwordEdit->setEnabled( true );
  delete mWallet;
  mWallet = 0;
}

void AccountDialog::walletOpenedForSaving( bool success )
{
  if ( success ) {
    if ( mWallet && mWallet->isOpen() ) {

      // Remove the password from the wallet if the user doesn't want to store it
      if ( !Settings::self()->storePassword() && mWallet->hasFolder( "pop3" ) ) {
        mWallet->setFolder( "pop3" );
        mWallet->removeEntry( mParentResource->identifier() );
      }

      // Store the password in the wallet if the user wants that
      else if ( Settings::self()->storePassword() ) {
        if ( !mWallet->hasFolder( "pop3" ) ) {
          mWallet->createFolder( "pop3" );
        }
        mWallet->setFolder( "pop3" );
        mWallet->writePassword( mParentResource->identifier(), passwordEdit->text() );
      }
    }
    else {
      kWarning() << "Wallet not open.";
    }
  }
  else {
    // Should we alert the user here?
    kWarning() << "Failed to open wallet for loading the password.";
  }

  delete mWallet;
  mWallet = 0;
  accept();
}

void AccountDialog::slotLeaveOnServerClicked()
{
  const bool state = leaveOnServerCheck->isChecked();
  leaveOnServerDaysCheck->setEnabled( state );
  leaveOnServerCountCheck->setEnabled( state );
  leaveOnServerSizeCheck->setEnabled( state );
  if ( state ) {
    if ( leaveOnServerDaysCheck->isChecked() ) {
      slotEnableLeaveOnServerDays( state );
    }
    if ( leaveOnServerCountCheck->isChecked() ) {
      slotEnableLeaveOnServerCount( state );
    }
    if ( leaveOnServerSizeCheck->isChecked() ) {
      slotEnableLeaveOnServerSize( state );
    }
  } else {
    slotEnableLeaveOnServerDays( state );
    slotEnableLeaveOnServerCount( state );
    slotEnableLeaveOnServerSize( state );
  }
  if ( mServerTest && !mServerTest->capabilities().contains( ServerTest::UIDL ) &&
       leaveOnServerCheck->isChecked() ) {
    KMessageBox::information( topLevelWidget(),
                              i18n("The server does not seem to support unique "
                                   "message numbers, but this is a "
                                   "requirement for leaving messages on the "
                                   "server.\n"
                                   "Since some servers do not correctly "
                                   "announce their capabilities you still "
                                   "have the possibility to turn leaving "
                                   "fetched messages on the server on.") );
  }
}

void AccountDialog::slotFilterOnServerClicked()
{
  if ( mServerTest && !mServerTest->capabilities().contains( ServerTest::Top ) &&
       filterOnServerCheck->isChecked() ) {
    KMessageBox::information( topLevelWidget(),
                              i18n("The server does not seem to support "
                                   "fetching message headers, but this is a "
                                   "requirement for filtering messages on the "
                                   "server.\n"
                                   "Since some servers do not correctly "
                                   "announce their capabilities you still "
                                   "have the possibility to turn filtering "
                                   "messages on the server on.") );
  }
}

void AccountDialog::slotPipeliningClicked()
{
  if (usePipeliningCheck->isChecked())
    KMessageBox::information( topLevelWidget(),
      i18n("Please note that this feature can cause some POP3 servers "
      "that do not support pipelining to send corrupted mail;\n"
      "this is configurable, though, because some servers support pipelining "
      "but do not announce their capabilities. To check whether your POP3 server "
      "announces pipelining support use the \"Check What the Server "
      "Supports\" button at the bottom of the dialog;\n"
      "if your server does not announce it, but you want more speed, then "
      "you should do some testing first by sending yourself a batch "
      "of mail and downloading it."), QString(),
      "pipelining");
}


void AccountDialog::slotPopEncryptionChanged( int id )
{
  // adjust port
  if ( id == Transport::EnumEncryption::SSL || portEdit->value() == 995 )
    portEdit->setValue( ( id == Transport::EnumEncryption::SSL ) ? 995 : 110 );

  enablePopFeatures();
  const QAbstractButton *old = authButtonGroup->checkedButton();
  if ( old && !old->isEnabled() )
    checkHighest( authButtonGroup );
}

void AccountDialog::slotPopPasswordChanged( const QString& text )
{
  if ( text.isEmpty() )
    storePasswordCheck->setCheckState( Qt::Unchecked );
  else
    storePasswordCheck->setCheckState( Qt::Checked );
}

void AccountDialog::slotCheckPopCapabilities()
{
  if ( hostEdit->text().isEmpty() )
  {
     KMessageBox::sorry( this, i18n( "Please specify a server and port on "
                                     "the General tab first." ) );
     return;
  }
  delete mServerTest;
  mServerTest = new ServerTest( this );
  BusyCursorHelper *busyCursorHelper = new BusyCursorHelper( mServerTest );
  mServerTest->setProgressBar( checkCapabilitiesProgress );
  checkCapabilitiesStack->setCurrentIndex( 1 );
  Transport::EnumEncryption::type encryptionType;
  if ( encryptionSSL->isChecked() )
    encryptionType = Transport::EnumEncryption::SSL;
  else
    encryptionType = Transport::EnumEncryption::None;
  mServerTest->setPort( encryptionType, portEdit->value() );
  mServerTest->setServer( hostEdit->text() );
  mServerTest->setProtocol( "pop" );
  connect( mServerTest, SIGNAL( finished(QList<int>) ),
           this, SLOT( slotPopCapabilities(QList<int>) ) );
  connect( mServerTest, SIGNAL( finished(QList<int>) ),
           busyCursorHelper, SLOT( deleteLater() ) );
  mServerTest->start();
  mServerTestFailed = false;
}

void AccountDialog::slotPopCapabilities( QList<int> encryptionTypes )
{
  checkCapabilitiesStack->setCurrentIndex( 0 );

  // If the servertest did not find any useable authentication modes, assume the
  // connection failed and don't disable any of the radioboxes.
  if ( encryptionTypes.isEmpty() ) {
    mServerTestFailed = true;
    return;
  }

  encryptionNone->setEnabled( encryptionTypes.contains( Transport::EnumEncryption::None ) );
  encryptionSSL->setEnabled( encryptionTypes.contains( Transport::EnumEncryption::SSL ) );
  encryptionTLS->setEnabled(  encryptionTypes.contains( Transport::EnumEncryption::TLS )  );
  checkHighest( encryptionButtonGroup );
}


void AccountDialog::enablePopFeatures()
{
  if ( !mServerTest || mServerTestFailed )
    return;

  QList<int> supportedAuths;
  if ( encryptionButtonGroup->checkedId() == Transport::EnumEncryption::None )
    supportedAuths = mServerTest->normalProtocols();
  if ( encryptionButtonGroup->checkedId() == Transport::EnumEncryption::SSL )
    supportedAuths = mServerTest->secureProtocols();
  if ( encryptionButtonGroup->checkedId() == Transport::EnumEncryption::TLS )
    supportedAuths = mServerTest->tlsProtocols();

  authPlain->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::PLAIN ) );
  authLogin->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::LOGIN ) );
  authCRAM_MD5->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::CRAM_MD5 ) );
  authDigestMd5->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::DIGEST_MD5 ) );
  authNTLM->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::NTLM ) );
  authGSSAPI->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::GSSAPI ) );
  authAPOP->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::APOP ) );

  if ( mServerTest && !mServerTest->capabilities().contains( ServerTest::Pipelining ) &&
       usePipeliningCheck->isChecked() ) {
    usePipeliningCheck->setChecked( false );
    KMessageBox::information( topLevelWidget(),
                              i18n("The server does not seem to support "
                                   "pipelining; therefore, this option has "
                                   "been disabled.\n"
                                   "Since some servers do not correctly "
                                   "announce their capabilities you still "
                                   "have the possibility to turn pipelining "
                                   "on. But please note that this feature can "
                                   "cause some POP servers that do not "
                                   "support pipelining to send corrupt "
                                   "messages. So before using this feature "
                                   "with important mail you should first "
                                   "test it by sending yourself a larger "
                                   "number of test messages which you all "
                                   "download in one go from the POP "
                                   "server.") );
  }

  if ( mServerTest && !mServerTest->capabilities().contains( ServerTest::UIDL ) &&
       leaveOnServerCheck->isChecked() ) {
    leaveOnServerCheck->setChecked( false );
    KMessageBox::information( topLevelWidget(),
                              i18n("The server does not seem to support unique "
                                   "message numbers, but this is a "
                                   "requirement for leaving messages on the "
                                   "server; therefore, this option has been "
                                   "disabled.\n"
                                   "Since some servers do not correctly "
                                   "announce their capabilities you still "
                                   "have the possibility to turn leaving "
                                   "fetched messages on the server on.") );
  }

  if ( mServerTest && !mServerTest->capabilities().contains( ServerTest::Top ) &&
       filterOnServerCheck->isChecked() ) {
    filterOnServerCheck->setChecked( false );
    KMessageBox::information( topLevelWidget(),
                              i18n("The server does not seem to support "
                                   "fetching message headers, but this is a "
                                   "requirement for filtering messages on the "
                                   "server; therefore, this option has been "
                                   "disabled.\n"
                                   "Since some servers do not correctly "
                                   "announce their capabilities you still "
                                   "have the possibility to turn filtering "
                                   "messages on the server on.") );
  }
}

void AccountDialog::slotLeaveOnServerDaysChanged ( int value )
{
  leaveOnServerDaysSpin->setSuffix( i18np(" day", " days", value) );
}


void AccountDialog::slotLeaveOnServerCountChanged ( int value )
{
  leaveOnServerCountSpin->setSuffix( i18np(" message", " messages", value) );
}


void AccountDialog::slotFilterOnServerSizeChanged ( int value )
{
  filterOnServerSizeSpin->setSuffix( i18np(" byte", " bytes", value) );
}

void AccountDialog::checkHighest( QButtonGroup *btnGroup )
{
  QListIterator<QAbstractButton*> it( btnGroup->buttons() );
  it.toBack();
  while ( it.hasPrevious() ) {
    QAbstractButton *btn = it.previous();
    if ( btn && btn->isEnabled() ) {
      btn->animateClick();
      return;
    }
  }
}

void AccountDialog::slotButtonClicked( int button )
{
  switch( button ) {
  case Ok:
    saveSettings();

    // Don't call accept() yet, saveSettings() triggers an asnychronous wallet operation,
    // which will call accept() when it is finished
    break;
  case Cancel:
    reject();
    return;
  }
}

void AccountDialog::saveSettings()
{
  mParentResource->setName( nameEdit->text() );

  Settings::self()->setIntervalCheckEnabled( intervalCheck->checkState() == Qt::Checked );
  Settings::self()->setIntervalCheckInterval( intervalSpin->value() );
  Settings::self()->setHost( hostEdit->text().trimmed() );
  Settings::self()->setPort( portEdit->value() );
  Settings::self()->setLogin( loginEdit->text().trimmed() );
  Settings::self()->setStorePassword( storePasswordCheck->isChecked() );
  Settings::self()->setPrecommand( precommand->text() );
  Settings::self()->setUseSSL( encryptionSSL->isChecked() );
  Settings::self()->setUseTLS( encryptionTLS->isChecked() );
  if (authUser->isChecked())
    Settings::self()->setAuthenticationMethod("USER");
  else if (authLogin->isChecked())
    Settings::self()->setAuthenticationMethod("LOGIN");
  else if (authPlain->isChecked())
    Settings::self()->setAuthenticationMethod("PLAIN");
  else if (authCRAM_MD5->isChecked())
    Settings::self()->setAuthenticationMethod("CRAM-MD5");
  else if (authDigestMd5->isChecked())
    Settings::self()->setAuthenticationMethod("DIGEST-MD5");
  else if (authNTLM->isChecked())
    Settings::self()->setAuthenticationMethod("NTLM");
  else if (authGSSAPI->isChecked())
    Settings::self()->setAuthenticationMethod("GSSAPI");
  else if (authAPOP->isChecked())
    Settings::self()->setAuthenticationMethod("APOP");
  else
    Settings::self()->setAuthenticationMethod("AUTO");

  Settings::self()->setPipelining( usePipeliningCheck->isChecked() );
  Settings::self()->setLeaveOnServer( leaveOnServerCheck->isChecked() );
  Settings::self()->setLeaveOnServerDays( leaveOnServerCheck->isChecked() ?
                                          ( leaveOnServerDaysCheck->isChecked() ?
                                            leaveOnServerDaysSpin->value() : -1 ) : 0);
  Settings::self()->setLeaveOnServerCount( leaveOnServerCheck->isChecked() ?
                                           ( leaveOnServerCountCheck->isChecked() ?
                                             leaveOnServerCountSpin->value() : -1 ) : 0 );
  Settings::self()->setLeaveOnServerSize( leaveOnServerCheck->isChecked() ?
                                          ( leaveOnServerSizeCheck->isChecked() ?
                                            leaveOnServerSizeSpin->value() : -1 ) : 0 );
  Settings::self()->setFilterOnServer( filterOnServerCheck->isChecked() );
  Settings::self()->setFilterCheckSize (filterOnServerSizeSpin->value() );
  Settings::self()->setTargetCollection( folderRequester->collection().id() );
  Settings::self()->writeConfig();

  // Now, either save the password or delete it from the wallet. For both, we need
  // to open it.
  const bool userChangedPassword = mInitalPassword != passwordEdit->text();
  const bool userWantsToDeletePassword =
      !Settings::self()->storePassword() && mInitallyStorePassword;

  if ( ( Settings::self()->storePassword() && userChangedPassword ) ||
       userWantsToDeletePassword ) {
    mWallet = Wallet::openWallet( Wallet::NetworkWallet(), winId(),
                                  Wallet::Asynchronous );
    connect( mWallet, SIGNAL(walletOpened(bool)),
             this, SLOT(walletOpenedForSaving(bool)) );
  }
  else {
    // Neither save nor delete the password, we're done!
    accept();
  }
}

void AccountDialog::slotEnableLeaveOnServerDays( bool state )
{
  if ( state && !leaveOnServerDaysCheck->isEnabled() )
    return;
  leaveOnServerDaysSpin->setEnabled( state );
}

void AccountDialog::slotEnableLeaveOnServerCount( bool state )
{
  if ( state && !leaveOnServerCountCheck->isEnabled() )
    return;
  leaveOnServerCountSpin->setEnabled( state );
  return;
}

void AccountDialog::slotEnableLeaveOnServerSize( bool state )
{
  if ( state && !leaveOnServerSizeCheck->isEnabled() )
    return;
  leaveOnServerSizeSpin->setEnabled( state );
  return;
}

void AccountDialog::slotEnablePopInterval( bool state )
{
  intervalSpin->setEnabled( state );
  intervalLabel->setEnabled( state );
}

void AccountDialog::slotFontChanged( void )
{
  QFont titleFont( titleLabel->font() );
  titleFont.setBold( true );
  titleLabel->setFont(titleFont);
}

void AccountDialog::targetCollectionReceived( Akonadi::Collection::List collections )
{
  folderRequester->setCollection( collections.first() );
  folderRequester->setEnabled( true );
}

void AccountDialog::localFolderRequestJobFinished( KJob *job )
{
  if ( !job->error() ) {
    Collection targetCollection = SpecialMailCollections::self()->defaultCollection( SpecialMailCollections::Inbox );
    Q_ASSERT( targetCollection.isValid() );
    folderRequester->setCollection( targetCollection );
  }
  folderRequester->setEnabled( true );
}

#include "accountdialog.moc"
