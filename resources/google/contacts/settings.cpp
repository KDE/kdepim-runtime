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

#include "settings.h"
#include "settingsbase.h"
#include "settingsadaptor.h"

#include <KGlobal>
#include <KWallet/Wallet>

#include <QDBusConnection>

class SettingsHelper
{
  public:
    SettingsHelper() : q( 0 )
    {
    }

    ~SettingsHelper()
    {
      delete q;
      q = 0;
    }

    Settings *q;
};

K_GLOBAL_STATIC( SettingsHelper, s_globalSettings )

Settings::Settings()
{
  Q_ASSERT( !s_globalSettings->q );
  s_globalSettings->q = this;

  new SettingsAdaptor( this );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ), this,
      QDBusConnection::ExportAdaptors | QDBusConnection::ExportScriptableContents );
}

Settings *Settings::self()
{
  if ( !s_globalSettings->q ) {
    new Settings;
    s_globalSettings->q->readConfig();
  }

  return s_globalSettings->q;

}

QString Settings::clientId() const
{
  return "554041944266.apps.googleusercontent.com";
}

QString Settings::clientSecret() const
{
  return "mdT1DjzohxN3npUUzkENT0gO";
}

void Settings::setWindowId( WId id )
{
  m_winId = id;
}

void Settings::setResourceId( const QString &resourceIdentificator )
{
  m_resourceId = resourceIdentificator;
}
