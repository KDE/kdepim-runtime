/*
    Copyright (C) 2011, 2012  Dan Vratil <dan@progdan.cz>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef SETTINGS_H
#define SETTINGS_H

#include "settingsbase.h"

#include <qwindowdefs.h>

/**
 * @brief Settings object
 *
 * Provides read-only access to application clientId and
 * clientSecret and read-write access to accessToken and
 * refreshToken.
 */
class Settings: public SettingsBase
{
    Q_OBJECT
    Q_CLASSINFO( "D-Bus Interface", "org.kde.Akonadi.GoogleContacts.ExtendedSettings" )
  public:
    Settings();
    void setWindowId( WId id );
    void setResourceId( const QString &resourceIdentifier );
    static Settings *self();

    QString appId() const;

    QString clientId() const;
    QString clientSecret() const;

  private:
    WId m_winId;
    QString m_resourceId;

};

#endif // SETTINGS_H
