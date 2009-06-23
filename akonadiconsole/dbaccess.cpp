/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "dbaccess.h"

#include <akonadi/private/xdgbasedirs_p.h>

#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>

#include <KGlobal>
#include <KLocale>
#include <KMessageBox>

using namespace Akonadi;

class DbAccessPrivate
{
  public:
    DbAccessPrivate()
    {
      const QString serverConfigFile = XdgBaseDirs::akonadiServerConfigFile( XdgBaseDirs::ReadWrite );
      QSettings settings( serverConfigFile, QSettings::IniFormat );

      const QString driver = settings.value( "General/Driver", "QMYSQL" ).toString();
      database = QSqlDatabase::addDatabase( driver );
      settings.beginGroup( driver );
      database.setHostName( settings.value( "Host", QString() ).toString() );
      database.setDatabaseName( settings.value( "Name", "akonadi" ).toString() );
      database.setUserName( settings.value( "User", QString() ).toString() );
      database.setPassword( settings.value( "Password", QString() ).toString() );
      database.setConnectOptions( settings.value( "Options", QString() ).toString() );
      if ( !database.open() ) {
        KMessageBox::error( 0, i18n( "Failed to connect to database: %1", database.lastError().text() ) );
      }
    }

    QSqlDatabase database;
};

K_GLOBAL_STATIC( DbAccessPrivate, sInstance )

QSqlDatabase DbAccess::database()
{
  return sInstance->database;
}
