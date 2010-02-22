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

namespace KWallet {
  class Wallet;
}

class Settings : public SettingsBase
{
  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.kde.Akonadi.davCalendar.Wallet" )

  public:
    class UrlConfiguration
    {
      public:
        UrlConfiguration();
        QString mUrl;
        QString mUser;
        int mProtocol;
        bool mAuthReq;
        bool mUseKWallet;
    };

    Settings();
    ~Settings();
    static Settings* self();
    void setWinId( WId wid );
    void writeConfig();

    DavUtils::DavUrl::List configuredDavUrls();
    /**
     * Creates and returns the DavUrl that corresponds to the configuration for searchUrl.
     * If finalUrl is supplied, then it will be used in the returned object instead of the searchUrl.
     */
    DavUtils::DavUrl configuredDavUrl( const QString &searchUrl, const QString &finalUrl = QString() );

    /**
     * Creates the DavUrl from the configured URL that most closely matches the given url.
     * Most closely means url.startsWith( configuredUrl ).
     */
    DavUtils::DavUrl davUrlFromUrl( const QString &url );

    UrlConfiguration * newUrlConfiguration( const QString &url );
    void removeUrlConfiguration( const QString &url );
    UrlConfiguration * urlConfiguration( const QString &url );

    bool authenticationRequired( const QString &url ) const;
    DavUtils::Protocol protocol( const QString &url ) const;
    QString username( const QString &url ) const;
    bool useKWallet( const QString &url ) const;

    void setPassword( const QString &url, const QString &username, const QString &password );
    QString password( const QString &url, const QString &username );

  private:
    QString requestPassword( const QString &url, const QString &username );
    QString promptForPassword( const QString &url, const QString &username );
    void storePassword( const QString &url, const QString &username, const QString &password );

    WId mWinId;
    QMap<QString, UrlConfiguration*> mUrls;
    QList<UrlConfiguration*> mToDeleteUrlConfigs;
    QMap<QString, QString> mCachedPasswords;
    KWallet::Wallet *mWallet;
};

#endif
