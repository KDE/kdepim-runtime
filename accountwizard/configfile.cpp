/*
    Copyright (c) 2010 Laurent Montel <montel@kde.org>

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

#include "configfile.h"

#include <KConfig>
#include <KConfigGroup>
#include <KLocale>


ConfigFile::ConfigFile( const QString & configName, QObject *parent )
  : SetupObject( parent )
{
  m_name = configName;
  m_config = new KConfig( configName );
}

ConfigFile::~ConfigFile()
{
  delete m_config;
}

void ConfigFile::create()
{
  emit info( i18n( "Writing config file for %1...",m_name ) );

  const int count = m_configData.count();
  for (int i = 0; i < count; ++i) {
    KConfigGroup grp = m_config->group( m_configData.at( i ).group );
    grp.writeEntry( m_configData.at( i ).key, m_configData.at( i ).value );
  }

  m_config->sync();
  emit finished( i18n( "Config file for %1 is writing.", m_name ) );
}

void ConfigFile::destroy()
{
  emit info( i18n( "Config file for %1 was not changed.", m_name ) );
}

void ConfigFile::setName( const QString &name )
{
  m_name = name;
}


void ConfigFile::setConfig( const QString &group, const QString &key, const QString &value )
{
  Config conf;
  conf.group = group;
  conf.key = key;
  conf.value = value;
  m_configData.append( conf );
}

#include "configfile.moc"
