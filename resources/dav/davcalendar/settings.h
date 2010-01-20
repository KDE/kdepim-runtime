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

namespace KWallet {
  class Wallet;
}

class Settings : public SettingsBase
{
  Q_OBJECT
  Q_CLASSINFO( "D-Bus Interface", "org.kde.Akonadi.davCalendar.Wallet" )
public:
  Settings();
  ~Settings();
  static Settings* self();
  void setWinId( WId wid );
  void storePassword();
  void getPassword();
  void requestPassword( const QString &username );

public Q_SLOTS:
  Q_SCRIPTABLE QString password() const;
  Q_SCRIPTABLE void setPassword( const QString &password );

private:
  WId winId;
  QString pwd;
  KWallet::Wallet *wallet;
};

#endif
