/*
    Copyright (C) 2011-2013  Dan Vratil <dan@progdan.cz>

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

#include "googlesettings.h"
#include "settingsbase.h"

GoogleSettings::GoogleSettings():
    m_winId(0)
{
}

QString GoogleSettings::clientId() const
{
    return QLatin1String("554041944266.apps.googleusercontent.com");
}

QString GoogleSettings::clientSecret() const
{
    return QLatin1String("mdT1DjzohxN3npUUzkENT0gO");
}

void GoogleSettings::setWindowId( WId id )
{
    m_winId = id;
}

void GoogleSettings::setResourceId( const QString &resourceIdentificator )
{
    m_resourceId = resourceIdentificator;
}

QString GoogleSettings::account() const
{
    return SettingsBase::account();
}

void GoogleSettings::setAccount( const QString &account )
{
    SettingsBase::setAccount( account );
}

