/* Copyright 2010 Thomas McGuire <mcguire@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef SETTINGS_H
#define SETTINGS_H

#include "settingsbase.h"

#include <qwindowdefs.h>

/**
 * Extended settings class that allows setting the password over dbus, which is used by the
 * wizard.
 */
class Settings : public SettingsBase
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.Akonadi.POP3.Wallet")
public:
    Settings(const KSharedConfigPtr &config);

    void setWindowId(WId id);
    void setResourceId(const QString &resourceIdentifier);
    static Settings *self();

public Q_SLOTS:
    Q_SCRIPTABLE void setPassword(const QString &password);
private:
    WId mWinId;
    QString mResourceId;
};

#endif
