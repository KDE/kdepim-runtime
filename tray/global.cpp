/* This file is part of the KDE project
   Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "global.h"

#include <QString>
#include <QSettings>

#include <akonadi/private/xdgbasedirs_p.h>

using namespace Akonadi;

namespace Tray
{
void Global::init()
{
    const QString serverConfigFile = XdgBaseDirs::akonadiServerConfigFile( XdgBaseDirs::ReadWrite );
    QSettings settings( serverConfigFile, QSettings::IniFormat );

    const QString defaultDriver = QLatin1String( "QMYSQL" );
    const QString driver = settings.value( QLatin1String( "General/Driver" ), defaultDriver ).toString();
    if ( driver != QLatin1String( "QMYSQL" ) ) {
        m_parsed = true;
        return;
    }

    settings.beginGroup( "QMYSQL" );
    const QString host = settings.value( "Host", "" ).toString();
    if ( host.isEmpty() ) {
        const QString options = settings.value( "Options", "" ).toString();
        const QStringList list = options.split( "=" );
        m_dboptions = "--socket=" + list.at( 1 );
    } else {
        m_dboptions = "--host=" + host;
        m_dboptions.append( " --user=" + settings.value( "User", "" ).toString() );
        m_dboptions.append( " --password=" + settings.value( "Password", "" ).toString() );
    }
    m_dbname = settings.value( "Name", "akonadi" ).toString();
    settings.endGroup();

    m_parsed = true;
}

const QString Global::dboptions()
{
    if ( !m_parsed )
        init();

    return m_dboptions;
}

const QString Global::dbname()
{
    if ( !m_parsed )
        init();

    return m_dbname;
}

}
