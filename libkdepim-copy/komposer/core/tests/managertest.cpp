/**
 * managertest.cpp
 *
 * Copyright (C)  2004  Zack Rusin <zack@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
#include "managertest.h"

#include "pluginmanager.h"
using namespace Komposer;

ManagerTest::ManagerTest( QObject* parent )
    : QObject( parent )
{
    m_manager = new PluginManager( this );
}


void ManagerTest::allTests()
{
    CHECK( m_manager->availablePlugins().isEmpty(), true );
    CHECK( m_manager->loadedPlugins().empty(), true );
    CHECK( m_manager->plugin( "non-existing" ), ( Plugin* )0 );
    m_manager->loadAllPlugins();
    CHECK( m_manager->loadedPlugins().empty(), true );
    m_manager->shutdown();
}

#include "managertest.moc"
