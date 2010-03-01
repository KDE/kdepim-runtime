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

  foreach ( const KUrl &url, remoteUrls() )
    newUrlConfiguration( url.prettyUrl() );
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
  const KUrl::List urls = remoteUrls();

  foreach( const KUrl &url, urls ) {
    const QString groupName = url.prettyUrl();
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

  if ( authenticationRequired( searchUrl ) ) {
    const QString user = username( searchUrl );
    fullUrl.setUser( user );
    fullUrl.setPassword( password( searchUrl, user ) );
  }

  return DavUtils::DavUrl( fullUrl, protocol( searchUrl ) );
}

DavUtils::DavUrl Settings::davUrlFromUrl( const QString &url )
{
  DavUtils::DavUrl davUrl;
  QString configuredUrl;

  foreach( const QString &collectionUrl, mCollectionsUrlsMapping.keys() ) {
    if ( url.startsWith( collectionUrl ) ) {
      configuredUrl = mCollectionsUrlsMapping.value( collectionUrl );
      break;
    }
  }

  if ( !configuredUrl.isEmpty() )
    davUrl = configuredDavUrl( configuredUrl, url );

  return davUrl;
}

void Settings::addCollectionUrlMapping( const QString &collectionUrl, const QString &configuredUrl )
{
  mCollectionsUrlsMapping.insert( collectionUrl, configuredUrl );
}

Settings::UrlConfiguration * Settings::newUrlConfiguration( const QString &url )
{
  UrlConfiguration *urlConfig;

  if ( !mUrls.contains( url ) ) {
    urlConfig = new UrlConfiguration();
    urlConfig->mUrl = url;
  } else {
    urlConfig = mUrls[ url ];
  }

  KConfigGroup group( config(), urlConfig->mUrl );
  setCurrentGroup( urlConfig->mUrl );

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
  addItem( protocolItem );

  KConfigSkeletonItem *authenticationRequiredItem =
      new KConfigSkeleton::ItemBool( currentGroup(), QLatin1String( "authenticationRequired" ), urlConfig->mAuthReq );
  addItem( authenticationRequiredItem );

  KConfigSkeletonItem *useKWalletItem =
      new KConfigSkeleton::ItemBool( currentGroup(), QLatin1String( "useKWallet" ), urlConfig->mUseKWallet );
  addItem( useKWalletItem );

  KConfigSkeletonItem *usernameItem =
      new KConfigSkeleton::ItemString( currentGroup(), QLatin1String( "username" ), urlConfig->mUser );
  addItem( usernameItem );

  setRemoteUrls( remoteUrls() << urlConfig->mUrl );

  mUrls.insert( urlConfig->mUrl, urlConfig );

  return urlConfig;
}

void Settings::removeUrlConfiguration( const QString &url )
{
  if ( !mUrls.contains( url ) )
    return;

  kDebug() << "Deleting URL " << url;

  QStringList newUrls;

  const QStringList oldUrls = remoteUrls();
  foreach ( const QString &tmpUrl, oldUrls ) {
    if ( tmpUrl != url )
      newUrls << tmpUrl;
  }

  setRemoteUrls( newUrls );
  kDebug() << "Remaining URLs " << newUrls;

  UrlConfiguration *urlConfig = mUrls[ url ];
  mUrls.remove( url );
  config()->deleteGroup( url );

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
  if ( mUrls.contains( url ) )
    return mUrls[ url ]->mAuthReq;
  else
    return false;
}

DavUtils::Protocol Settings::protocol( const QString &url ) const
{
  if ( mUrls.contains( url ) )
    return DavUtils::Protocol( mUrls[ url ]->mProtocol );
  else
    return DavUtils::CalDav;
}

QString Settings::username( const QString &url ) const
{
  if ( mUrls.contains( url ) )
    return mUrls[ url ]->mUser;
  else
    return QString();
}

bool Settings::useKWallet( const QString &url ) const
{
  if ( mUrls.contains( url ) )
    return mUrls[ url ]->mUseKWallet;
  else
    return false;
}

void Settings::setPassword( const QString &url, const QString &username, const QString &password )
{
  const QString passwordKey( username + QLatin1Char( '-' ) + url );

  mCachedPasswords[ passwordKey ] = password;

  if ( useKWallet( url ) )
    storePassword( url, username, password );
}

QString Settings::password( const QString &url, const QString &username )
{
  const QString passwordKey( username + QLatin1Char( '-' ) + url );

  QString password;
  if ( mCachedPasswords.contains( passwordKey ) )
    password = mCachedPasswords[ passwordKey ];
  else
    password = requestPassword( url, username );

  return password;
}

QString Settings::requestPassword( const QString &url, const QString &username )
{
  QString password;

  if ( authenticationRequired( url ) ) {
    const QString passwordKey( username + QLatin1Char( '-' ) + url );

    if ( useKWallet( url ) ) {
      if ( !mWallet )
        mWallet = Wallet::openWallet( Wallet::NetworkWallet(), mWinId );

      if ( mWallet && mWallet->isOpen() &&
           mWallet->hasFolder( "dav-akonadi-resource" ) &&
           mWallet->setFolder( "dav-akonadi-resource" ) ) {
        mWallet->readPassword( passwordKey, password );
      }
    } else {
      password = promptForPassword( url, username );
    }

    mCachedPasswords[ passwordKey ] = password;
  }

  return password;
}

QString Settings::promptForPassword( const QString &url, const QString &username )
{
  KPasswordDialog dlg;
  dlg.setPrompt( i18n( "Please enter the password for %1 at %2" ).arg( username ).arg( url ) );
  dlg.exec();

  QString password;
  if ( dlg.result() == QDialog::Accepted )
    password = dlg.password();

  return password;
}

void Settings::storePassword( const QString &url, const QString &username, const QString &password )
{
  if ( !mWallet )
    mWallet = Wallet::openWallet( Wallet::NetworkWallet(), mWinId );

  if ( mWallet && mWallet->isOpen() ) {
    if ( !mWallet->hasFolder( "dav-akonadi-resource" ) )
      mWallet->createFolder( "dav-akonadi-resource" );

    mWallet->setFolder( "dav-akonadi-resource" );

    const QString passwordKey( username + QLatin1Char( '-' ) + url );
    mWallet->writePassword( passwordKey, password );
  }
}

#include "settings.moc"
