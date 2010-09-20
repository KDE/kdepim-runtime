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

#include <QtDBus/QDBusConnection>

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
  const QStringList splitString = serialized.split( '|' );

  if ( splitString.size() == 3 ) {
    mUrl = splitString.at( 2 );
    mProtocol = DavUtils::protocolByName( splitString.at( 1 ) );
    mUser = splitString.at( 0 );
  }
}

QString Settings::UrlConfiguration::serialize()
{
  QString serialized = mUser;
  serialized.append( '|' ).append( DavUtils::protocolName( DavUtils::Protocol( mProtocol ) ) );
  serialized.append( '|' ).append( mUrl );
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
  Q_ASSERT( !s_globalSettings->q );
  s_globalSettings->q = this;

  new SettingsAdaptor( this );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ), this,
                              QDBusConnection::ExportAdaptors | QDBusConnection::ExportScriptableContents );

  foreach ( const QString &serializedUrl, remoteUrls() ) {
    UrlConfiguration *urlConfig = new UrlConfiguration( serializedUrl );
    mUrls[ urlConfig->mUrl ] = urlConfig;
  }
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

DavUtils::DavUrl::List Settings::configuredDavUrls()
{
  DavUtils::DavUrl::List davUrls;
  QMapIterator<QString, UrlConfiguration*> it( mUrls );

  while ( it.hasNext() ) {
    it.next();
    davUrls << configuredDavUrl( it.key() );
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

  const QString user = username( searchUrl );
  fullUrl.setUser( user );

  return DavUtils::DavUrl( fullUrl, protocol( searchUrl ) );
}

DavUtils::DavUrl Settings::davUrlFromCollectionUrl( const QString &collectionUrl, const QString &finalUrl )
{
  DavUtils::DavUrl davUrl;
  QString targetUrl = finalUrl.isEmpty() ? collectionUrl : finalUrl;

  if ( mCollectionsUrlsMapping.contains( collectionUrl ) ) {
    davUrl = configuredDavUrl( mCollectionsUrlsMapping[ collectionUrl ], targetUrl );
  }

  return davUrl;
}

void Settings::addCollectionUrlMapping( const QString &collectionUrl, const QString &configuredUrl )
{
  mCollectionsUrlsMapping.insert( collectionUrl, configuredUrl );
}

void Settings::newUrlConfiguration( Settings::UrlConfiguration *urlConfig )
{
  if ( mUrls.contains( urlConfig->mUrl ) ) {
    removeUrlConfiguration( urlConfig->mUrl );
    updateRemoteUrls();
  }

  mUrls[ urlConfig->mUrl ] = urlConfig;
  updateRemoteUrls();
}

void Settings::removeUrlConfiguration( const QString &url )
{
  if ( !mUrls.contains( url ) )
    return;

  delete mUrls[ url ];
  mUrls.remove( url );
  updateRemoteUrls();
}

Settings::UrlConfiguration * Settings::urlConfiguration( const QString &url )
{
  UrlConfiguration *ret = 0;
  if ( mUrls.contains( url ) )
    ret = mUrls[url];

  return ret;
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

#include "settings.moc"
