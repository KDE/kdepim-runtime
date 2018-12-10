/*
   Copyright (C) 2013 Daniel Vrátil <dvratil@redhat.com>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef GOOGLESETTINGS_H
#define GOOGLESETTINGS_H

#include "settingsbase.h"

#include <qwindowdefs.h>

/**
 * @brief Settings object
 *
 * Provides read-only access to application clientId and
 * clientSecret and read-write access to accessToken and
 * refreshToken.
 */
class GoogleSettings : public SettingsBase
{
    Q_OBJECT

public:
    GoogleSettings();
    void setWindowId(WId id);
    void setResourceId(const QString &resourceIdentifier);

    QString appId() const;

    QString clientId() const;
    QString clientSecret() const;

    virtual QString account() const;
    virtual void setAccount(const QString &account);

private:
    WId m_winId;
    QString m_resourceId;
};

#endif // GOOGLESETTINGS_H
