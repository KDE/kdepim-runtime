/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "setupwizard.h"

#include "davcollectionsmultifetchjob.h"

#include <kicon.h>
#include <klocale.h>
#include <klineedit.h>
#include <kservice.h>
#include <kservicetypetrader.h>
#include <ktextbrowser.h>

#include <QtCore/QUrl>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QCheckBox>
#include <QtGui/QFormLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QRegExpValidator>
#include <QtGui/QTextBrowser>

#include <kdebug.h>

enum GroupwareServers
{
  Citadel,
  DAVical,
  eGroupware,
  OpenGroupware,
  ScalableOGo,
  Scalix,
  Zarafa,
  Zimbra
};

static QString settingsToUrl( const QWizard *wizard, const QString &protocol )
{
  QString desktopFilePath = wizard->property( "providerDesktopFilePath" ).toString();
  if ( desktopFilePath.isEmpty() )
    return QString();

  KService::Ptr service = KService::serviceByStorageId( desktopFilePath );
  if ( !service )
    return QString();

  QStringList supportedProtocols = service->property( "X-DavGroupware-SupportedProtocols" ).toStringList();
  if ( !supportedProtocols.contains( protocol ) )
    return QString();

  QString pathPattern;

  QString pathPropertyName( "X-DavGroupware-" + protocol + "Path" );
  if ( service->property( pathPropertyName ).isNull() )
    return QString();

  pathPattern.append( service->property( pathPropertyName ).toString() + "/" );
  pathPattern.replace( "$user$", wizard->field( "credentialsUserName" ).toString() );

  QString localPath = wizard->field( "installationPath" ).toString();
  if ( !localPath.isEmpty() ) {
    if ( !localPath.startsWith( "/" ) )
      pathPattern.prepend( "/" + localPath );
    else
      pathPattern.prepend( localPath );
  }

  QUrl url;

  if ( !wizard->property( "usePredefinedProvider" ).isNull() ) {
    if ( service->property( "X-DavGroupware-ProviderUsesSSL" ).toBool() )
      url.setScheme( "https" );
    else
      url.setScheme( "http" );

    QString hostPropertyName( "X-DavGroupware-" + protocol + "Host" );
    if ( service->property( hostPropertyName ).isNull() )
      return QString();

    url.setHost( service->property( hostPropertyName ).toString() );
    url.setPath( pathPattern );
  }
  else {
    if ( wizard->field( "connectionUseSecureConnection" ).toBool() )
      url.setScheme( "https" );
    else
      url.setScheme( "http" );

    QString host = wizard->field( "connectionHost" ).toString();
    if ( host.isEmpty() )
      return QString();
    QStringList hostParts = host.split( ':' );
    url.setHost( hostParts.at( 0 ) );
    url.setPath( pathPattern );

    if ( hostParts.size() == 2 ) {
      int port = hostParts.at( 1 ).toInt();
      if ( port )
        url.setPort( port );
    }
  }

  return url.toString();
}

/*
 * SetupWizard
 */

SetupWizard::SetupWizard( QWidget *parent )
  : QWizard( parent )
{
  setWindowTitle( i18n( "DAV groupware configuration wizard" ) );
  setPage( W_CredentialsPage, new CredentialsPage );
  setPage( W_PredefinedProviderPage, new PredefinedProviderPage );
  setPage( W_ServerTypePage, new ServerTypePage );
  setPage( W_ConnectionPage, new ConnectionPage );
  setPage( W_CheckPage, new CheckPage );
}

QString SetupWizard::displayName() const
{
  QString desktopFilePath = property( "providerDesktopFilePath" ).toString();
  if ( desktopFilePath.isEmpty() )
    return QString();

  KService::Ptr service = KService::serviceByStorageId( desktopFilePath );
  if ( !service )
    return QString();

  return service->name();
}

SetupWizard::Url::List SetupWizard::urls() const
{
  Url::List urls;

  QString desktopFilePath = property( "providerDesktopFilePath" ).toString();
  if ( desktopFilePath.isEmpty() )
    return urls;

  KService::Ptr service = KService::serviceByStorageId( desktopFilePath );
  if ( !service )
    return urls;

  QStringList supportedProtocols = service->property( "X-DavGroupware-SupportedProtocols" ).toStringList();
  foreach ( const QString &protocol, supportedProtocols ) {
    Url url;

    if ( protocol == "CalDav" )
      url.protocol = DavUtils::CalDav;
    else if ( protocol == "CardDav" )
      url.protocol = DavUtils::CardDav;
    else if ( protocol == "GroupDav" )
      url.protocol = DavUtils::GroupDav;
    else
      return urls;

    QString urlStr = settingsToUrl( this, protocol );

    if ( !urlStr.isEmpty() ) {
      url.url = urlStr;
      url.userName = field( "credentialsUserName" ).toString();
      urls << url;
    }
  }

  return urls;
}

/*
 * CredentialsPage
 */

CredentialsPage::CredentialsPage( QWidget *parent )
  : QWizardPage( parent )
{
  setTitle( i18n( "Login Credentials" ) );
  setSubTitle( i18n( "Enter your credentials to login to the groupware server" ) );

  QFormLayout *layout = new QFormLayout( this );

  mUserName = new KLineEdit;
  layout->addRow( i18n( "User" ), mUserName );
  registerField( "credentialsUserName*", mUserName );

  mPassword = new KLineEdit;
  mPassword->setPasswordMode( true );
  layout->addRow( i18n( "Password" ), mPassword );
  registerField( "credentialsPassword*", mPassword );
}

int CredentialsPage::nextId() const
{
  QString userName = field( "credentialsUserName" ).toString();
  if ( userName.endsWith( QLatin1String( "@yahoo.com" ) ) ) {
    KService::List offers;
    offers = KServiceTypeTrader::self()->query( "DavGroupwareProvider", "Name == 'Yahoo!'" );
    if ( offers.isEmpty() )
      return SetupWizard::W_ServerTypePage;

    wizard()->setProperty( "usePredefinedProvider", true );
    wizard()->setProperty( "predefinedProviderName", offers.at( 0 )->name() );
    wizard()->setProperty( "providerDesktopFilePath", offers.at( 0 )->desktopEntryPath() );
    return SetupWizard::W_PredefinedProviderPage;
  }
  else {
    return SetupWizard::W_ServerTypePage;
  }
}

/*
 * PredefinedProviderPage
 */

PredefinedProviderPage::PredefinedProviderPage( QWidget* parent )
  : QWizardPage( parent )
{
  setTitle( i18n( "Predefined provider found" ) );
  setSubTitle( i18n( "Select if you want to use the auto-detected provider" ) );

  QVBoxLayout *layout = new QVBoxLayout( this );

  mLabel = new QLabel;
  layout->addWidget( mLabel );

  mProviderGroup = new QButtonGroup( this );
  mProviderGroup->setExclusive( true );

  mUseProvider = new QRadioButton;
  mProviderGroup->addButton( mUseProvider );
  mUseProvider->setChecked( true );
  layout->addWidget( mUseProvider );

  mDontUseProvider = new QRadioButton( i18n( "No, choose another server" ) );
  mProviderGroup->addButton( mDontUseProvider );
  layout->addWidget( mDontUseProvider );
}

void PredefinedProviderPage::initializePage()
{
  mLabel->setText( i18n( "Based on the email address you used as a login, this wizard\n"
                         "can configure automatically an account for %1 services.\n"
                         "Do you wish to do so?", wizard()->property( "predefinedProviderName" ).toString() ) );

  mUseProvider->setText( i18n( "Yes, use %1 as provider", wizard()->property( "predefinedProviderName" ).toString() ) );
}

int PredefinedProviderPage::nextId() const
{
  if ( mUseProvider->isChecked() ) {
    return SetupWizard::W_CheckPage;
  }
  else {
    wizard()->setProperty( "usePredefinedProvider", QVariant() );
    wizard()->setProperty( "providerDesktopFilePath", QVariant() );
    return SetupWizard::W_ServerTypePage;
  }
}

/*
 * ServerTypePage
 */

ServerTypePage::ServerTypePage( QWidget *parent )
  : QWizardPage( parent )
{
  setTitle( i18n( "Groupware Server" ) );
  setSubTitle( i18n( "Select the groupware server the resource shall be configured for" ) );
  setFinalPage( true );

  mProvidersCombo = new QComboBox( this );
  KService::List providers;
  KServiceTypeTrader *trader = KServiceTypeTrader::self();
  providers = trader->query( "DavGroupwareProvider" );
  foreach ( const KService::Ptr &provider, providers ) {
    mProvidersCombo->addItem( provider->name(), provider->desktopEntryPath() );
  }
  registerField( "provider", mProvidersCombo, "currentText" );

  QVBoxLayout *layout = new QVBoxLayout( this );

  mServerGroup = new QButtonGroup( this );
  mServerGroup->setExclusive( true );

  QRadioButton *button;

  QHBoxLayout *hLayout = new QHBoxLayout( this );
  button = new QRadioButton( i18n( "Use one of those servers:" ) );
  mServerGroup->addButton( button );
  mServerGroup->setId( button, 0 );
  hLayout->addWidget( button );
  hLayout->addWidget( mProvidersCombo );
  hLayout->addStretch( 1 );
  layout->addLayout( hLayout );

  button = new QRadioButton( i18n( "Configure the resource manually" ) );
  mServerGroup->addButton( button );
  mServerGroup->setId( button, 1 );
  layout->addWidget( button );
  registerField( "manualConfiguration", button );

  connect( mServerGroup, SIGNAL(buttonReleased(int)),
           this, SLOT(typeToggled(int)) );

  layout->addStretch( 1 );
}

bool ServerTypePage::isComplete() const
{
  return false;
}

void ServerTypePage::initializePage()
{
  QWizardPage::initializePage();
  int buttonId = mServerGroup->checkedId();
  if ( buttonId != -1 ) {
    typeToggled( buttonId );
  }
  else {
    wizard()->button( QWizard::FinishButton )->setEnabled( false );
    wizard()->button( QWizard::NextButton )->setEnabled( false );
  }
}

bool ServerTypePage::validatePage()
{
  QVariant desktopFilePath = mProvidersCombo->itemData( mProvidersCombo->currentIndex() );
  if ( desktopFilePath.isNull() ) {
    return false;
  }
  else {
    wizard()->setProperty( "providerDesktopFilePath", desktopFilePath );
    return true;
  }
}

void ServerTypePage::typeToggled(int buttonId)
{
  bool enableFinish;

  if ( buttonId == 0 )
    enableFinish = false;
  else
    enableFinish = true;

  wizard()->button( QWizard::FinishButton )->setEnabled( enableFinish );
  wizard()->button( QWizard::NextButton )->setDisabled( enableFinish );
}

/*
 * ConnectionPage
 */

ConnectionPage::ConnectionPage( QWidget *parent )
  : QWizardPage( parent ), mPreviewLayout( 0 ), mCalDavUrlPreview( 0 ), mCardDavUrlPreview( 0 ), mGroupDavUrlPreview( 0 )
{
  setTitle( i18n( "Connection" ) );
  setSubTitle( i18n( "Enter the connection information for the groupware server" ) );

  mLayout = new QFormLayout( this );

  QRegExp hostnameRegexp( "^[a-z0-9][.a-z0-9-]*[a-z0-9](?::[0-9]+)?$" );
  mHost = new KLineEdit;
  registerField( "connectionHost*", mHost );
  mHost->setValidator( new QRegExpValidator( hostnameRegexp, this ) );
  mLayout->addRow( i18n( "Host" ), mHost );

  mPath = new KLineEdit;
  mLayout->addRow( i18n( "Installation path" ), mPath );
  registerField( "installationPath", mPath );

  mUseSecureConnection = new QCheckBox( i18n( "Use secure connection" ) );
  mUseSecureConnection->setChecked( true );
  registerField( "connectionUseSecureConnection", mUseSecureConnection );
  mLayout->addRow( QString(), mUseSecureConnection );

  connect( mHost, SIGNAL(textChanged(QString)), this, SLOT(urlElementChanged()) );
  connect( mPath, SIGNAL(textChanged(QString)), this, SLOT(urlElementChanged()) );
  connect( mUseSecureConnection, SIGNAL(toggled(bool)), this, SLOT(urlElementChanged()) );
}

void ConnectionPage::initializePage()
{
  KService::Ptr service = KService::serviceByStorageId( wizard()->property( "providerDesktopFilePath" ).toString() );
  if ( !service )
    return;

  QString providerInstallationPath = service->property( "X-DavGroupware-InstallationPath" ).toString();
  kDebug() << wizard()->property( "providerDesktopFilePath" ).toString() << providerInstallationPath;
  if ( !providerInstallationPath.isEmpty() )
    mPath->setText( providerInstallationPath );

  QStringList supportedProtocols = service->property( "X-DavGroupware-SupportedProtocols" ).toStringList();

  mPreviewLayout = new QFormLayout( this );
  mLayout->addRow( mPreviewLayout );

  if ( supportedProtocols.contains( "CalDav" ) ) {
    mCalDavUrlLabel = new QLabel( i18n( "Final URL (CalDav)" ) );
    mCalDavUrlPreview = new QLabel;
    mPreviewLayout->addRow( mCalDavUrlLabel, mCalDavUrlPreview );
  }
  if ( supportedProtocols.contains( "CardDav" ) ) {
    mCardDavUrlLabel = new QLabel( i18n( "Final URL (CardDav)" ) );
    mCardDavUrlPreview = new QLabel;
    mPreviewLayout->addRow( mCardDavUrlLabel, mCardDavUrlPreview );
  }
  if ( supportedProtocols.contains( "GroupDav" ) ) {
    mGroupDavUrlLabel = new QLabel( i18n( "Final URL (GroupDav)" ) );
    mGroupDavUrlPreview = new QLabel;
    mPreviewLayout->addRow( mGroupDavUrlLabel, mGroupDavUrlPreview );
  }
}

void ConnectionPage::cleanupPage()
{
  delete mPreviewLayout;

  if ( mCalDavUrlPreview ) {
    delete mCalDavUrlLabel;
    delete mCalDavUrlPreview;
    mCalDavUrlPreview = 0;
  }

  if ( mCardDavUrlPreview ) {
    delete mCardDavUrlLabel;
    delete mCardDavUrlPreview;
    mCardDavUrlPreview = 0;
  }

  if ( mGroupDavUrlPreview ) {
    delete mGroupDavUrlLabel;
    delete mGroupDavUrlPreview;
    mGroupDavUrlPreview = 0;
  }

  QWizardPage::cleanupPage();
}

void ConnectionPage::urlElementChanged()
{
  if ( mHost->text().isEmpty() ) {
    if ( mCalDavUrlPreview )
      mCalDavUrlPreview->setText( "-" );
    if ( mCardDavUrlPreview )
      mCardDavUrlPreview->setText( "-" );
    if ( mGroupDavUrlPreview )
      mGroupDavUrlPreview->setText( "-" );
  }
  else {
    if ( mCalDavUrlPreview )
      mCalDavUrlPreview->setText( settingsToUrl( this->wizard(), "CalDav" ) );
    if ( mCardDavUrlPreview )
      mCardDavUrlPreview->setText( settingsToUrl( this->wizard(), "CardDav" ) );
    if ( mGroupDavUrlPreview )
      mGroupDavUrlPreview->setText( settingsToUrl( this->wizard(), "GroupDav" ) );
  }
}

/*
 * CheckPage
 */

CheckPage::CheckPage( QWidget *parent )
  : QWizardPage( parent )
{
  setTitle( i18n( "Test Connection" ) );
  setSubTitle( i18n( "You can test now whether the groupware server can be accessed with the current configuration" ) );

  QVBoxLayout *layout = new QVBoxLayout( this );

  QPushButton *button = new QPushButton( i18n( "Test Connection" ) );
  layout->addWidget( button );

  mStatusLabel = new KTextBrowser;
  layout->addWidget( mStatusLabel );

  connect( button, SIGNAL( clicked() ), SLOT( checkConnection() ) );
}

void CheckPage::checkConnection()
{
  mStatusLabel->clear();

  DavUtils::DavUrl::List davUrls;

  // convert list of SetupWizard::Url to list of DavUtils::DavUrl
  const SetupWizard::Url::List urls = static_cast<SetupWizard*>( wizard() )->urls();
  foreach ( const SetupWizard::Url &url, urls ) {
    DavUtils::DavUrl davUrl;
    davUrl.setProtocol( url.protocol );

    KUrl serverUrl( url.url );
    serverUrl.setUser( url.userName );
    serverUrl.setPass( wizard()->field( "credentialsPassword" ).toString() );
    davUrl.setUrl( serverUrl );

    davUrls << davUrl;
  }

  // start the dav collections fetch job to test connectivity
  DavCollectionsMultiFetchJob *job = new DavCollectionsMultiFetchJob( davUrls, this );
  connect( job, SIGNAL( result( KJob* ) ), SLOT( onFetchDone( KJob* ) ) );
  job->start();
}

void CheckPage::onFetchDone( KJob *job )
{
  QString msg;
  QPixmap icon;

  if ( job->error() ) {
    msg = i18n( "An error occurred: " ) + job->errorText();
    icon = KIcon( "dialog-close" ).pixmap( 16, 16 );
  } else {
    msg = i18n( "Connected successfully" );
    icon = KIcon( "dialog-ok-apply" ).pixmap( 16, 16 );
  }

  mStatusLabel->setHtml( QString( "<html><body><img src=\"icon\"> %1</body></html>" ).arg( msg ) );
  mStatusLabel->document()->addResource( QTextDocument::ImageResource, QUrl( "icon" ), QVariant( icon ) );
}

#include "setupwizard.moc"
