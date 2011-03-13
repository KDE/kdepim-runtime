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

#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
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

  if ( !collectionsUrlsMappings().isEmpty() ) {
    QByteArray rawMappings = QByteArray::fromBase64( collectionsUrlsMappings().toAscii() );
    QDataStream stream( &rawMappings, QIODevice::ReadOnly );
    stream >> mCollectionsUrlsMapping;
    Q_ASSERT_X( !mCollectionsUrlsMapping.isEmpty(), "Settings::Settings()", "Failed to import collections mappings" );
  }

  foreach ( const QString &serializedUrl, remoteUrls() ) {
    UrlConfiguration *urlConfig = new UrlConfiguration( serializedUrl );
    QString key = urlConfig->mUrl + "," + DavUtils::protocolName( DavUtils::Protocol( urlConfig->mProtocol ) );
    mUrls[ key ] = urlConfig;
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
    QStringList split = it.key().split( "," );
    davUrls << configuredDavUrl( DavUtils::protocolByName( split.at( 1 ) ), split.at( 0 ) );
  }

  return davUrls;
}

DavUtils::DavUrl Settings::configuredDavUrl( DavUtils::Protocol proto, const QString &searchUrl, const QString &finalUrl )
{
  KUrl fullUrl;

  if ( !finalUrl.isEmpty() )
    fullUrl = finalUrl;
  else
    fullUrl = searchUrl;

  const QString user = username( proto, searchUrl );
  fullUrl.setUser( user );

  return DavUtils::DavUrl( fullUrl, proto );
}

DavUtils::DavUrl Settings::davUrlFromCollectionUrl( const QString &collectionUrl, const QString &finalUrl )
{
  DavUtils::DavUrl davUrl;
  QString targetUrl = finalUrl.isEmpty() ? collectionUrl : finalUrl;

  if ( mCollectionsUrlsMapping.contains( collectionUrl ) ) {
    QStringList split = mCollectionsUrlsMapping[ collectionUrl ].split( "," );
    if ( split.size() == 2 )
      davUrl = configuredDavUrl( DavUtils::protocolByName( split.at( 1 ) ), split.at( 0 ), targetUrl );
  }

  return davUrl;
}

void Settings::addCollectionUrlMapping( DavUtils::Protocol proto, const QString &collectionUrl, const QString &configuredUrl )
{
  QString value = configuredUrl + "," + DavUtils::protocolName( proto );
  mCollectionsUrlsMapping.insert( collectionUrl, value );

  // Update the settings now
  QMap<QString, QString> tmp( mCollectionsUrlsMapping );
  QByteArray rawMappings;
  QDataStream stream( &rawMappings, QIODevice::WriteOnly );
  stream << tmp;
  setCollectionsUrlsMappings( QString::fromAscii( rawMappings.toBase64() ) );
}

void Settings::newUrlConfiguration( Settings::UrlConfiguration *urlConfig )
{
  QString key = urlConfig->mUrl + "," + DavUtils::protocolName( DavUtils::Protocol( urlConfig->mProtocol ) );

  if ( mUrls.contains( key ) ) {
    removeUrlConfiguration( DavUtils::Protocol( urlConfig->mProtocol ), urlConfig->mUrl );
  }

  mUrls[ key ] = urlConfig;
  updateRemoteUrls();
}

void Settings::removeUrlConfiguration( DavUtils::Protocol proto, const QString &url )
{
  QString key = url + "," + DavUtils::protocolName( proto );

  if ( !mUrls.contains( key ) )
    return;

  delete mUrls[ key ];
  mUrls.remove( key );
  updateRemoteUrls();
}

Settings::UrlConfiguration * Settings::urlConfiguration( DavUtils::Protocol proto, const QString &url )
{
  QString key = url + "," + DavUtils::protocolName( proto );

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
  QString key = url + "," + DavUtils::protocolName( proto );

  if ( mUrls.contains( key ) )
    return mUrls[ key ]->mUser;
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
