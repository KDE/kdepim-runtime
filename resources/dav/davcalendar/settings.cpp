/*
    Copyright (c) 2009 Grégory Oestreicher <greg@kamago.net>
      Based on an original work for the IMAP resource which is :
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>
    Copyright (c) 2008 Omat Holding B.V. <info@omat.nl>

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

#include "settings.h"

#include "settingsadaptor.h"

#include <kglobal.h>
#include <klocale.h>
#include <kpassworddialog.h>
#include <kwallet.h>

#include <QtDBus/QDBusConnection>

using namespace KWallet;

class SettingsHelper
{
  public:
    SettingsHelper()
      : q( 0 )
    {
    }

    ~SettingsHelper()
    {
      delete q;
    }

    Settings *q;
};

K_GLOBAL_STATIC( SettingsHelper, s_globalSettings )

Settings::UrlConfiguration::UrlConfiguration()
{
}

Settings *Settings::self()
{
  if ( !s_globalSettings->q ) {
    new Settings;
    s_globalSettings->q->readConfig();
  }

  return s_globalSettings->q;
}

Settings::Settings()
  : SettingsBase(), mWinId( 0 ), mWallet( 0 )
{
  Q_ASSERT( !s_globalSettings->q );
  s_globalSettings->q = this;

  new SettingsAdaptor( this );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ), this,
                              QDBusConnection::ExportAdaptors | QDBusConnection::ExportScriptableContents );

  foreach( KUrl url, this->remoteUrls() ) {
    UrlConfiguration *urlConfig = this->newUrlConfiguration( url.prettyUrl() );
  }
}

Settings::~Settings()
{
  delete mWallet;

  QMapIterator<QString, UrlConfiguration*> it( mUrls );
  while( it.hasNext() ) {
    it.next();
    delete it.value();
  }
}

void Settings::setWinId( WId winId )
{
  mWinId = winId;
}

void Settings::writeConfig()
{
  SettingsBase::writeConfig();
  QListIterator<UrlConfiguration*> it( mToDeleteUrlConfigs );
  while( it.hasNext() )
    delete it.next();
}

DavUtils::DavUrl::List Settings::configuredDavUrls()
{
  DavUtils::DavUrl::List davUrls;
  KUrl::List urls = this->remoteUrls();

  foreach( KUrl url, urls ) {
    QString groupName = url.prettyUrl();
    davUrls << configuredDavUrl( groupName );
  }

  return davUrls;
}

DavUtils::DavUrl Settings::configuredDavUrl( const QString &searchUrl, const QString &finalUrl )
{
  KUrl fullUrl;

  if ( !finalUrl.isEmpty() )
    fullUrl = finalUrl;
  else
    fullUrl = searchUrl;

  if ( this->authenticationRequired( searchUrl ) ) {
    QString username = this->username( searchUrl );
    fullUrl.setUser( username );
    fullUrl.setPassword( this->password( searchUrl, username ) );
  }

  DavUtils::Protocol protocol = this->protocol( searchUrl );
  DavUtils::DavUrl davUrl( fullUrl, protocol );
  return davUrl;
}

DavUtils::DavUrl Settings::davUrlFromUrl( const QString &url )
{
  DavUtils::DavUrl ret;
  QString configuredUrl;

  foreach( const QString &tmpUrl, this->remoteUrls() ) {
    if ( url.startsWith( tmpUrl ) ) {
      configuredUrl = tmpUrl;
    }
  }

  if ( !configuredUrl.isEmpty() ) {
    ret = this->configuredDavUrl( configuredUrl, url );
  }

  return ret;
}

Settings::UrlConfiguration * Settings::newUrlConfiguration( const QString &url )
{
  UrlConfiguration *urlConfig;

  if ( !mUrls.contains( url ) ) {
    urlConfig = new UrlConfiguration();
    urlConfig->mUrl = url;
  }
  else {
    urlConfig = mUrls[url];
  }

  KConfigGroup group( this->config(), urlConfig->mUrl );
  this->setCurrentGroup( urlConfig->mUrl );

  QList<KConfigSkeleton::ItemEnum::Choice2> protocolValues;
  {
    KConfigSkeleton::ItemEnum::Choice2 choice;
    choice.name = QLatin1String( "caldav" );
    protocolValues.append( choice );
  }
  {
    KConfigSkeleton::ItemEnum::Choice2 choice;
    choice.name = QLatin1String( "carddav" );
    protocolValues.append( choice );
  }
  {
    KConfigSkeleton::ItemEnum::Choice2 choice;
    choice.name = QLatin1String( "groupdav" );
    protocolValues.append( choice );
  }
  KConfigSkeletonItem *protocolItem =
      new KConfigSkeleton::ItemEnum( currentGroup(), QLatin1String( "protocol" ), urlConfig->mProtocol, protocolValues );
  this->addItem( protocolItem );

  KConfigSkeletonItem *authenticationRequiredItem =
      new KConfigSkeleton::ItemBool( currentGroup(), QLatin1String( "authenticationRequired" ), urlConfig->mAuthReq );
  this->addItem( authenticationRequiredItem );

  KConfigSkeletonItem *useKWalletItem =
      new KConfigSkeleton::ItemBool( currentGroup(), QLatin1String( "useKWallet" ), urlConfig->mUseKWallet );
  this->addItem( useKWalletItem );

  KConfigSkeletonItem *usernameItem =
      new KConfigSkeleton::ItemString( currentGroup(), QLatin1String( "username" ), urlConfig->mUser );
  this->addItem( usernameItem );

  QStringList rUrls = this->remoteUrls();
  rUrls << urlConfig->mUrl;
  this->setRemoteUrls( rUrls );

  mUrls.insert( urlConfig->mUrl, urlConfig );
  return urlConfig;
}

void Settings::removeUrlConfiguration( const QString &url )
{
  if ( !mUrls.contains( url ) )
    return;

  kDebug() << "Deleting URL " << url;

  QStringList oldUrls = this->remoteUrls();
  QStringList newUrls;
  foreach( QString tmpUrl, oldUrls ) {
    if ( tmpUrl != url )
      newUrls << tmpUrl;
  }
  this->setRemoteUrls( newUrls );
  kDebug() << "Remaining URLs " << newUrls;

  UrlConfiguration *urlConfig = mUrls[url];
  mUrls.remove( url );
  this->config()->deleteGroup( url );
  // Apparently it is not possible to call delete urlConfig here,
  // so postpone the deletion until after writeConfig()
  mToDeleteUrlConfigs << urlConfig;
}

Settings::UrlConfiguration * Settings::urlConfiguration( const QString &url )
{
  UrlConfiguration *ret = 0;
  if ( mUrls.contains( url ) )
    ret = mUrls[url];
  return ret;
}

bool Settings::authenticationRequired( const QString &url ) const
{
  if ( mUrls.contains( url ) ) {
    return mUrls[url]->mAuthReq;
  }
  else {
    return false;
  }
}

DavUtils::Protocol Settings::protocol( const QString &url ) const
{
  if ( mUrls.contains( url ) ) {
    return DavUtils::Protocol( mUrls[url]->mProtocol );
  }
  else {
    return DavUtils::CalDav;
  }
}

QString Settings::username( const QString &url ) const
{
  if ( mUrls.contains( url ) ) {
    return mUrls[url]->mUser;
  }
  else {
    return QString();
  }
}

bool Settings::useKWallet( const QString &url ) const
{
  if ( mUrls.contains( url ) ) {
    return mUrls[url]->mUseKWallet;
  }
  else {
    return false;
  }
}

void Settings::setPassword( const QString &url, const QString &username, const QString &password )
{
  QString passwordKey( username );
  passwordKey.append( "-" ).append( url );
  mCachedPasswords[passwordKey] = password;

  if ( useKWallet( url ) )
    storePassword( url, username, password );
}

QString Settings::password( const QString &url, const QString &username )
{
  QString ret;
  QString passwordKey( username );
  passwordKey.append( "-" ).append( url );

  if ( mCachedPasswords.contains( passwordKey ) ) {
    ret = mCachedPasswords[passwordKey];
  }
  else {
    ret = requestPassword( url, username );
  }

  return ret;
}

QString Settings::requestPassword( const QString &url, const QString &username )
{
  QString ret;

  if ( authenticationRequired( url ) ) {
    QString passwordKey( username );
    passwordKey.append( "-" ).append( url );

    if ( useKWallet( url ) ) {
      if ( !mWallet )
        mWallet = Wallet::openWallet( Wallet::NetworkWallet(), mWinId );

      if ( mWallet && mWallet->isOpen() &&
           mWallet->hasFolder( "dav-akonadi-resource" ) &&
           mWallet->setFolder( "dav-akonadi-resource" ) ) {
        mWallet->readPassword( passwordKey, ret );
           }
    }
    else {
      ret = promptForPassword( url, username );
    }

    mCachedPasswords[passwordKey] = ret;
  }

  return ret;
}

QString Settings::promptForPassword( const QString &url, const QString &username )
{
  QString ret;
  KPasswordDialog dlg;
  dlg.setPrompt( i18n( "Please enter the password for %1 at %2" ).arg( username ).arg( url ) );
  dlg.exec();

  if ( dlg.result() == QDialog::Accepted )
    ret = dlg.password();

  return ret;
}

void Settings::storePassword( const QString &url, const QString &username, const QString &password )
{
  if ( !mWallet )
    mWallet = Wallet::openWallet( Wallet::NetworkWallet(), mWinId );

  if ( mWallet && mWallet->isOpen() ) {
    if ( !mWallet->hasFolder( "dav-akonadi-resource" ) )
      mWallet->createFolder( "dav-akonadi-resource" );

    mWallet->setFolder( "dav-akonadi-resource" );
    QString passwordKey( username );
    passwordKey.append( "-" ).append( url );
    mWallet->writePassword( passwordKey, password );
  }
}

#include "settings.moc"
