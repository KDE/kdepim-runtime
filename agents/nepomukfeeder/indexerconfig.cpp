/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright 2013  Vishesh Handa <me@vhanda.in>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "indexerconfig.h"

#include <KConfigGroup>

#include <KDirWatch>
#include <KStandardDirs>
#include <KDebug>

IndexerConfig::IndexerConfig(QObject* parent)
    : QObject(parent)
    , m_config("akonadi_nepomuk_feederrc")
    , m_isEnabled( true )
{
    KDirWatch* dirWatch = KDirWatch::self();
    connect( dirWatch, SIGNAL( dirty( const QString& ) ),
             this, SLOT( slotConfigDirty() ) );
    connect( dirWatch, SIGNAL( created( const QString& ) ),
             this, SLOT( slotConfigDirty() ) );
    dirWatch->addFile( KStandardDirs::locateLocal( "config", m_config.name() ) );

    KConfigGroup cfgGrp( &m_config, "akonadi_nepomuk_email_feeder" );
    m_isEnabled = cfgGrp.readEntry( "Enabled", true );
}

bool IndexerConfig::isEnabled()
{
    return m_isEnabled;
}

void IndexerConfig::slotConfigDirty()
{
    m_config.reparseConfiguration();

    KConfigGroup cfgGrp( &m_config, "akonadi_nepomuk_email_feeder" );
    bool enabled = cfgGrp.readEntry( "Enabled", true );
    if( m_isEnabled != enabled ) {
        m_isEnabled = enabled;
        emit configChanged();
    }
}

