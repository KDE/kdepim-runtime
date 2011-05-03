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

#ifndef SETTINGS_H
#define SETTINGS_H

#include "settingsbase.h"

#include "davutils.h"

#include <QtCore/QMap>

class Settings : public SettingsBase
{
  Q_OBJECT

  public:
    class UrlConfiguration
    {
      public:
        UrlConfiguration();
        explicit UrlConfiguration( const QString &serialized );

        /**
         * Serializes the object.
         * The string is the concatenation of the fields separated by a "|" :
         * user|protocol|url
         * The "protocol" component is the symbolic name of the protocol,
         * as returned by DavUtils::protocolName().
         */
        QString serialize();
        QString mUrl;
        QString mUser;
        QString mPassword;
        int mProtocol;
    };

    Settings();
    virtual ~Settings();
    static Settings* self();
    void setWinId( WId wid );
    void cleanup();

    DavUtils::DavUrl::List configuredDavUrls();

    /**
     * Creates and returns the DavUrl that corresponds to the configuration for searchUrl.
     * If finalUrl is supplied, then it will be used in the returned object instead of the searchUrl.
     */
    DavUtils::DavUrl configuredDavUrl( DavUtils::Protocol protocol, const QString &searchUrl, const QString &finalUrl = QString() );

	/**
	 * Creates and return the DavUrl from the configured URL that has a mapping with @p collectionUrl.
	 * If @p finalUrl is supplied it will be used in the returned object, else @p collectionUrl will
	 * be used.
	 * If no configured URL can be found the returned DavUrl will have an empty url().
	 */
     DavUtils::DavUrl davUrlFromCollectionUrl( const QString &collectionUrl, const QString &finalUrl = QString() );

    /**
     * Add a new mapping between the collection URL, as seen on the backend, and the
     * URL configured by the user. A mapping here means that the collectionUrl has
     * been discovered by a DavCollectionsFetchJob on the configuredUrl.
     */
    void addCollectionUrlMapping( DavUtils::Protocol protocol, const QString &collectionUrl, const QString &configuredUrl );

    void newUrlConfiguration( UrlConfiguration *urlConfig );
    void removeUrlConfiguration( DavUtils::Protocol protocol, const QString &url );
    UrlConfiguration * urlConfiguration( DavUtils::Protocol protocol, const QString &url );

    //DavUtils::Protocol protocol( const QString &url ) const;
    QString username( DavUtils::Protocol protocol, const QString &url ) const;
    QString password( DavUtils::Protocol protocol, const QString &url );

  private:
    void updateRemoteUrls();
    void savePassword( const QString &key, const QString &user, const QString &password );
    QString loadPassword( const QString &key, const QString &user );

    WId mWinId;
    QMap<QString, UrlConfiguration*> mUrls;
    QString mCollectionsUrlsMappingCache;
    QMap<QString, QString> mCollectionsUrlsMapping;
    QList<UrlConfiguration*> mToDeleteUrlConfigs;
};

#endif
