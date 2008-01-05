/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "settings.h"

#include <kglobal.h>

class SettingsHelper
{
  public:
    SettingsHelper() : q(0) {}
    ~SettingsHelper() { delete q; }
    Settings *q;
};

K_GLOBAL_STATIC(SettingsHelper, s_globalSettings)

Settings *Settings::self()
{
  if (!s_globalSettings->q) {
    new Settings;
    s_globalSettings->q->readConfig();
  }

  return s_globalSettings->q;
}

Settings::Settings() : SettingsBase()
{
  Q_ASSERT(!s_globalSettings->q);
  s_globalSettings->q = this;
}

QString Settings::password() const
{
  return mPassword;
}

void Settings::setPassword(const QString & password)
{
  mPassword = password;
}

