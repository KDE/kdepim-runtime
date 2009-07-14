/*
 *   kmail: KDE mail client
 *   Copyright (C) 2000 Espen Sand, espen@kde.org
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
#include "settings.h"
#include "settingsadaptor.h"

#include <QtDBus/QDBusConnection>

#include "accountdialog.h"
#include <kpimidentities/identitymanager.h>
#include <kpimidentities/identitycombo.h>
#include <kpimidentities/identity.h>
#include <mailtransport/servertest.h>
using namespace MailTransport;

#include <KComboBox>
#include <KGlobalSettings>
#include <KFileDialog>
#include <KLocale>
#include <KDebug>
#include <KMessageBox>
#include <KNumInput>
#include <KSeparator>
#include <KProtocolInfo>
#include <KIconLoader>
#include <KMenu>

#include <QButtonGroup>
#include <QCheckBox>
#include <QLayout>
#include <QRadioButton>
#include <QValidator>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QGroupBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>

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


AccountDialog::AccountDialog( QWidget *parent )
  : KDialog( parent ),
    mServerTest( 0 )
{
  //setCaption( caption );
  setButtons( Ok|Cancel|Help );
  mValidator = new QRegExpValidator( QRegExp( "[A-Za-z0-9-_:.]*" ), this );
  setHelp("receiving-mail");

  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ), Settings::self(), QDBusConnection::ExportAdaptors );

  makePopAccountPage();

  setupSettings();
  connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
}

AccountDialog::~AccountDialog()
{
  delete mServerTest;
  mServerTest = 0;
}

void AccountDialog::makePopAccountPage()
{
  QWidget *page = new QWidget( this );
  mPop.ui.setupUi( page );
  setMainWidget( page );

  connect( mPop.ui.passwordEdit, SIGNAL( textEdited( const QString& ) ),
           this, SLOT( slotPopPasswordChanged( const QString& ) ) );

  // only letters, digits, '-', '.', ':' (IPv6) and '_' (for Windows
  // compatibility) are allowed
  mPop.ui.hostEdit->setValidator( mValidator );

  connect( mPop.ui.leaveOnServerCheck, SIGNAL( clicked() ),
           this, SLOT( slotLeaveOnServerClicked() ) );
  connect( mPop.ui.leaveOnServerDaysCheck, SIGNAL( toggled(bool) ),
           this, SLOT( slotEnableLeaveOnServerDays(bool)) );
  connect( mPop.ui.leaveOnServerDaysSpin, SIGNAL(valueChanged(int)),
           SLOT(slotLeaveOnServerDaysChanged(int)));
  connect( mPop.ui.leaveOnServerCountCheck, SIGNAL( toggled(bool) ),
           this, SLOT( slotEnableLeaveOnServerCount(bool)) );
  connect( mPop.ui.leaveOnServerCountSpin, SIGNAL(valueChanged(int)),
           SLOT(slotLeaveOnServerCountChanged(int)));
  connect( mPop.ui.leaveOnServerSizeCheck, SIGNAL( toggled(bool) ),
           this, SLOT( slotEnableLeaveOnServerSize(bool)) );

  connect(mPop.ui.filterOnServerSizeSpin, SIGNAL(valueChanged(int)),
          SLOT(slotFilterOnServerSizeChanged(int)));
  connect( mPop.ui.filterOnServerCheck, SIGNAL(toggled(bool)),
           mPop.ui.filterOnServerSizeSpin, SLOT(setEnabled(bool)) );
  connect( mPop.ui.filterOnServerCheck, SIGNAL( clicked() ),
           this, SLOT( slotFilterOnServerClicked() ) );

  connect( mPop.ui.intervalCheck, SIGNAL(toggled(bool)),
           this, SLOT(slotEnablePopInterval(bool)) );
//  mPop.intervalSpin->setRange( GlobalSettings::self()->minimumCheckInterval(),
//                                  10000, 1 );

//  Page 2
  connect( mPop.ui.checkCapabilities, SIGNAL(clicked()),
           SLOT(slotCheckPopCapabilities()) );
  mPop.encryptionButtonGroup = new QButtonGroup();
  mPop.encryptionButtonGroup->addButton( mPop.ui.encryptionNone,
                                         Transport::EnumEncryption::None );
  mPop.encryptionButtonGroup->addButton( mPop.ui.encryptionSSL,
                                         Transport::EnumEncryption::SSL );
  mPop.encryptionButtonGroup->addButton( mPop.ui.encryptionTLS,
                                         Transport::EnumEncryption::TLS );

  connect( mPop.encryptionButtonGroup, SIGNAL(buttonClicked(int)),
           SLOT(slotPopEncryptionChanged(int)) );

  if ( KProtocolInfo::capabilities("pop3").contains("SASL") == 0 )
  {
    mPop.ui.authNTLM->hide();
    mPop.ui.authGSSAPI->hide();
  }
  mPop.authButtonGroup = new QButtonGroup();
  mPop.authButtonGroup->addButton( mPop.ui.authUser );
  mPop.authButtonGroup->addButton( mPop.ui.authLogin );
  mPop.authButtonGroup->addButton( mPop.ui.authPlain );
  mPop.authButtonGroup->addButton( mPop.ui.authCRAM_MD5 );
  mPop.authButtonGroup->addButton( mPop.ui.authDigestMd5 );
  mPop.authButtonGroup->addButton( mPop.ui.authNTLM );
  mPop.authButtonGroup->addButton( mPop.ui.authGSSAPI );
  mPop.authButtonGroup->addButton( mPop.ui.authAPOP );

  connect( mPop.ui.usePipeliningCheck, SIGNAL(clicked()),
           SLOT(slotPipeliningClicked()) );

  connect(KGlobalSettings::self(),SIGNAL(kdisplayFontChanged()),
          SLOT(slotFontChanged()));
}

void AccountDialog::setupSettings()
{
  KComboBox *folderCombo = 0;
  //bool intervalCheckingEnabled = ( mAccount->checkInterval() > 0 );
  //int interval = mAccount->checkInterval();
  //if ( !intervalCheckingEnabled ) // Default to 5 minutes when the user enables
  //  interval = 5;                 // interval checking for the first time

    //PopAccount &ap = *(PopAccount*)mAccount;
    //mPop.ui.nameEdit->setText( Settings::self()->name() );
    //mPop.ui.nameEdit->setFocus();
    mPop.ui.loginEdit->setText( Settings::self()->login() );
    mPop.ui.passwordEdit->setText( Settings::self()->password());
    mPop.ui.hostEdit->setText( Settings::self()->host() );
    mPop.ui.portEdit->setValue( Settings::self()->port() );
    //mPop.ui.usePipeliningCheck->setChecked( Settings::self()->usePipelining() );
    mPop.ui.storePasswordCheck->setChecked( Settings::self()->storePassword() );
    mPop.ui.leaveOnServerCheck->setChecked( Settings::self()->leaveOnServer() );
    mPop.ui.leaveOnServerDaysCheck->setEnabled( Settings::self()->leaveOnServer() );
    mPop.ui.leaveOnServerDaysCheck->setChecked( Settings::self()->leaveOnServerDays() >= 1 );
    mPop.ui.leaveOnServerDaysSpin->setValue( Settings::self()->leaveOnServerDays() >= 1 ?
                                            Settings::self()->leaveOnServerDays() : 7 );
    mPop.ui.leaveOnServerCountCheck->setEnabled( Settings::self()->leaveOnServer() );
    mPop.ui.leaveOnServerCountCheck->setChecked( Settings::self()->leaveOnServerCount() >= 1 );
    mPop.ui.leaveOnServerCountSpin->setValue( Settings::self()->leaveOnServerCount() >= 1 ?
                                            Settings::self()->leaveOnServerCount() : 100 );
    mPop.ui.leaveOnServerSizeCheck->setEnabled( Settings::self()->leaveOnServer() );
    mPop.ui.leaveOnServerSizeCheck->setChecked( Settings::self()->leaveOnServerSize() >= 1 );
    mPop.ui.leaveOnServerSizeSpin->setValue( Settings::self()->leaveOnServerSize() >= 1 ?
                                            Settings::self()->leaveOnServerSize() : 10 );
    mPop.ui.filterOnServerCheck->setChecked( Settings::self()->filterOnServer() );
    mPop.ui.filterOnServerSizeSpin->setValue( Settings::self()->filterCheckSize() );
    //mPop.ui.intervalCheck->setChecked( intervalCheckingEnabled );
    //mPop.ui.intervalSpin->setValue( interval );
    //mPop.ui.includeInCheck->setChecked( !mAccount->checkExclude() );
    //mPop.ui.precommand->setText( Settings::self()->precommand() );
    if (Settings::self()->useSSL())
      mPop.ui.encryptionSSL->setChecked( true );
    else if (Settings::self()->useTLS())
      mPop.ui.encryptionTLS->setChecked( true );
    else mPop.ui.encryptionNone->setChecked( true );
    if (Settings::self()->authenticationMethod() == "LOGIN")
      mPop.ui.authLogin->setChecked( true );
    else if (Settings::self()->authenticationMethod() == "PLAIN")
      mPop.ui.authPlain->setChecked( true );
    else if (Settings::self()->authenticationMethod() == "CRAM-MD5")
      mPop.ui.authCRAM_MD5->setChecked( true );
    else if (Settings::self()->authenticationMethod() == "DIGEST-MD5")
      mPop.ui.authDigestMd5->setChecked( true );
    else if (Settings::self()->authenticationMethod() == "NTLM")
      mPop.ui.authNTLM->setChecked( true );
    else if (Settings::self()->authenticationMethod() == "GSSAPI")
      mPop.ui.authGSSAPI->setChecked( true );
    else if (Settings::self()->authenticationMethod() == "APOP")
      mPop.ui.authAPOP->setChecked( true );
    else mPop.ui.authUser->setChecked( true );

    slotEnableLeaveOnServerDays( mPop.ui.leaveOnServerDaysCheck->isEnabled() ?
                                   Settings::self()->leaveOnServerDays() >= 1 : 0);
    slotEnableLeaveOnServerCount( mPop.ui.leaveOnServerCountCheck->isEnabled() ?
                                    Settings::self()->leaveOnServerCount() >= 1 : 0);
    slotEnableLeaveOnServerSize( mPop.ui.leaveOnServerSizeCheck->isEnabled() ?
                                    Settings::self()->leaveOnServerSize() >= 1 : 0);
    //slotEnablePopInterval( intervalCheckingEnabled );
    folderCombo = mPop.ui.folderCombo;

  if (!folderCombo) return;
/*
  KMFolderDir *fdir = (KMFolderDir*)&kmkernel->folderMgr()->dir();
  KMFolder *acctFolder = mAccount->folder();
  if( acctFolder == 0 )
  {
    acctFolder = (KMFolder*)fdir->first();
  }
  if( acctFolder == 0 )
  {
    folderCombo->addItem( i18nc("Placeholder for the case that there is no folder."
      , "<placeholder>none</placeholder>") );
  }
  else
  {
    int i = 0;
    int curIndex = -1;
    kmkernel->folderMgr()->createI18nFolderList(&mFolderNames, &mFolderList);
    while (i < mFolderNames.count())
    {
      //QList<QPointer<KMFolder> >::Iterator it = mFolderList.at(i);
      KMFolder *folder = mFolderList.at(i);
      if (folder->isSystemFolder())
      {
        mFolderList.removeAll(folder);
        mFolderNames.removeAt(i);
      } else {
        if (folder == acctFolder) curIndex = i;
        i++;
      }
    }
    mFolderNames.prepend(i18n("inbox"));
    mFolderList.prepend(kmkernel->inboxFolder());
    folderCombo->addItems(mFolderNames);
    folderCombo->setCurrentIndex(curIndex + 1);

    // -sanders hack for startup users. Must investigate this properly
    if (folderCombo->count() == 0)
      folderCombo->addItem( i18n("inbox") );
  }*/
}

void AccountDialog::slotLeaveOnServerClicked()
{
  bool state = mPop.ui.leaveOnServerCheck->isChecked();
  mPop.ui.leaveOnServerDaysCheck->setEnabled( state );
  mPop.ui.leaveOnServerCountCheck->setEnabled( state );
  mPop.ui.leaveOnServerSizeCheck->setEnabled( state );
  if ( state ) {
    if ( mPop.ui.leaveOnServerDaysCheck->isChecked() ) {
      slotEnableLeaveOnServerDays( state );
    }
    if ( mPop.ui.leaveOnServerCountCheck->isChecked() ) {
      slotEnableLeaveOnServerCount( state );
    }
    if ( mPop.ui.leaveOnServerSizeCheck->isChecked() ) {
      slotEnableLeaveOnServerSize( state );
    }
  } else {
    slotEnableLeaveOnServerDays( state );
    slotEnableLeaveOnServerCount( state );
    slotEnableLeaveOnServerSize( state );
  }
  if ( mServerTest && !mServerTest->capabilities().contains( ServerTest::UIDL ) &&
       mPop.ui.leaveOnServerCheck->isChecked() ) {
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
       mPop.ui.filterOnServerCheck->isChecked() ) {
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
  if (mPop.ui.usePipeliningCheck->isChecked())
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
  kDebug() << "ID:" << id;
  // adjust port
  if ( id == Transport::EnumEncryption::SSL || mPop.ui.portEdit->value() == 995 )
    mPop.ui.portEdit->setValue( ( id == Transport::EnumEncryption::SSL ) ? 995 : 110 );

  enablePopFeatures();
  const QAbstractButton *old = mPop.authButtonGroup->checkedButton();
  if ( old && !old->isEnabled() )
    checkHighest( mPop.authButtonGroup );
}

void AccountDialog::slotPopPasswordChanged( const QString& text )
{
  if ( text.isEmpty() )
    mPop.ui.storePasswordCheck->setCheckState( Qt::Unchecked );
  else
    mPop.ui.storePasswordCheck->setCheckState( Qt::Checked );
}

void AccountDialog::slotCheckPopCapabilities()
{
  if ( mPop.ui.hostEdit->text().isEmpty() )
  {
     KMessageBox::sorry( this, i18n( "Please specify a server and port on "
                                     "the General tab first." ) );
     return;
  }
  delete mServerTest;
  mServerTest = new ServerTest( this );
  BusyCursorHelper *busyCursorHelper = new BusyCursorHelper( mServerTest );
  mServerTest->setProgressBar( mPop.ui.checkCapabilitiesProgress );
  mPop.ui.checkCapabilitiesStack->setCurrentIndex( 1 );
  Transport::EnumEncryption::type encryptionType;
  if ( mPop.ui.encryptionSSL->isChecked() )
    encryptionType = Transport::EnumEncryption::SSL;
  else
    encryptionType = Transport::EnumEncryption::None;
  mServerTest->setPort( encryptionType, mPop.ui.portEdit->value() );
  mServerTest->setServer( mPop.ui.hostEdit->text() );
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
  mPop.ui.checkCapabilitiesStack->setCurrentIndex( 0 );

  // If the servertest did not find any useable authentication modes, assume the
  // connection failed and don't disable any of the radioboxes.
  if ( encryptionTypes.isEmpty() ) {
    mServerTestFailed = true;
    return;
  }

  mPop.ui.encryptionNone->setEnabled( encryptionTypes.contains( Transport::EnumEncryption::None ) );
  mPop.ui.encryptionSSL->setEnabled( encryptionTypes.contains( Transport::EnumEncryption::SSL ) );
  mPop.ui.encryptionTLS->setEnabled(  encryptionTypes.contains( Transport::EnumEncryption::TLS )  );
  checkHighest( mPop.encryptionButtonGroup );
}


void AccountDialog::enablePopFeatures()
{
  kDebug();
  if ( !mServerTest || mServerTestFailed )
    return;

  QList<int> supportedAuths;
  if ( mPop.encryptionButtonGroup->checkedId() == Transport::EnumEncryption::None )
    supportedAuths = mServerTest->normalProtocols();
  if ( mPop.encryptionButtonGroup->checkedId() == Transport::EnumEncryption::SSL )
    supportedAuths = mServerTest->secureProtocols();
  if ( mPop.encryptionButtonGroup->checkedId() == Transport::EnumEncryption::TLS )
    supportedAuths = mServerTest->tlsProtocols();

  mPop.ui.authPlain->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::PLAIN ) );
  mPop.ui.authLogin->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::LOGIN ) );
  mPop.ui.authCRAM_MD5->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::CRAM_MD5 ) );
  mPop.ui.authDigestMd5->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::DIGEST_MD5 ) );
  mPop.ui.authNTLM->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::NTLM ) );
  mPop.ui.authGSSAPI->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::GSSAPI ) );
  mPop.ui.authAPOP->setEnabled( supportedAuths.contains( Transport::EnumAuthenticationType::APOP ) );

  if ( mServerTest && !mServerTest->capabilities().contains( ServerTest::Pipelining ) &&
       mPop.ui.usePipeliningCheck->isChecked() ) {
    mPop.ui.usePipeliningCheck->setChecked( false );
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
       mPop.ui.leaveOnServerCheck->isChecked() ) {
    mPop.ui.leaveOnServerCheck->setChecked( false );
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
       mPop.ui.filterOnServerCheck->isChecked() ) {
    mPop.ui.filterOnServerCheck->setChecked( false );
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
  mPop.ui.leaveOnServerDaysSpin->setSuffix( i18np(" day", " days", value) );
}


void AccountDialog::slotLeaveOnServerCountChanged ( int value )
{
  mPop.ui.leaveOnServerCountSpin->setSuffix( i18np(" message", " messages", value) );
}


void AccountDialog::slotFilterOnServerSizeChanged ( int value )
{
  mPop.ui.filterOnServerSizeSpin->setSuffix( i18np(" byte", " bytes", value) );
}

void AccountDialog::slotIdentityCheckboxChanged()
{
   //assert( false );
}


void AccountDialog::checkHighest( QButtonGroup *btnGroup )
{
  kDebug() << btnGroup;
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


void AccountDialog::slotOk()
{
  saveSettings();
  accept();
}


void AccountDialog::saveSettings()
{
    //Settings::self()->setName( mPop.ui.nameEdit->text() );
    //Settings::self()->setCheckInterval( mPop.ui.intervalCheck->isChecked() ?
    //                            mPop.ui.intervalSpin->value() : 0 );
    //Settings::self()->setCheckExclude( !mPop.ui.includeInCheck->isChecked() );

    //Settings::self()->setFolder( mFolderList.at(mPop.ui.folderCombo->currentIndex()) );

    initAccountForConnect();
    Settings::self()->setPipelining( mPop.ui.usePipeliningCheck->isChecked() );
    Settings::self()->setLeaveOnServer( mPop.ui.leaveOnServerCheck->isChecked() );
    Settings::self()->setLeaveOnServerDays( mPop.ui.leaveOnServerCheck->isChecked() ?
                              ( mPop.ui.leaveOnServerDaysCheck->isChecked() ?
                                mPop.ui.leaveOnServerDaysSpin->value() : -1 ) : 0);
    Settings::self()->setLeaveOnServerCount( mPop.ui.leaveOnServerCheck->isChecked() ?
                               ( mPop.ui.leaveOnServerCountCheck->isChecked() ?
                                 mPop.ui.leaveOnServerCountSpin->value() : -1 ) : 0 );
    Settings::self()->setLeaveOnServerSize( mPop.ui.leaveOnServerCheck->isChecked() ?
                              ( mPop.ui.leaveOnServerSizeCheck->isChecked() ?
                                mPop.ui.leaveOnServerSizeSpin->value() : -1 ) : 0 );
    Settings::self()->setFilterOnServer( mPop.ui.filterOnServerCheck->isChecked() );
    Settings::self()->setFilterCheckSize (mPop.ui.filterOnServerSizeSpin->value() );
    //Settings::self()->setPrecommand( mPop.precommand->text() );

  
  //kmkernel->acctMgr()->writeConfig(true);
  // get the new account and register the new destination folder
  // this is the target folder for local or pop accounts and the root folder
  // of the account for (d)imap
/*  KMAccount* newAcct = kmkernel->acctMgr()->find(mAccount->id());
  if (newAcct)
  {
      newAcct->setFolder( mFolderList.at(mPop.folderCombo->currentIndex()), true );
  }*/
}

void AccountDialog::slotEnableLeaveOnServerDays( bool state )
{
  if ( state && !mPop.ui.leaveOnServerDaysCheck->isEnabled()) return;
  mPop.ui.leaveOnServerDaysSpin->setEnabled( state );
}

void AccountDialog::slotEnableLeaveOnServerCount( bool state )
{
  if ( state && !mPop.ui.leaveOnServerCountCheck->isEnabled()) return;
  mPop.ui.leaveOnServerCountSpin->setEnabled( state );
  return;
}

void AccountDialog::slotEnableLeaveOnServerSize( bool state )
{
  if ( state && !mPop.ui.leaveOnServerSizeCheck->isEnabled()) return;
  mPop.ui.leaveOnServerSizeSpin->setEnabled( state );
  return;
}

void AccountDialog::slotEnablePopInterval( bool state )
{
  mPop.ui.intervalSpin->setEnabled( state );
  mPop.ui.intervalLabel->setEnabled( state );
}

void AccountDialog::slotFontChanged( void )
{
    QFont titleFont( mPop.ui.titleLabel->font() );
    titleFont.setBold( true );
    mPop.ui.titleLabel->setFont(titleFont);
}

const QString AccountDialog::namespaceListToString( const QStringList& list )
{
  QStringList myList = list;
  for ( QStringList::Iterator it = myList.begin(); it != myList.end(); ++it ) {
    if ( (*it).isEmpty() ) {
      (*it) = '<' + i18nc("Empty namespace string.", "Empty") + '>';
    }
  }
  return myList.join(",");
}

void AccountDialog::initAccountForConnect()
{
    Settings::self()->setHost( mPop.ui.hostEdit->text().trimmed() );
    Settings::self()->setPort( mPop.ui.portEdit->value() );
    Settings::self()->setLogin( mPop.ui.loginEdit->text().trimmed() );
    Settings::self()->setStorePassword( mPop.ui.storePasswordCheck->isChecked() );
    Settings::self()->setPassword( mPop.ui.passwordEdit->text() );
    Settings::self()->setUseSSL( mPop.ui.encryptionSSL->isChecked() );
    Settings::self()->setUseTLS( mPop.ui.encryptionTLS->isChecked() );
    if (mPop.ui.authUser->isChecked())
      Settings::self()->setAuthenticationMethod("USER");
    else if (mPop.ui.authLogin->isChecked())
      Settings::self()->setAuthenticationMethod("LOGIN");
    else if (mPop.ui.authPlain->isChecked())
      Settings::self()->setAuthenticationMethod("PLAIN");
    else if (mPop.ui.authCRAM_MD5->isChecked())
      Settings::self()->setAuthenticationMethod("CRAM-MD5");
    else if (mPop.ui.authDigestMd5->isChecked())
      Settings::self()->setAuthenticationMethod("DIGEST-MD5");
    else if (mPop.ui.authNTLM->isChecked())
      Settings::self()->setAuthenticationMethod("NTLM");
    else if (mPop.ui.authGSSAPI->isChecked())
      Settings::self()->setAuthenticationMethod("GSSAPI");
    else if (mPop.ui.authAPOP->isChecked())
      Settings::self()->setAuthenticationMethod("APOP");
    else Settings::self()->setAuthenticationMethod("AUTO");
}

#include "accountdialog.moc"
