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
#include <ktextbrowser.h>

#include <QtCore/QUrl>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QFormLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QRegExpValidator>
#include <QtGui/QTextBrowser>

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

static QString settingsToUrl( const QWizard *wizard )
{
  QString pathPattern;

  if ( wizard->field( "serverTypeCitadel" ).toBool() )
    pathPattern = "/groupdav/";
  else if ( wizard->field( "serverTypeDavIcal" ).toBool() )
    pathPattern = "/caldav.php/principals/users/$user$/";
  else if ( wizard->field( "serverTypeEGroupware" ).toBool() )
    pathPattern = "/groupdav.php/";
  else if ( wizard->field( "serverTypeOpenGroupware" ).toBool() )
    pathPattern = "/zidestore/dav/$user/";
  else if ( wizard->field( "serverTypeScalableOGo" ).toBool() )
    pathPattern = "/SOGo/dav/$user$/";
  else if ( wizard->field( "serverTypeScalix" ).toBool() )
    pathPattern = "/api/dav/Principals/$user$";
  else if ( wizard->field( "serverTypeZarafa" ).toBool() )
    pathPattern = "/caldav/$user$/";
  else if ( wizard->field( "serverTypeZimbra" ).toBool() )
    pathPattern = "/principals/users/$user$/";

  pathPattern.replace( "$user$", wizard->field( "credentialsUserName" ).toString() );

  QUrl url;
  if ( wizard->field( "connectionUseSecureConnection" ).toBool() )
    url.setScheme( "https" );
  else
    url.setScheme( "http" );

  url.setHost( wizard->field( "connectionHost" ).toString() );
  url.setPath( pathPattern );

  return url.toString();
}

SetupWizard::SetupWizard( QWidget *parent )
  : QWizard( parent )
{
  addPage( new ServerTypePage );
  addPage( new ConnectionPage );
  addPage( new CredentialsPage );
  addPage( new CheckPage );
}

QString SetupWizard::displayName() const
{
  if ( field( "serverTypeCitadel" ).toBool() )
    return i18n( "Citadel" );
  else if ( field( "serverTypeDavIcal" ).toBool() )
    return i18n( "DAViCal" );
  else if ( field( "serverTypeEGroupware" ).toBool() )
    return i18n( "eGroupware" );
  else if ( field( "serverTypeOpenGroupware" ).toBool() )
    return i18n( "OpenGroupware" );
  else if ( field( "serverTypeScalableOGo" ).toBool() )
    return i18n( "ScalableOGo" );
  else if ( field( "serverTypeScalix" ).toBool() )
    return i18n( "Scalix" );
  else if ( field( "serverTypeZarafa" ).toBool() )
    return i18n( "Zarafa" );
  else if ( field( "serverTypeZimbra" ).toBool() )
    return i18n( "Zimbra" );

  return QString();
}

SetupWizard::Url::List SetupWizard::urls() const
{
  Url::List urls;

  if ( field( "serverTypeCitadel" ).toBool() ) {
    Url url;
    url.protocol = DavUtils::GroupDav;
    url.url = settingsToUrl( this );
    url.userName = field( "credentialsUserName" ).toString();

    urls << url;
  } else if ( field( "serverTypeDavIcal" ).toBool() ) {
    Url url;
    url.protocol = DavUtils::CalDav;
    url.url = settingsToUrl( this );
    url.userName = field( "credentialsUserName" ).toString();

    urls << url;
  } else if ( field( "serverTypeEGroupware" ).toBool() ) {
    Url url;
    url.protocol = DavUtils::GroupDav;
    url.url = settingsToUrl( this );
    url.userName = field( "credentialsUserName" ).toString();

    urls << url;
  } else if ( field( "serverTypeOpenGroupware" ).toBool() ) {
    Url url;
    url.protocol = DavUtils::GroupDav;
    url.url = settingsToUrl( this );
    url.userName = field( "credentialsUserName" ).toString();

    urls << url;
  } else if ( field( "serverTypeScalableOGo" ).toBool() ) {
    Url contactUrl;
    contactUrl.protocol = DavUtils::CardDav;
    contactUrl.url = settingsToUrl( this ) + "Contacts/";
    contactUrl.userName = field( "credentialsUserName" ).toString();

    Url calendarUrl;
    calendarUrl.protocol = DavUtils::CalDav;
    calendarUrl.url = settingsToUrl( this ) + "Calendar/";
    calendarUrl.userName = field( "credentialsUserName" ).toString();

    urls << contactUrl << calendarUrl;
  } else if ( field( "serverTypeScalix" ).toBool() ) {
    Url url;
    url.protocol = DavUtils::CalDav;
    url.url = settingsToUrl( this );
    url.userName = field( "credentialsUserName" ).toString();

    urls << url;
  } else if ( field( "serverTypeZarafa" ).toBool() ) {
    Url url;
    url.protocol = DavUtils::CalDav;
    url.url = settingsToUrl( this );
    url.userName = field( "credentialsUserName" ).toString();

    urls << url;
  } else if ( field( "serverTypeZimbra" ).toBool() ) {
    Url url;
    url.protocol = DavUtils::CalDav;
    url.url = settingsToUrl( this );
    url.userName = field( "credentialsUserName" ).toString();

    urls << url;
  }

  return urls;
}

ServerTypePage::ServerTypePage( QWidget *parent )
  : QWizardPage( parent )
{
  setTitle( i18n( "Groupware Server" ) );
  setSubTitle( i18n( "Select the groupware server the resource shall be configured for" ) );

  QVBoxLayout *layout = new QVBoxLayout( this );

  mServerGroup = new QButtonGroup( this );
  mServerGroup->setExclusive( true );
  connect( mServerGroup, SIGNAL( buttonClicked( QAbstractButton* ) ), SIGNAL( completeChanged() ) );

  QRadioButton *button;

  button = new QRadioButton( i18n( "Citadel" ) );
  mServerGroup->addButton( button );
  layout->addWidget( button );
  registerField( "serverTypeCitadel", button );

  button = new QRadioButton( i18n( "DAVical" ) );
  mServerGroup->addButton( button );
  layout->addWidget( button );
  registerField( "serverTypeDavIcal", button );

  button = new QRadioButton( i18n( "eGroupware" ) );
  mServerGroup->addButton( button );
  layout->addWidget( button );
  registerField( "serverTypeEGroupware", button );

  button = new QRadioButton( i18n( "OpenGroupware" ) );
  mServerGroup->addButton( button );
  layout->addWidget( button );
  registerField( "serverTypeOpenGroupware", button );

  button = new QRadioButton( i18n( "ScalableOGo" ) );
  mServerGroup->addButton( button );
  layout->addWidget( button );
  registerField( "serverTypeScalableOGo", button );

  button = new QRadioButton( i18n( "Scalix" ) );
  mServerGroup->addButton( button );
  layout->addWidget( button );
  registerField( "serverTypeScalix", button );

  button = new QRadioButton( i18n( "Zarafa" ) );
  mServerGroup->addButton( button );
  layout->addWidget( button );
  registerField( "serverTypeZarafa", button );

  button = new QRadioButton( i18n( "Zimbra" ) );
  mServerGroup->addButton( button );
  layout->addWidget( button );
  registerField( "serverTypeZimbra", button );

  layout->addStretch( 1 );
}

bool ServerTypePage::isComplete() const
{
  return (mServerGroup->checkedButton() != 0);
}

ConnectionPage::ConnectionPage( QWidget *parent )
  : QWizardPage( parent )
{
  setTitle( i18n( "Connection" ) );
  setSubTitle( i18n( "Enter the connection information for the groupware server" ) );

  QFormLayout *layout = new QFormLayout( this );

  QRegExp hostnameRegexp( "^[a-z0-9][.a-z0-9-]*[a-z0-9]$" );
  mHost = new KLineEdit;
  mHost->setValidator( new QRegExpValidator( hostnameRegexp, this ) );
  layout->addRow( i18n( "Host" ), mHost );

  mUseSecureConnection = new QCheckBox( i18n( "Use secure connection" ) );
  mUseSecureConnection->setChecked( true );
  layout->addRow( QString(), mUseSecureConnection );

  registerField( "connectionHost*", mHost );
  registerField( "connectionUseSecureConnection", mUseSecureConnection );
}

CredentialsPage::CredentialsPage( QWidget *parent )
  : QWizardPage( parent )
{
  setTitle( i18n( "Login Credentials" ) );
  setSubTitle( i18n( "Enter your credentials to login to the groupware server" ) );

  QFormLayout *layout = new QFormLayout( this );

  mUserName = new KLineEdit;
  layout->addRow( i18n( "User" ), mUserName );

  registerField( "credentialsUserName*", mUserName );
}

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
