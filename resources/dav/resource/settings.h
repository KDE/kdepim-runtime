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
        QString mUrl;
        QString mUser;
        int mProtocol;
    };

    Settings();
    ~Settings();
    static Settings* self();
    void setWinId( WId wid );
    virtual void readConfig();
    virtual void writeConfig();

    DavUtils::DavUrl::List configuredDavUrls();
    /**
     * Creates and returns the DavUrl that corresponds to the configuration for searchUrl.
     * If finalUrl is supplied, then it will be used in the returned object instead of the searchUrl.
     */
    DavUtils::DavUrl configuredDavUrl( const QString &searchUrl, const QString &finalUrl = QString() );

    /**
     * Creates the DavUrl from the configured URL that most closely matches the given url.
     * Most closely means url.startsWith( collectionUrl ), where collectionUrl
     * is taken from the mappings registered with addCollectionUrlMapping().
     */
    DavUtils::DavUrl davUrlFromUrl( const QString &url );

    /**
     * Add a new mapping between the collection URL, as seen on the backend, and the
     * URL configured by the user. A mapping here means that the collectionUrl has
     * been discovered by a DavCollectionsFetchJob on the configuredUrl.
     */
    void addCollectionUrlMapping( const QString &collectionUrl, const QString &configuredUrl );

    UrlConfiguration * newUrlConfiguration( const QString &url );
    void removeUrlConfiguration( const QString &url );
    UrlConfiguration * urlConfiguration( const QString &url );

    DavUtils::Protocol protocol( const QString &url ) const;
    QString username( const QString &url ) const;

  private:
    WId mWinId;
    QMap<QString, UrlConfiguration*> mUrls;
    QMap<QString, QString> mCollectionsUrlsMapping;
    QList<UrlConfiguration*> mToDeleteUrlConfigs;
};

#endif
