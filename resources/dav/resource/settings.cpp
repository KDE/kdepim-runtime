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
#ifdef HAVE_ACCOUNTS
#include "../shared/getcredentialsjob.h"
#endif

#include <kapplication.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kglobal.h>
#include <klineedit.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kwallet.h>

#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QFile>
#include <QtCore/QPointer>
#include <QtCore/QRegExp>
#include <QtDBus/QDBusConnection>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>

#ifdef HAVE_ACCOUNTS
#include <Accounts/Account>
#include <Accounts/Manager>
#endif

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

Settings::UrlConfiguration::UrlConfiguration( const QString &serialized )
{
  const QStringList splitString = serialized.split( QLatin1Char('|') );

  if ( splitString.size() == 3 ) {
    mUrl = splitString.at( 2 );
    mProtocol = DavUtils::protocolByName( splitString.at( 1 ) );
    mUser = splitString.at( 0 );
  }
}

QString Settings::UrlConfiguration::serialize()
{
  QString serialized = mUser;
  serialized.append( QLatin1Char('|') ).append( DavUtils::protocolName( DavUtils::Protocol( mProtocol ) ) );
  serialized.append( QLatin1Char('|') ).append( mUrl );
  return serialized;
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
  : SettingsBase(), mWinId( 0 )
{

#ifdef HAVE_ACCOUNTS
  m_manager = 0;
#endif
  Q_ASSERT( !s_globalSettings->q );
  s_globalSettings->q = this;

  new SettingsAdaptor( this );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ), this,
                              QDBusConnection::ExportAdaptors | QDBusConnection::ExportScriptableContents );

  if ( settingsVersion() == 1 )
    updateToV2();

  if ( settingsVersion() == 2 )
    updateToV3();
}

Settings::~Settings()
{
  QMapIterator<QString, UrlConfiguration*> it( mUrls );
  while ( it.hasNext() ) {
    it.next();
    delete it.value();
  }
}

void Settings::setWinId( WId winId )
{
  mWinId = winId;
}

void Settings::cleanup()
{
  QFile cacheFile( mCollectionsUrlsMappingCache );
  cacheFile.remove();
}

void Settings::setResourceIdentifier(const QString& identifier)
{
  mResourceIdentifier = identifier;
}

void Settings::setDefaultPassword( const QString &password )
{
  savePassword( mResourceIdentifier, QLatin1String("$default$"), password );
}

QString Settings::defaultPassword()
{
  return loadPassword( mResourceIdentifier, QLatin1String("$default$") );
}

DavUtils::DavUrl::List Settings::configuredDavUrls()
{
  if ( mUrls.isEmpty() )
    buildUrlsList();
  DavUtils::DavUrl::List davUrls;
  QMapIterator<QString, UrlConfiguration*> it( mUrls );

  while ( it.hasNext() ) {
    it.next();
    QStringList split = it.key().split( QLatin1Char(',') );
    davUrls << configuredDavUrl( DavUtils::protocolByName( split.at( 1 ) ), split.at( 0 ) );
  }

  return davUrls;
}

DavUtils::DavUrl Settings::configuredDavUrl( DavUtils::Protocol proto, const QString &searchUrl, const QString &finalUrl )
{
  if ( mUrls.isEmpty() )
    buildUrlsList();

  KUrl fullUrl;

  if ( !finalUrl.isEmpty() )
    fullUrl = finalUrl;
  else
    fullUrl = searchUrl;

  const QString user = username( proto, searchUrl );
  fullUrl.setUser( user );
  fullUrl.setPassword( password( proto, searchUrl ) );

  return DavUtils::DavUrl( fullUrl, proto );
}

DavUtils::DavUrl Settings::davUrlFromCollectionUrl( const QString &collectionUrl, const QString &finalUrl )
{
  if ( mCollectionsUrlsMapping.isEmpty() )
    loadMappings();

  DavUtils::DavUrl davUrl;
  QString targetUrl = finalUrl.isEmpty() ? collectionUrl : finalUrl;

  if ( mCollectionsUrlsMapping.contains( collectionUrl ) ) {
    QStringList split = mCollectionsUrlsMapping[ collectionUrl ].split( QLatin1Char(',') );
    if ( split.size() == 2 )
      davUrl = configuredDavUrl( DavUtils::protocolByName( split.at( 1 ) ), split.at( 0 ), targetUrl );
  }

  return davUrl;
}

void Settings::addCollectionUrlMapping( DavUtils::Protocol proto, const QString &collectionUrl, const QString &configuredUrl )
{
  if ( mCollectionsUrlsMapping.isEmpty() )
    loadMappings();

  QString value = configuredUrl + QLatin1Char(',') + DavUtils::protocolName( proto );
  mCollectionsUrlsMapping.insert( collectionUrl, value );

  // Update the cache now
  //QMap<QString, QString> tmp( mCollectionsUrlsMapping );
  QFile cacheFile( mCollectionsUrlsMappingCache );
  if ( cacheFile.open( QIODevice::WriteOnly ) ) {
    QDataStream cache( &cacheFile );
    cache.setVersion( QDataStream::Qt_4_7 );
    cache << mCollectionsUrlsMapping;
    cacheFile.close();
  }
}

QStringList Settings::mappedCollections( DavUtils::Protocol proto, const QString &configuredUrl )
{
  if ( mCollectionsUrlsMapping.isEmpty() )
    loadMappings();

  QString value = configuredUrl + QLatin1Char(',') + DavUtils::protocolName( proto );
  return mCollectionsUrlsMapping.keys( value );
}

void Settings::reloadConfig()
{
#ifdef HAVE_ACCOUNTS
    importFromAccounts();
#endif
    buildUrlsList();
    updateRemoteUrls();
    loadMappings();
}

void Settings::newUrlConfiguration( Settings::UrlConfiguration *urlConfig )
{
  QString key = urlConfig->mUrl + QLatin1Char(',') + DavUtils::protocolName( DavUtils::Protocol( urlConfig->mProtocol ) );

  if ( mUrls.contains( key ) ) {
    removeUrlConfiguration( DavUtils::Protocol( urlConfig->mProtocol ), urlConfig->mUrl );
  }

  mUrls[ key ] = urlConfig;
  if ( urlConfig->mUser != QLatin1String( "$default$" ) )
    savePassword( key, urlConfig->mUser, urlConfig->mPassword );
  updateRemoteUrls();
}

void Settings::removeUrlConfiguration( DavUtils::Protocol proto, const QString &url )
{
  QString key = url + QLatin1Char(',') + DavUtils::protocolName( proto );

  if ( !mUrls.contains( key ) )
    return;

  delete mUrls[ key ];
  mUrls.remove( key );
  updateRemoteUrls();
}

Settings::UrlConfiguration * Settings::urlConfiguration( DavUtils::Protocol proto, const QString &url )
{
  QString key = url + QLatin1Char(',') + DavUtils::protocolName( proto );

  UrlConfiguration *ret = 0;
  if ( mUrls.contains( key ) )
    ret = mUrls[ key ];

  return ret;
}

// DavUtils::Protocol Settings::protocol( const QString &url ) const
// {
//   if ( mUrls.contains( url ) )
//     return DavUtils::Protocol( mUrls[ url ]->mProtocol );
//   else
//     return DavUtils::CalDav;
// }

QString Settings::username( DavUtils::Protocol proto, const QString &url ) const
{
  QString key = url + QLatin1Char(',') + DavUtils::protocolName( proto );

  if ( mUrls.contains( key ) )
    if ( mUrls[ key ]->mUser == QLatin1String( "$default$" ) )
      return defaultUsername();
#ifdef HAVE_ACCOUNTS
    else if ( mUrls[ key ]->mUser == QLatin1String( "$accounts$" ) )
      return accountsUsername();
#endif
    else
      return mUrls[ key ]->mUser;
  else
    return QString();
}

QString Settings::password(DavUtils::Protocol proto, const QString& url)
{
  QString key = url + QLatin1Char(',') + DavUtils::protocolName( proto );

  if ( mUrls.contains( key ) )
    if ( mUrls[ key ]->mUser == QLatin1String( "$default$" ) )
      return defaultPassword();
    else
      return mUrls[ key ]->mPassword;
  else
    return QString();
}
#ifdef HAVE_ACCOUNTS
void Settings::importFromAccounts()
{
  kDebug();
  Accounts::AccountId id = accountId();
  kDebug() << "Account Id: " << id;

  if ( !m_manager ) {
    m_manager = new Accounts::Manager( this );
  }

  if ( !m_manager->accountList().contains( id ) ) {
    return;
  }

  removeAccountsDisabledServices();
  addAccountsEnabledServices();

  setSettingsVersion( 3 );
  writeConfig();
}

void Settings::addAccountsEnabledServices()
{
  kDebug();
  Accounts::Account *acc = m_manager->account( accountId() );
  QStringList enabledServices = accountServices();
  kDebug() << "Enabled" << enabledServices;
  foreach( QString serviceType, enabledServices ) {
    Accounts::ServiceList services = acc->services( serviceType );
    foreach( const Accounts::Service &service, services ) {
      configureAccountService( acc, service );
    }
  }
}

void Settings::removeAccountsDisabledServices()
{
  kDebug();
  QStringList urls = remoteUrls();
  for (int i = 0; i < urls.size(); ++i) {
    if ( !urls.at( i ).startsWith( "$accounts$" ) ) {
      continue;
    }

    if (urls.at( i ).contains( "carddav" )
      && accountServices().contains( "dav-contacts" )) {
      continue;
    }
    if (urls.at( i ).contains( "caldav" )
      && accountServices().contains( "dav-calendar" )) {
      continue;
    }

    urls.removeAt( i );
  }

  setRemoteUrls( urls );
}

void Settings::configureAccountService(Accounts::Account *acc, const Accounts::Service& service)
{
  kDebug() << "Configuring service: " << service.name();

  acc->selectService();
  QString domain = acc->valueAsString( "dav/scheme" ) + "://" + acc->valueAsString( "dav/host" );
  acc->selectService( service );

  QString type;
  if ( service.serviceType() == "dav-contacts" ) {
    type = "CardDav";
  } else {
    type = "CalDav";
  }

  QString url = "$accounts$|" + type + "|" + domain + acc->valueAsString("dav/path");
  kDebug() << url;
  acc->selectService();

  QStringList urls = remoteUrls();
  foreach ( const QString &serializedUrl, urls ) {
    if ( url == serializedUrl ) {
      kDebug() << "Url already configured";
      return;
    }
  }

  kDebug() << "Adding url";
  urls.append( url );
  setRemoteUrls( urls );
}
#endif

void Settings::buildUrlsList()
{
  foreach ( const QString &serializedUrl, remoteUrls() ) {
    UrlConfiguration *urlConfig = new UrlConfiguration( serializedUrl );
    QString key = urlConfig->mUrl + QLatin1Char(',') + DavUtils::protocolName( DavUtils::Protocol( urlConfig->mProtocol ) );
    QString pass = loadPassword( key, urlConfig->mUser );
    if ( !pass.isNull() ) {
      urlConfig->mPassword = pass;
      mUrls[ key ] = urlConfig;
    }
  }
}

void Settings::loadMappings()
{
  QString collectionsMappingCacheBase = QString::fromLatin1( "akonadi-davgroupware/%1_c2u.dat" ).arg( KApplication::applicationName() );
  mCollectionsUrlsMappingCache = KStandardDirs::locateLocal( "data", collectionsMappingCacheBase );
  QFile collectionsMappingsCache( mCollectionsUrlsMappingCache );

  if ( collectionsMappingsCache.exists() ) {
    if ( collectionsMappingsCache.open( QIODevice::ReadOnly ) ) {
      QDataStream cache( &collectionsMappingsCache );
      cache >> mCollectionsUrlsMapping;
      collectionsMappingsCache.close();
    }
  } else if ( !collectionsUrlsMappings().isEmpty() ) {
    QByteArray rawMappings = QByteArray::fromBase64( collectionsUrlsMappings().toLatin1() );
    QDataStream stream( &rawMappings, QIODevice::ReadOnly );
    stream >> mCollectionsUrlsMapping;
    setCollectionsUrlsMappings( QString() );
  }
}

void Settings::updateRemoteUrls()
{
  QStringList newUrls;

  QMapIterator<QString, UrlConfiguration*> it( mUrls );
  while ( it.hasNext() ) {
    it.next();
    newUrls << it.value()->serialize();
  }

  setRemoteUrls( newUrls );
}

void Settings::savePassword( const QString &key, const QString &user, const QString &password )
{
  QString entry = key + QLatin1Char(',') + user;
  mPasswordsCache[entry] = password;

  KWallet::Wallet *wallet = KWallet::Wallet::openWallet( KWallet::Wallet::NetworkWallet(), mWinId );
  if ( !wallet )
    return;

  if ( !wallet->hasFolder( KWallet::Wallet::PasswordFolder() ) )
    wallet->createFolder( KWallet::Wallet::PasswordFolder() );

  if ( !wallet->setFolder( KWallet::Wallet::PasswordFolder() ) )
    return;

  wallet->writePassword( entry, password );
}

QString Settings::loadPassword( const QString &key, const QString &user )
{
  QString entry;
  QString pass;

  if ( user == QLatin1String( "$default$" ) )
    entry = mResourceIdentifier + QLatin1Char(',') + user;
#ifdef HAVE_ACCOUNTS
  else if (user == QLatin1String( "$accounts$" ))
      return loadPasswordFromAccounts();
#endif
  else
    entry = key + QLatin1Char(',') + user;

  if ( mPasswordsCache.contains( entry ) )
    return mPasswordsCache[entry];

  KWallet::Wallet *wallet = KWallet::Wallet::openWallet( KWallet::Wallet::NetworkWallet(), mWinId );
  if ( wallet ) {
    if ( !wallet->hasFolder( KWallet::Wallet::PasswordFolder() ) )
      wallet->createFolder( KWallet::Wallet::PasswordFolder() );

    if ( wallet->setFolder( KWallet::Wallet::PasswordFolder() ) ) {
      if ( !wallet->hasEntry( entry ) ) {
        pass = promptForPassword( user );
        wallet->writePassword( entry, pass );
      } else {
        wallet->readPassword( entry, pass );
      }
    }
  }

  if ( pass.isNull() && !KWallet::Wallet::isEnabled() )
    pass = promptForPassword( user );

  if ( !pass.isNull() )
    mPasswordsCache[entry] = pass;

  return pass;
}

#ifdef HAVE_ACCOUNTS
QString Settings::loadPasswordFromAccounts()
{
    kDebug() << "Getting credentials for: " << accountId();
    GetCredentialsJob *job = new GetCredentialsJob(accountId());
    job->exec();

    return job->credentialsData().value("Secret").toString();
}

QString Settings::accountsUsername() const
{
    kDebug() << "Getting credentials for: " << accountId();
    GetCredentialsJob *job = new GetCredentialsJob(accountId());
    job->exec();

    kDebug() << "Got some: " << job->credentialsData();

    return job->credentialsData().value("UserName").toString();
}
#endif

QString Settings::promptForPassword( const QString &user )
{
  QPointer<KDialog> dlg = new KDialog();
  QString password;

  QWidget *mainWidget = new QWidget( dlg );
  QVBoxLayout *vLayout = new QVBoxLayout();
  mainWidget->setLayout( vLayout );
  QLabel *label = new QLabel( i18n( "A password is required for user %1",
                                    ( user == QLatin1String( "$default$" ) ? defaultUsername() : user ) ),
                              mainWidget
                            );
  vLayout->addWidget( label );
  QHBoxLayout *hLayout = new QHBoxLayout();
  label = new QLabel( i18n( "Password: " ), mainWidget );
  hLayout->addWidget( label );
  KLineEdit *lineEdit = new KLineEdit();
  lineEdit->setPasswordMode( true );
  hLayout->addWidget( lineEdit );
  vLayout->addLayout( hLayout );
  dlg->setMainWidget( mainWidget );
  lineEdit->setFocus();

  const int result = dlg->exec();

  if ( result == QDialog::Accepted && !dlg.isNull() ) {
    password = lineEdit->text();
  }

  delete dlg;
  return password;
}

void Settings::updateToV2()
{
  // Take the first URL that was configured to get the username that
  // has the most chances being the default

  QStringList urls = remoteUrls();
  if ( urls.isEmpty() )
    return;

  QString urlConfigStr = urls.at( 0 );
  UrlConfiguration urlConfig( urlConfigStr );
  QRegExp regexp( QLatin1Char('^') + urlConfig.mUser );

  QMutableStringListIterator it( urls );
  while ( it.hasNext() ) {
    it.next();
    it.value().replace( regexp, QLatin1String( "$default$" ) );
  }

  setDefaultUsername( urlConfig.mUser );
  QString key = urlConfig.mUrl + QLatin1Char(',') + DavUtils::protocolName( DavUtils::Protocol( urlConfig.mProtocol ) );
  QString pass = loadPassword( key, urlConfig.mUser );
  if ( !pass.isNull() )
    setDefaultPassword( pass );
  setRemoteUrls( urls );
  setSettingsVersion( 2 );
  writeConfig();
}

void Settings::updateToV3()
{
  QStringList updatedUrls;

  foreach ( const QString &url, remoteUrls() ) {
    QStringList splitUrl = url.split( QLatin1Char('|') );

    if ( splitUrl.size() == 3 ) {
      DavUtils::Protocol protocol = DavUtils::protocolByTranslatedName( splitUrl.at( 1 ) );
      splitUrl[1] = DavUtils::protocolName( protocol );
      updatedUrls << splitUrl.join( QLatin1String("|") );
    }
  }

  setRemoteUrls( updatedUrls );
  setSettingsVersion( 3 );
  writeConfig();
}

